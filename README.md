# messenger-2017

https://sphere.mail.ru/blog/topic/view/9408/

## entrace_test

### entrance_test/downloader
Cross-platform command-line asynchronous downloader. Ignores file content and just prints file size.
Written from scratch.

Dependencies: libcurl, libevent

#### Usage
downloader &lt;urls&gt;

### entrance_test/server
Asynchronous http server. Based on HTTP Server example from boost::asio.

Dependencies: boost::asio 

#### Usage
##### Start
async_server &lt;port&gt;
##### Get File
localhost:&lt;port&gt;/get/file_from_working_dir
