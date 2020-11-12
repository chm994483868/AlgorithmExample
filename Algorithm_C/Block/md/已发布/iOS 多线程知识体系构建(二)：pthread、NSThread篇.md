# iOS 多线程知识体系构建(二)：Pthread、NSThread篇

> &emsp;本篇首先来学习 iOS 多线程技术中的 NSThread。⛽️⛽️

## pthread
> &emsp;可移植操作系统接口（英语：Portable Operating System Interface，缩写为POSIX）是 IEEE（电气和电子工程师协会）为要在各种 UNIX 操作系统上运行软件，而定义 API 的一系列互相关联的标准的总称，其正式称呼为 IEEE Std 1003，而国际标准名称为 ISO/IEC 9945。此标准源于一个大约开始于1985 年的项目。POSIX 这个名称是由理查德·斯托曼（RMS）应 IEEE 的要求而提议的一个易于记忆的名称。它基本上是 Portable Operating System Interface（可移植操作系统接口）的缩写，而 X 则表明其对 Unix API 的传承。

&emsp;`Pthread` 一般指 POSIX 线程。 POSIX 线程（POSIX Threads，常被缩写为 Pthreads）是 POSIX 的线程标准，定义了创建和操纵线程的一套 API。
&emsp;实现 POSIX 线程标准的库常被称作 Pthreads，一般用于 Unix-like POSIX  系统，如 Linux、Solaris。但是 Microsoft Windows 上的实现也存在，例如直接使用 Windows API 实现的第三方库 pthreads-w32，而利用 Windows 的 SFU/SUA 子系统，则可以使用微软提供的一部分原生 POSIX API。

是一套通用的多线程  `API`，可以在 `Unix/Linux/Windows` 等系统跨平台使用，使用 `C` 语言编写，需要程序员自己管理线程的生命周期。


## 参考链接
**参考链接:🔗**
+ [pthread-百度百科词条](https://baike.baidu.com/item/POSIX线程?fromtitle=Pthread&fromid=4623312)
+ [在Linux中使用线程](https://blog.csdn.net/jiajun2001/article/details/12624923)
+ [iOS多线程：『pthread、NSThread』详尽总结](https://juejin.im/post/6844903556009443335)
+ [iOS 多线程系列 -- pthread](https://www.jianshu.com/p/291598217865)
+ [iOS底层原理总结 - pthreads](https://www.jianshu.com/p/4434f18c5a95)
+ [C语言多线程pthread库相关函数说明](https://www.cnblogs.com/mq0036/p/3710475.html)
+ [iOS多线程中的实际方案之一pthread](https://www.jianshu.com/p/cfc6e7d2316a)

