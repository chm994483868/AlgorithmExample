# iOS 从源码解析Runloop (一)：runloop 基本概念篇

> &emsp;什么是RunLoop，顾名思义，RunLoop就是在‘跑圈’，其本质是一个do while循环。RunLoop提供了这么一种机制，当有任务处理时，线程的RunLoop会保持忙碌，而在没有任何任务处理时，会让线程休眠，从而让出CPU。当再次有任务需要处理时，RunLoop会被唤醒，来处理事件，直到任务处理完毕，再次进入休眠。

&emsp;


## 参考链接
**参考链接:🔗**
+ [runloop 源码](https://opensource.apple.com/tarballs/CF/)
+ [Run Loops 官方文档](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/Multithreading/RunLoopManagement/RunLoopManagement.html#//apple_ref/doc/uid/10000057i-CH16-SW1)
+ [iOS RunLoop完全指南](https://blog.csdn.net/u013378438/article/details/80239686)
+ [iOS源码解析: runloop的底层数据结构](https://juejin.cn/post/6844904090330234894)
+ [iOS源码解析: runloop的运行原理](https://juejin.cn/post/6844904090166624270)
+ [深入理解RunLoop](https://blog.ibireme.com/2015/05/18/runloop/)
+ [iOS底层学习 - 深入RunLoop](https://juejin.cn/post/6844903973665636360)
