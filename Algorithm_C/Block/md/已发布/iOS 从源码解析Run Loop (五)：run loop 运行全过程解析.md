# iOS 从源码解析Run Loop (五)：run loop 运行全过程解析

> &emsp;前面几篇算是把 run loop 相关的数据结构都看完了，也算是把 run loop 开启运行的前期数据都准备好了，下面我们开始正式进入 run loop 的整个的运行过程的探索和学习。⛽️⛽️

&emsp;查看 CFRunLoop.h 文件，看到涉及 run loop 运行的函数有两个 `CFRunLoopRun` 和 `CFRunLoopRunInMode` 下面我们跟着源码学习一下这两个函数。
## CFRunLoopRun/CFRunLoopRunInMode
&emsp;`CFRunLoopRun` 函数同 NSRunLoop 的 `- (void)run;` 函数，无限期地以其默认模式运行当前线程的 CFRunLoop 对象。当前线程的运行循环将以默认模式运行，直到使用 `CFRunLoopStop` 停止 run loop 或将所有 Sources 和 Timers 从默认运行循环模式中移除为止。run loop 可以递归运行，你可以从任何 run loop 调用中调用 `CFRunLoopRun` 函数，并在当前线程的调用堆栈上创建嵌套的 run loop 激活。

&emsp;`CFRunLoopRunInMode` 在特定模式下运行当前线程的 CFRunLoop 对象。
+ `mode`：以运行循环模式运行。模式可以是任意 CFString。尽管运行循环模式需要至少包含一个源或计时器才能运行，但是你无需显式创建运行循环模式。
+ `seconds`：运行 run loop 的时间长度。如果为 0，则返回之前仅运行循环一次；如果有多个源或计时器准备立即触发，那么无论 returnAfterSourceHandled 的值如何，都将仅触发一个（如果一个是 version 0 source，则可能触发两个）。
+ `returnAfterSourceHandled`：一个标志，指示 run loop 是否应在处理一个源之后退出。如果为 false，则运行循环将继续处理事件，直到经过 `seconds`。

&emsp;返回一个值，指示 run loop 退出的原因。可能的值如下所述：


运行循环可以递归运行。您可以从任何运行循环调用中调用CFRunLoopRunInMode，并在当前线程的调用堆栈上创建嵌套的运行循环激活。在调用中可以运行的模式不受限制。您可以创建另一个在任何可用的运行循环模式下运行的运行循环激活，包括调用堆栈中已经运行的任何模式。

&emsp;下面是 `CFRunLoopRun` 函数的定义:
```c++
/* Reasons for CFRunLoopRunInMode() to Return */
// CFRunLoopRunInMode 函数返回的原因
enum {
    kCFRunLoopRunFinished = 1,
    kCFRunLoopRunStopped = 2,
    kCFRunLoopRunTimedOut = 3, 
    kCFRunLoopRunHandledSource = 4 
};

void CFRunLoopRun(void) {    /* DOES CALLOUT */
    int32_t result;
    do {
    
        result = CFRunLoopRunSpecific(CFRunLoopGetCurrent(), kCFRunLoopDefaultMode, 1.0e10, false);
        
        CHECK_FOR_FORK();
    } while (kCFRunLoopRunStopped != result && kCFRunLoopRunFinished != result);
}
```
&emsp;看到 `CFRunLoopRun` 函数是内部是一个 do while 循环，内部调用了 `CFRunLoopRunSpecific` 函数根据其返回值判断是否结束循环。
### CFRunLoopRunSpecific
&emsp;
```c++

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


