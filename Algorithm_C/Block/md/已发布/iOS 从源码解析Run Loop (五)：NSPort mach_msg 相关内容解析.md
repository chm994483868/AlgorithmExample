# iOS 从源码解析Run Loop (五)：NSPort mach_msg 相关内容解析

> &emsp;Port 相关的内容不知道如何入手学习，那么就从 NSPort 开始吧。

## NSPort 
&emsp;`NSPort` 表示通信通道（communication channel）的抽象类。
```c++
@interface NSPort : NSObject <NSCopying, NSCoding>
```
&emsp;通信发生在 NSPort 对象之间，这些对象通常位于不同的线程或任务中。分布式对象系统（distributed objects system）使用 NSPort 对象来回发送 NSPortMessage 对象。尽可能使用分布式对象（distributed objects）实现应用程序间通信（interapplication communication），并且仅在必要时使用 NSPort 对象。

&emsp;要接收传入的消息，必须将 NSPort 对象作为 input sources 添加到 NSRunLoop 对象中。 NSConnection 对象在初始化时会自动添加其接收端口（receive port）。

&emsp;当 NSPort 对象接收到端口消息时，它将消息通过 `handleMachMessage:` 或 `handlePortMessage:` 消息转发给其 delegate。delegate 应仅实现这些方法中的一种，以所需的任何形式处理传入的消息。`handleMachMessage:` 提供以 msg_header_t 结构开头的 "原始 Mach 消息" 的消息。`handlePortMessage:` 将消息作为 NSPortMessage 对象提供，它是 Mach 消息的面向对象封装。如果尚未设置委托，NSPort 对象将处理消息本身。

&emsp;使用完端口对象后，必须先显式地使端口对象无效，然后再向其发送释放消息。类似地，如果应用程序使用垃圾回收，则必须在删除对端口对象的任何强引用之前使其无效。如果不使端口无效，则生成的端口对象可能会延迟并导致内存泄漏。要使端口对象无效，请调用其 `invalidate` 方法。

&emsp;基础定义了NSPort的三个具体子类。NSMachPort和NSMessagePort只允许本地（在同一台机器上）通信。NSSocketPort允许本地和远程通信，但对于本地情况，可能比其他端口更昂贵。使用allocWithZone:或port创建NSPort对象时，将改为创建NSMachPort对象。

## 参考链接
**参考链接:🔗**
+ [runloop 源码](https://opensource.apple.com/tarballs/CF/)
+ [Run Loops 官方文档](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/Multithreading/RunLoopManagement/RunLoopManagement.html#//apple_ref/doc/uid/10000057i-CH16-SW1)
+ [iOS RunLoop完全指南](https://blog.csdn.net/u013378438/article/details/80239686)
+ [iOS源码解析: runloop的底层数据结构](https://juejin.cn/post/6844904090330234894)
+ [iOS源码解析: runloop的运行原理](https://juejin.cn/post/6844904090166624270)
+ [深入理解RunLoop](https://blog.ibireme.com/2015/05/18/runloop/)
+ [iOS底层学习 - 深入RunLoop](https://juejin.cn/post/6844903973665636360)
+ [一份走心的runloop源码分析](https://cloud.tencent.com/developer/article/1633329)
+ [NSRunLoop](https://www.cnblogs.com/wsnb/p/4753685.html)
+ [iOS刨根问底-深入理解RunLoop](https://www.cnblogs.com/kenshincui/p/6823841.html)
+ [RunLoop总结与面试](https://www.jianshu.com/p/3ccde737d3f3)
+ [Runloop-实际开发你想用的应用场景](https://juejin.cn/post/6889769418541252615)
+ [RunLoop 源码阅读](https://juejin.cn/post/6844903592369848328#heading-17)
+ [do {...} while (0) 在宏定义中的作用](https://www.cnblogs.com/lanxuezaipiao/p/3535626.html)
+ [CFRunLoop 源码学习笔记(CF-1151.16)](https://www.cnblogs.com/chengsh/p/8629605.html)
+ [操作系统大端模式和小端模式](https://www.cnblogs.com/wuyuankun/p/3930829.html)
+ [CFBag](https://nshipster.cn/cfbag/)
