# iOS 从源码解析Run Loop (五)：run loop 运行全过程解析

> &emsp;前面几篇算是把 run loop 相关的数据结构都看完了，也算是把 run loop 开启运行的前期数据都准备好了，下面我们开始正式进入 run loop 的整个的运行过程的探索和学习。⛽️⛽️

&emsp;查看 CFRunLoop.h 文件，看到涉及 run loop 运行的函数有两个 `CFRunLoopRun` 和 `CFRunLoopRunInMode` 下面我们跟着源码学习一下这两个函数。
## CFRunLoopRun/CFRunLoopRunInMode
&emsp;`CFRunLoopRun` 函数同 NSRunLoop 的 `- (void)run;` 函数，无限期地以其默认模式运行当前线程的 CFRunLoop 对象。当前线程的运行循环将以默认模式运行，直到使用 `CFRunLoopStop` 停止 run loop 或将所有 Sources 和 Timers 从默认运行循环模式中移除为止。run loop 可以递归运行，你可以从任何 run loop 调用中调用 `CFRunLoopRun` 函数，并在当前线程的调用堆栈上创建嵌套的 run loop 激活。

&emsp;`CFRunLoopRunInMode` 在特定模式下运行当前线程的 CFRunLoop 对象。
+ `mode`：以运行循环模式运行。模式可以是任意 CFString。尽管运行循环模式需要至少包含一个源或计时器才能运行，但是你无需显式创建运行循环模式。
+ `seconds`：运行 run loop 的时间长度。如果为 0，则返回之前仅运行循环一次；如果有多个源或计时器准备立即触发，那么无论 `returnAfterSourceHandled` 的值如何，都将仅触发一个（如果一个是 version 0 source，则可能触发两个）。
+ `returnAfterSourceHandled`：一个标志，指示 run loop 是否应在处理一个源之后退出。如果为 false，则运行循环将继续处理事件，直到经过 `seconds`。

&emsp;`CFRunLoopRunInMode` 函数返回一个值，指示 run loop 退出的原因。

&emsp;Run loops 可以递归运行。你可以从任何运行循环调用中调用 `CFRunLoopRunInMode` 函数，并在当前线程的调用堆栈上创建嵌套的运行循环激活。在调用中可以运行的模式不受限制。你可以创建另一个在任何可用的运行循环模式下运行的运行循环激活，包括调用堆栈中已经运行的任何模式。

&emsp;在指定的条件下，运行循环退出并返回以下值:
+ `kCFRunLoopRunFinished` 运行循环模式没有源或计时器。
+ `kCFRunLoopRunStopped` 运行循环已使用 `CFRunLoopStop` 函数停止。
+ `kCFRunLoopRunTimedOut` 时间间隔秒数（seconds）过去了。
+ `kCFRunLoopRunHandledSource` 已处理源。此退出条件仅适用于 `returnAfterSourceHandled` 为 `true` 时。

&emsp;不能为 `mode` 参数指定 `kCFRunLoopCommonModes` 常量。运行循环总是以特定模式运行。只有在配置运行循环观察者时，以及仅在希望该观察者以多种模式运行的情况下，才能指定 common mode。

&emsp;下面是 `CFRunLoopRun` 和 `CFRunLoopRunInMode` 函数的定义:
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
        
        // 以 kCFRunLoopDefaultMode 启动当前线程的 run loop
        result = CFRunLoopRunSpecific(CFRunLoopGetCurrent(), kCFRunLoopDefaultMode, 1.0e10, false);
        
        CHECK_FOR_FORK();
    } while (kCFRunLoopRunStopped != result && kCFRunLoopRunFinished != result);
}

SInt32 CFRunLoopRunInMode(CFStringRef modeName,
                          CFTimeInterval seconds,
                          Boolean returnAfterSourceHandled) { 
    CHECK_FOR_FORK();
    
    // 以指定的 run loop mode 启动当前线程的 run loop，且可以自定义 seconds 和 returnAfterSourceHandled 参数的值
    return CFRunLoopRunSpecific(CFRunLoopGetCurrent(), modeName, seconds, returnAfterSourceHandled);
}

