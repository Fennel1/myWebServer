## ET、LT、EPOLLONESHOT
- LT水平触发模式
    - epoll_wait检测到文件描述符有事件发生，则将其通知给应用程序，应用程序可以不立即处理该事件。
    - 当下一次调用epoll_wait时，epoll_wait还会再次向应用程序报告此事件，直至被处理
- ET边缘触发模式
    - epoll_wait检测到文件描述符有事件发生，则将其通知给应用程序，应用程序必须立即处理该事件
    - 必须要一次性将数据读取完，使用非阻塞I/O，读取到出现eagain
- EPOLLONESHOT
    - 一个线程读取某个socket上的数据后开始处理数据，在处理过程中该socket上又有新数据可读，此时另一个线程被唤醒读取，此时出现两个线程处理同一个socket
    - 我们期望的是一个socket连接在任一时刻都只被一个线程处理，通过epoll_ctl对该文件描述符注册epolloneshot事件，一个线程处理socket时，其他线程将无法处理，当该线程处理完后，需要通过epoll_ctl重置epolloneshot事件

## 请求报文
HTTP请求报文由请求行（request line）、请求头部（header）、空行和请求数据四个部分组成。

- GET
```
1    GET /562f25980001b1b106000338.jpg HTTP/1.1
2    Host:img.mukewang.com
3    User-Agent:Mozilla/5.0 (Windows NT 10.0; WOW64)
4    AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.106 Safari/537.36
5    Accept:image/webp,image/*,*/*;q=0.8
6    Referer:http://www.imooc.com/
7    Accept-Encoding:gzip, deflate, sdch
8    Accept-Language:zh-CN,zh;q=0.8
9    空行
10   请求数据为空
```

- POST
```
1    POST / HTTP1.1
2    Host:www.wrox.com
3    User-Agent:Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 2.0.50727; .NET CLR 3.0.04506.648; .NET CLR 3.5.21022)
4    Content-Type:application/x-www-form-urlencoded
5    Content-Length:40
6    Connection: Keep-Alive
7    空行
8    name=Professional%20Ajax&publisher=Wiley
```

## 响应报文
HTTP响应也由四个部分组成，分别是：状态行、消息报头、空行和响应正文。

```
1   HTTP/1.1 200 OK
2   Date: Fri, 22 May 2009 06:07:21 GMT
3   Content-Type: text/html; charset=UTF-8
4   空行
5   <html>
6      <head></head>
7      <body>
8            <!--body goes here-->
9      </body>
10  </html>
```

## http报文处理流程

- 浏览器端发出http连接请求，主线程创建http对象接收请求并将所有数据读入对应buffer，将该对象插入任务队列，工作线程从任务队列中取出一个任务进行处理。

- 工作线程取出任务后，调用process_read函数，通过主、从状态机对请求报文进行解析。

- 解析完之后，跳转do_request函数生成响应报文，通过process_write写入buffer，返回给浏览器端。

## 说明

**主状态机**
- CHECK_STATE_REQUESTLINE，解析请求行
- CHECK_STATE_HEADER，解析请求头
- CHECK_STATE_CONTENT，解析消息体，仅用于解析POST请求

<br>

**从状态机**
- LINE_OK，完整读取一行
- LINE_BAD，报文语法有误
- LINE_OPEN，读取的行不完整

<br>

**HTTP_CODE**
- NO_REQUEST, 请求不完整，需要继续读取请求报文数据
- GET_REQUEST, 获得了完整的HTTP请求
- BAD_REQUEST, HTTP请求报文有语法错误
- INTERNAL_ERROR, 服务器内部错误，该结果在主状态机逻辑switch的default下，一般不会触发