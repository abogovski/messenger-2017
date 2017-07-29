#include <curl/curl.h>
#include <string>
#include <iostream>
#include <event2/event.h>
#include <unordered_map>

size_t write_cbcurl(char *ptr, size_t size, size_t nmemb, void *userdata);
void event_cbev(evutil_socket_t socket_fd, short kind, void* userp);
void socket_cbcurl(CURL*, curl_socket_t, int, void*, void*); 
void timer_cbev(evutil_socket_t fd, short what, void *userp);
void timer_cbcurl(CURLM* multi_handle, long timeout_ms, void* userp);


static void errmsg_curl(const char* url, const char* msg, CURLcode code) {
    std::cerr << msg << " failed with code " << curl_easy_strerror(code)
              << " for url " << url << std::endl;
}

static void errmsg_curlm(const char* msg, CURLMcode code) {
    std::cerr << msg << " failed with code " << curl_multi_strerror(code) << std::endl;
}


class AsyncDownloader
{
public:
    struct Job;
    friend void event_cbev(evutil_socket_t, short, void*);
    friend void socket_cbcurl(CURL*, curl_socket_t, int, void*, void*);
    friend void timer_cbcurl(CURLM*, long, void*);

protected:
    CURLM* multi_handle;
    event_base* evbase;
    event* evtimeout;
    std::unordered_map<CURL*, Job*> jobs;
    AsyncDownloader() = default;
    ~AsyncDownloader() = default;

public:
    static AsyncDownloader* create() {
        CURLM* multi_handle = curl_multi_init();
        if (multi_handle == NULL) {
            std::cerr << "curl_multi_init falied" << std::endl;
            return NULL;
        }

        event_base* evbase = event_base_new();
        if (evbase == NULL) {
            std::cerr << "event_base_new failed" << std::endl;
            curl_multi_cleanup(multi_handle);
            return NULL;
        }

        AsyncDownloader* async_downloader = new AsyncDownloader();
        async_downloader->multi_handle = multi_handle;
        async_downloader->evbase = evbase;
        
        curl_multi_setopt(multi_handle, CURLMOPT_TIMERFUNCTION, timer_cbcurl);
        curl_multi_setopt(multi_handle, CURLMOPT_TIMERDATA, async_downloader);

        curl_multi_setopt(multi_handle, CURLMOPT_SOCKETFUNCTION, socket_cbcurl);
        curl_multi_setopt(multi_handle, CURLMOPT_SOCKETDATA, async_downloader);

        return async_downloader;
    }

    static void destroy(AsyncDownloader* async_downloader) {
        if (async_downloader->multi_handle != NULL) {
            curl_multi_cleanup(async_downloader->multi_handle);
            async_downloader->multi_handle = NULL;
        }

        if (async_downloader->evbase != NULL) {
            event_base_free(async_downloader->evbase);
            async_downloader->evbase = NULL;
        }

        delete async_downloader;
    }

    struct Job
    {
        unsigned long long file_size;
        CURL* curl_easy;
        event* ev;
        const char* url;
        AsyncDownloader* async_downloader;
    };

    void addJob(const char* url) {
        CURL *easy_handle = curl_easy_init();
        if (easy_handle == NULL) {
            std::cerr << "addJob: curl_easy_init failed for url " << url << std::endl;
            exit(1);
        }

        CURLcode ccode = curl_easy_setopt(easy_handle, CURLOPT_URL, url);
        if (ccode != CURLE_OK) {
            errmsg_curl(url, "addJob: curl_easy_setopt URL", ccode);
            curl_easy_cleanup(easy_handle);
            exit(1);
        }

        ccode = curl_easy_setopt(easy_handle, CURLOPT_WRITEFUNCTION, write_cbcurl);
        if (ccode != CURLE_OK) {
            errmsg_curl(url, "addJob: curl_easy_setopt WRITEFUNCTION", ccode);
            curl_easy_cleanup(easy_handle);
            exit(1);
        }

        Job* job = new Job();
        job->url = url;
        job->curl_easy = easy_handle;

        ccode = curl_easy_setopt(easy_handle, CURLOPT_WRITEDATA, job);
        if (ccode != CURLE_OK) {
            errmsg_curl(url, "addJob: curl_easy_setopt WRITEDATA", ccode);
            curl_easy_cleanup(easy_handle);
            delete job;
            job = NULL;
            exit(1);
        }

        //curl_easy_setopt(easy_handle, CURLOPT_VERBOSE, 1); 

        jobs.insert({job->curl_easy, job});

        CURLMcode cmcode = curl_multi_add_handle(multi_handle, job->curl_easy);
        if (cmcode != CURLM_OK) {
            errmsg_curlm("addJob: curl_multi_add_handle", cmcode);
            exit(1);
        }
    }

    int timeout() { 
        int running = 0;
        curl_multi_socket_action(multi_handle, CURL_SOCKET_TIMEOUT, 0, &running);
        return running;
    }

    event_base* get_event_base() {
        return evbase;
    }
};

size_t write_cbcurl(char *ptr, size_t size, size_t nmemb, void *userdata) {
    auto job = (AsyncDownloader::Job*)userdata;
    job->file_size += nmemb * size;
    return nmemb * size;
}