```
&emsp;看到 `CFRunLoopRun` 函数是内部是一个 do while 循环，内部调用了 `CFRunLoopRunSpecific` 函数当其返回值是 `kCFRunLoopRunTimedOut` 或 `kCFRunLoopRunHandledSource` 时一直持续进行 do while 循环。（根据之前的文章记得只有当前 run loop mode 没有 source/timer/observe 时当前线程的 NSRunLoop 对象调用 `-(void)run;` 函数，run loop 会启动失败，其他情况就是一直无限循环，所以想这里的 do while 结束循环的条件不是应该只有 `kCFRunLoopRunFinished != result` 吗，即使是调用了 `CFRunLoopStop` 函数，结束的也只是本次 run loop 并不会导致 do while 退出...但是现在则是多了 `kCFRunLoopRunStopped != result`）

&emsp;看到 `CFRunLoopRun` 和 `CFRunLoopRunInMode` 函数内部都是调用了 `CFRunLoopRunSpecific` 函数，第一个参数都是直接使用 `CFRunLoopGetCurrent` 函数获取当前线程的 run loop，然后是第二个参数 `CFStringRef modeName` 则是传入 run loop mode 的名字，而非直接传入 CFRunLoopMode 实例，第三个参数则是 `CFTimeInterval seconds` 指示 run loop 需要运行多久。

### CFRunLoopRunSpecific
&emsp;`CFRunLoopRunSpecific` 函数
```c++
SInt32 CFRunLoopRunSpecific(CFRunLoopRef rl, CFStringRef modeName, CFTimeInterval seconds, Boolean returnAfterSourceHandled) {     /* DOES CALLOUT */
    CHECK_FOR_FORK();
    
    // 从 rl 的 _cfinfo 字段中取 rl 是否正在释放的标记值，如果是第话，则直接返回 kCFRunLoopRunFinished
    if (__CFRunLoopIsDeallocating(rl)) return kCFRunLoopRunFinished;
    
    // CFRunLoop 加锁
    __CFRunLoopLock(rl);
    
    // 调用 __CFRunLoopFindMode 函数从 rl 的 _modes 中找到名字是 modeName 的 run loop mode，
    // 如果找不到的话第三个参数传的是 false，则直接返回 NULL。 
    //（CFRunLoopMode 加锁）
    CFRunLoopModeRef currentMode = __CFRunLoopFindMode(rl, modeName, false);
    
    // 如果 currentMode 为 NULL 或者 currentMode 里面是空的不包含 source/timer 则 return 
    if (NULL == currentMode || __CFRunLoopModeIsEmpty(rl, currentMode, rl->_currentMode)) {
        Boolean did = false;
        
        // 如果 currentMode 存在，则进行 CFRunLoopMode 解锁，对应了上面 __CFRunLoopFindMode(rl, modeName, false) 调用内部的 CFRunLoopMode 加锁 
        if (currentMode) __CFRunLoopModeUnlock(currentMode);
        
        // CFRunLoop 解锁
        __CFRunLoopUnlock(rl);
        
        return did ? kCFRunLoopRunHandledSource : kCFRunLoopRunFinished;
    }
    
    // __CFRunLoopPushPerRunData 函数内部是修改 rl 的 _perRunData 字段的各成员变量的值，并返回之前的 _perRunData 
    volatile _per_run_data *previousPerRun = __CFRunLoopPushPerRunData(rl);
    
    // previousMode 记录 rl 当前的 run loop mode，相比入参传入的 modeName 取得的 run loop mode 而言，它是之前的 run loop mode 
    CFRunLoopModeRef previousMode = rl->_currentMode;
    
    // 更新 rl 的 _currentMode 为入参 modeName 对应的 run loop mode 
    rl->_currentMode = currentMode;
    
    // 临时变量 result
    int32_t result = kCFRunLoopRunFinished;
    
    // 判断如果 currentMode 的 _observerMask 字段中包含 kCFRunLoopEntry 的值（_observerMask 内记录了需要观察 run loop 哪些状态变化），
    // 则告诉 currentMode 的 run loop observer 发生了一个 run loop 即将进入循环的状态变化。 
    if (currentMode->_observerMask & kCFRunLoopEntry ) __CFRunLoopDoObservers(rl, currentMode, kCFRunLoopEntry);
    
    // 启动 run loop，__CFRunLoopRun 函数超长，可能是看源码以来最长的一个函数，下面会逐行进行细致的分析
    result = __CFRunLoopRun(rl, currentMode, seconds, returnAfterSourceHandled, previousMode);
    
    // 同上的 kCFRunLoopEntry 进入循环的回调，这里则是退出 run loop 的回调。
    // 如果 currentMode 的 _observerMask 中包含 kCFRunLoopExit 的值，即 run loop observer 需要观察 run loop 的 kCFRunLoopExit 退出状态切换
    if (currentMode->_observerMask & kCFRunLoopExit ) __CFRunLoopDoObservers(rl, currentMode, kCFRunLoopExit);
    
    // CFRunLoopMode 解锁
    __CFRunLoopModeUnlock(currentMode);
    
    // 销毁 rl 当前的 _perRunData，并把 previousPerRun 重新赋值给 rl 的 _perRunData 
    __CFRunLoopPopPerRunData(rl, previousPerRun);
    
    // 回到之前的 _currentMode 
    rl->_currentMode = previousMode;
    
    // CFRunLoop 解锁
    __CFRunLoopUnlock(rl);
    
    // 返回 result 结果
    return result;
}
```
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


