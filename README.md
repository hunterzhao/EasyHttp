#EasyHttp 最简单的http服务器


##说明：
这是一个C语言编写的http服务器简单实现，使用python语言作为cgi程序处理用户的表单输入。通过这个项目有利于理解http服务流程，以及多线程调试等诸多知识点。


##使用方法：
1. 平台：centos7 编译器：gcc 4.8.4
2. 下载项目，修改htdocs目录下easycgi.py 的运行权限 chmod
3. cd EasyHttp && make && ./httpd
4.  可以通过修改 index.html 表格中的请求方式来测试get post方法
5.  建议初学者通过逐步调试的方式学习代码，每个函数都有详细的注释哦~

##流程：
- 代理发送过来请求，服务器建立client_sock套接字，创建进程处理请求 accept_request
- 逐行从套接字中读取http请求报文，判断请求类型 GET \ POST 判断 url 根据url上的参数读取服务器上的文件或者准备执行cgi文件
- 根据具体的方法 将参数存储到 meth_env （putenv）中
- 创建子进程执行cgi程序，父进程通过管道向子进程传入数据，从管道读取子进程输出的数据 父子进程通信示意图如下
![父子进程通信](https://github.com/hunterzhao/EasyHttp/blob/master/pic/3.png?raw=true)
- 该服务器采用python编写cgi程序


##注意：
1. strlen 与 sizeof的区别 如果send中使用 sizeof 很有可能由于缓冲区被占满导致 send 被阻塞
2. 用户的代理（浏览器）会检查http的响应报文格式，所以http服务器返回的报文一定要严格按照要求编写



##测试：
1.  浏览器第一次请求到页面

![浏览器第一次请求到页面](https://github.com/hunterzhao/EasyHttp/blob/master/pic/1.png?raw=true)

2.  浏览器提交用户的输入，并返回执行结果

![浏览器提交用户的输入，并返回执行结果](https://github.com/hunterzhao/EasyHttp/blob/master/pic/2.png?raw=true)

##感谢：
**谢谢您的查看，水平有限献丑了,如果对您有帮助请给我点star哦 :)**

##参考：
1. [http 请求报文 与 响应报文][1]
2. [GDB 调试多进程方法][2]
3. [python cgi编写方法][3]

##后续：
1. 支持图片
2. https
3. 大量的并发请求

[1]: http://network.chinabyte.com/401/13238901.shtml
[2]: http://blog.csdn.net/pbymw8iwm/article/details/7876797
[3]: http://www.runoob.com/python/python-cgi.html