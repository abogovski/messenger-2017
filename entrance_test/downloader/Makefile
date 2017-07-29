all: downloader.cpp
	g++ downloader.cpp -lcurl -levent -odownloader

clean:
	rm -f downloader

run_test:
	./downloader https://curl.haxx.se/libcurl/c/hiperfifo.html https://curl.haxx.se/libcurl/c/curl_multi_info_read.html
