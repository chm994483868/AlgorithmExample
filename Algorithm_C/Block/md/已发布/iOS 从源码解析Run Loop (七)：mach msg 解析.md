# iOS 从源码解析Run Loop (七)：mach msg 解析
> &emsp;经过前面 NSPort 内容的学习，我们大概对 port 在线程通信中的使用有一点模糊的概念了，那么本篇我们学习一下 Mach。

&emsp;Run Loop 最核心的事情就是保证线程在没有消息时休眠以避免占用系统资源，有消息时能够及时唤醒。Run Loop 的这个机制完全依靠系统内核来完成，具体来说是苹果操作系统核心组件 Darwin 中的 Mach 来完成的。**Mach 与 BSD、File System、Mach、Networking 共同位于 Kernel and Device Drivers 层。**

&emsp;Mach 是 Darwin 的核心，可以说是内核的核心，提供了进程间通信（IPC）、处理器调度等基础服务。在 Mach 中，进程、线程间的通信是以消息的方式来完成的，消息在两个 Port 之间进行传递（这也正是 Source1 之所以称之为 Port-based Source 的原因，因为它就是依靠系统发送消息到指定的 Port 来触发）。消息的发送和接收使用 `mach_msg` 函数，而 `mach_msg` 的本质是调用了 `mach_msg_trap`，这相当于一个系统调用，会触发内核态与用户态的切换。

&emsp;当程序没有 source/timer 需要处理时，run loop 会进入休眠状态。通过上篇 \__CFRunLoopRun 函数的学习，已知 run loop 进入休眠状态时会调用 \__CFRunLoopServiceMachPort 函数，该函数内部即调用了 `mach_msg` 相关的函数操作使得系统内核状态发生改变：用户态切换至内核态。

&emsp;点击 App 图标，App 启动完成处于静止状态时，此时主线程的 run loop 会进入休眠状态，通过在主线程的 run loop 添加 CFRunLoopObserverRef 在回调函数中可看到主线程的 run loop 的最后活动状态是 kCFRunLoopBeforeWaiting，此时点击 Xcode 控制台底部的 Pause program execution 按钮，可看到主线程的调用栈停在了 mach_msg_trap，在控制台输入 bt 回车，可看到如下调用栈：
```c++
(lldb) bt
* thread #1, queue = 'com.apple.main-thread', stop reason = signal SIGSTOP
  * frame #0: 0x00007fff60c51e6e libsystem_kernel.dylib`mach_msg_trap + 10 // ⬅️ mach_msg_trap
    frame #1: 0x00007fff60c521e0 libsystem_kernel.dylib`mach_msg + 60
    frame #2: 0x00007fff2038e9bc CoreFoundation`__CFRunLoopServiceMachPort + 316 // ⬅️ __CFRunLoopServiceMachPort 进入休眠
    frame #3: 0x00007fff203890c5 CoreFoundation`__CFRunLoopRun + 1284 // ⬅️ __CFRunLoopRun
    frame #4: 0x00007fff203886d6 CoreFoundation`CFRunLoopRunSpecific + 567 // ⬅️  CFRunLoopRunSpecific
    frame #5: 0x00007fff2bededb3 GraphicsServices`GSEventRunModal + 139
    frame #6: 0x00007fff24690e0b UIKitCore`-[UIApplication _run] + 912 // ⬅️ main run loop 启动
    frame #7: 0x00007fff24695cbc UIKitCore`UIApplicationMain + 101
    frame #8: 0x0000000107121d4a Simple_iOS`main(argc=1, argv=0x00007ffee8addcf8) at main.m:20:12
    frame #9: 0x00007fff202593e9 libdyld.dylib`start + 1
    frame #10: 0x00007fff202593e9 libdyld.dylib`start + 1
(lldb) 
```
&emsp;可看到 run loop 从启动函数一步步进入到 mach_msg_trap。

&emsp;mach_msg 函数可以设置 timeout 参数，如果在 timeout 到来之前没有读到 msg，当前线程的 run loop 会处于休眠状态。

&emsp;通过上面简略的分析大概我们知道了 `mach_msg` 函数的使用是和 Port 相关的，那么从第一看 run loop 到现在我们在代码层面有遇到过哪些 Port 呢？下面我们就一起回顾一下。
## \__CFRunLoop \_wakeUpPort
&emsp;struct \__CFRunLoop 结构体的成员变量 \__CFPort \_wakeUpPort 应该是我们见到的第一个 Port，它被用于 `CFRunLoopWakeUp` 函数来唤醒 run loop。
```c++
struct __CFRunLoop {
    ...
    // typedef mach_port_t __CFPort;
    __CFPort _wakeUpPort; // used for CFRunLoopWakeUp 用于 CFRunLoopWakeUp 函数
    ...
};
```
&emsp;当为线程创建 run loop 对象时会对直接对 run loop 的 \_wakeUpPort 成员变量进行初始化。在 `__CFRunLoopCreate` 函数中初始化 \_wakeUpPort。
```c++
static CFRunLoopRef __CFRunLoopCreate(pthread_t t) {
    ...
    loop->_wakeUpPort = __CFPortAllocate();
    if (CFPORT_NULL == loop->_wakeUpPort) HALT; // 创建失败的话会直接 crash
    ...
}
```










&emsp;在前面 NSPort 的学习中提到：`handleMachMessage:` 提供以 msg_header_t（mach_msg_header_t） 结构开头的 "原始 Mach 消息" 的消息，以及 NSMachPort 中： `+ (NSPort *)portWithMachPort:(uint32_t)machPort;` 函数中 `machPort` 参数原始为 mach_port_t 类型。

## mach_msg_header_t
&emsp;
```c++
typedef struct{
    // typedef unsigned int mach_msg_bits_t;
    mach_msg_bits_t       msgh_bits;
    
    // typedef natural_t mach_msg_size_t; => 
    // typedef __darwin_natural_t natural_t; => 
    // typedef unsigned int __darwin_natural_t;
    // 实际类型是 unsigned int
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
