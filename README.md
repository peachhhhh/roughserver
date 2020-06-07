## 基于C++的多线程Web静态服务器

实现了一个C++编写的Linux下多线程Web静态服务器  
* 基于事件驱动和多路IO复用  
* 使用固定线程池和epoll边沿触发模式(ET模式)  
* 主线程负责接受套接字，将连接轮询分发给线程池中的IO线程处理  
* 使用封装为RAII形式的互斥器和条件变量，实现主线程和IO线程间的线程安全，在每个IO线程内执行事件循环，处理活跃事件  
* 将HTTPData类的处理读写的函数注册到EventLoop，由IO线程在事件循环时处理。