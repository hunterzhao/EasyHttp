#EasyHttp 最简单的http服务器


##流程：
- 代理发送过来请求，服务器建立client_sock套接字，创建进程处理请求 accept_request
- 逐行从套接字中读取http请求报文，判断请求类型 GET \ POST 判断 url 根据url上的参数读取服务器上的文件或者准备执行cgi文件
- 根据具体的方法 将参数存储到 meth_env （putenv）中
- 创建子进程执行cgi程序，父进程通过管道向子进程传入数据，从管道读取子进程输出的数据 父子进程通信示意图 1-1
- 该服务器采用python编写cgi程序


##注意：
1. strlen 与 sizeof的区别 如果send中使用 sizeof 很有可能由于缓冲区被占满导致 send 被阻塞
2. 用户的代理（浏览器）会检查http的响应报文格式，所以http服务器返回的报文一定要严格按照要求编写




**项目地址：https://github.com/hunterzhao/EasyHttp**


##参考：
1. [http 请求报文 与 响应报文][1]
2. [GDB 调试多进程方法][2]
3. [python cgi编写方法][3]


[1]: http://network.chinabyte.com/401/13238901.shtml
[2]: http://blog.csdn.net/pbymw8iwm/article/details/7876797
[3]: http://www.runoob.com/python/python-cgi.html