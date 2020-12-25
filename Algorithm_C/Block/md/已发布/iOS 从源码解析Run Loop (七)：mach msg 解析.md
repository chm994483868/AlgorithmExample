# iOS 从源码解析Run Loop (七)：mach msg 解析
> &emsp;经过前面 NSPort 内容的学习，我们大概能对 port 有一点模糊的概念，那么本篇我们学习一下 Mach 消息。

&emsp;在前面 NSPort 的学习中提到：`handleMachMessage:` 提供以 msg_header_t（mach_msg_header_t） 结构开头的 "原始 Mach 消息" 的消息，以及 NSMachPort 中： `+ (NSPort *)portWithMachPort:(uint32_t)machPort;` 函数中 `machPort` 参数原始为 mach_port_t 类型。
## mach_msg_header_t
&emsp;
```c++
typedef struct{
    // typedef unsigned int mach_msg_bits_t;
    mach_msg_bits_t       msgh_bits;
    
    // typedef natural_t mach_msg_size_t; => typedef __darwin_natural_t natural_t; => typedef unsigned int __darwin_natural_t;
    mach_msg_size_t       msgh_size;
    
    // typedef __darwin_mach_port_t mach_port_t; => 
    mach_port_t           msgh_remote_port;
    mach_port_t           msgh_local_port;
    
    mach_port_name_t      msgh_voucher_port;
    mach_msg_id_t         msgh_id;
} mach_msg_header_t;
```






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
+ [mach_absolute_time 使用](https://www.cnblogs.com/zpsoe/p/6994811.html)
+ [iOS 探讨之 mach_absolute_time](https://blog.csdn.net/yanglei3kyou/article/details/86679177)
+ [iOS多线程——RunLoop与GCD、AutoreleasePool你要知道的iOS多线程NSThread、GCD、NSOperation、RunLoop都在这里](https://cloud.tencent.com/developer/article/1089330)
+ [Mach原语：一切以消息为媒介](https://www.jianshu.com/p/284b1777586c?nomobile=yes)
+ [操作系统双重模式和中断机制和定时器概念](https://blog.csdn.net/zcmuczx/article/details/79937023)


## Mach Overview
&emsp;OS X 内核的基本服务和原语（fundamental services and primitives）基于 Mach 3.0。苹果已经修改并扩展了 Mach，以更好地满足 OS X 的功能和性能目标。

&emsp;Mach 3.0 最初被认为是一个简单，可扩展的通信微内核。它能够作为独立的内核运行，并与其他传统的操作系统服务（例如 I/O，文件系统和作为用户模式服务器运行的网络堆栈）一起运行。

&emsp;但是，在 OS X 中，Mach 与其他内核组件链接到单个内核地址空间中。这主要是为了提高性能；在链接的组件之间进行直接调用比在单独的任务之间发送消息或进行远程过程调用（RPC）要快得多。这种模块化结构导致了比单核内核所允许的更健壮和可扩展的系统，而没有纯微内核的性能损失。

&emsp;因此，在 OS X 中，Mach 主要不是客户端和服务器之间的通信中心。相反，它的价值包括其抽象性，可扩展性和灵活性。特别是，Mach 提供:
+ 以通信通道（communication channels）（例如 port）作为对象引用的 object-based 的 APIs。（NSPort 文档第一句话：`NSPort` 表示通信通道（communication channel）的抽象类。）
+ 高度并行执行，包括抢占调度线程和对 SMP 的支持。
+ 灵活的调度框架，支持实时使用。
+ 一组完整的 IPC 原语，包括消息传递、RPC、同步和通知。
+ 支持大型虚拟地址空间、共享内存区域和持久存储支持的内存对象。
+ 经验证的可扩展性和可移植性，例如跨指令集体系结构和分布式环境。
+ 安全和资源管理作为设计的基本原则；所有资源都是虚拟化的。