void event_cbev(evutil_socket_t socket_fd, short kind, void* userp) {
    AsyncDownloader* async_downloader = (AsyncDownloader*)userp;

    int running = 0;
    if (kind & (EV_READ | EV_WRITE)) {
        int flg = 0;
        if (kind & EV_READ) {
            flg |= CURL_CSELECT_IN;
        }
        if (kind & EV_WRITE) {
            flg |= CURL_CSELECT_OUT;
        }
        CURLMcode cmcode = curl_multi_socket_action(async_downloader->multi_handle, socket_fd, flg, &running);
	if (cmcode != CURLM_OK) {
	    errmsg_curlm("cbevent: curl_multi_socket_action", cmcode);
            event_base_loopbreak(async_downloader->evbase);
	}
    }
    
    CURLMsg* m = NULL;
    do {
        int msgq = 0;
        m = curl_multi_info_read(async_downloader->multi_handle, &msgq);
        if (m && (m->msg == CURLMSG_DONE)) {
            AsyncDownloader::Job* finished_job = async_downloader->jobs.at(m->easy_handle);
            async_downloader->jobs.erase(m->easy_handle);

            std::cout << "[DOWNLOADED] " << finished_job->file_size
                      << " bytes from " << finished_job->url << std::endl;

            CURLMcode cmcode = curl_multi_remove_handle(async_downloader->multi_handle, finished_job->curl_easy);
            if (cmcode != CURLM_OK) {
                errmsg_curlm("cbevent: curl_multi_remove_handle", cmcode);
                exit(1);
            }

            if (finished_job->curl_easy != NULL) {
                curl_easy_cleanup(finished_job->curl_easy);
                finished_job->curl_easy = NULL;
            }

            if (finished_job->ev != NULL) {
                event_del(finished_job->ev);
                event_free(finished_job->ev);
                finished_job->ev = NULL;
            }
        }
    } while(m != NULL);
}

void socket_cbcurl(CURL* easy_handle, curl_socket_t sockfd, int what, void *cbp, void *sockp) {
    AsyncDownloader* async_downloader = (AsyncDownloader*)cbp;

    if (what != CURL_POLL_REMOVE) {
        int ev_flags = EV_PERSIST;
        if (what == CURL_POLL_IN || what == CURL_POLL_INOUT) {
            ev_flags |= EV_READ; 
        }
        if (what == CURL_POLL_OUT || what == CURL_POLL_INOUT) {
            ev_flags |= EV_WRITE;
        }
 
        if (async_downloader->jobs.at(easy_handle)->ev != NULL) {
            event_del(async_downloader->jobs.at(easy_handle)->ev);
            event_free(async_downloader->jobs.at(easy_handle)->ev);
            async_downloader->jobs.at(easy_handle)->ev = NULL;          
        }
 
        event* ev = event_new(async_downloader->get_event_base(), sockfd, ev_flags, event_cbev, async_downloader);
        int evrv = event_add(ev, NULL);
        if (evrv == -1) {		
            std::cerr << "event_add failed" << std::endl;
            exit(1);
        }
        async_downloader->jobs.at(easy_handle)->ev = ev;
    } else {
        // do nothing on CURL_POLL_REMOVE
    }
}

void timer_cbev(evutil_socket_t fd, short what, void *userp) {
    AsyncDownloader* async_downloader = (AsyncDownloader*)userp;
    async_downloader->timeout();
}

void timer_cbcurl(CURLM* multi_handle, long timeout_ms, void* userp) {
    AsyncDownloader* async_downloader = (AsyncDownloader*)userp;    
    if (timeout_ms > 0) {
        timeval tv;
        tv.tv_sec = timeout_ms/1000;
        tv.tv_usec = (timeout_ms%1000)*1000;
        if (async_downloader->evtimeout == NULL) {
            event* ev= evtimer_new(async_downloader->evbase, timer_cbev, async_downloader);
            if (ev == NULL) {
                std::cerr << "falied to create timeout event" << std::endl;
                exit(1);
            }
            async_downloader->evtimeout = ev;
        }
        int evrv = evtimer_add(async_downloader->evtimeout, &tv);
        if (evrv != 0) {
            std::cerr << "failed to add timeout event" << std::endl;
        }
    } else if (timeout_ms == 0) {
        timer_cbev(-1, 0, async_downloader);
    } else {
        event_del(async_downloader->evtimeout);
        event_free(async_downloader->evtimeout);
        async_downloader->evtimeout = NULL;
    }
}

int main(int argc, char *argv[])
{
    // init
    CURLcode ccode = curl_global_init(CURL_GLOBAL_NOTHING);
    if (ccode != CURLE_OK) {
        errmsg_curl("None", "curl_global_init", ccode);
        exit(1);
    }

    AsyncDownloader* async_downloader = AsyncDownloader::create();
    for (int i = 1; i < argc; ++i) {
        async_downloader->addJob(argv[i]);
    }
    async_downloader->timeout();

    // event loop
    event_base_dispatch(async_downloader->get_event_base());	

    // finilize
    AsyncDownloader::destroy(async_downloader);
    async_downloader = NULL;
    curl_global_cleanup();
    return 0;
}

