# iOS 从源码解析Run Loop (九)：Run Loop 与GCD、FPS

> &emsp;本篇我们继续学习日常开发中可能被我们忽略但是内部实现其实涉及到 Run Loop 做支撑的一些知识点。

## GCD
&emsp;在 Run Loop 和 GCD 的底层双方各自都会相互用到对方。首先我们先看一下读 run loop 源码的过程中用到 GCD 的地方，前面我们学习 GCD 的时候已知使用 `dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue)` 可以构建计时器，且它比 NSTimer 的精度更高。

&emsp;在 run loop 中有两个地方用到了 dispatch_source，一是在 run loop mode 中有一个与 \_timerPort（mk_timer_create）对应的 \_timerSource，它们两个的作用是相同的，都是用来当到达 run loop mode 的 \_timers 数组中最近的某个计时器的触发时间时用来唤醒当前 run loop 的，然后还有一个地方是在 \__CFRunLoopRun 函数中直接使用 dispatch_source 构建一个计时器用来为入参 run loop 的运行时间计时的，当入参 run loop 运行超时时此计时器便会触发。

&emsp;在 run loop mode 中使用 dispatch_source 还是 MK_TIMER 来构建一个计时器是有一个平台限制的，源码内部使用了两个宏做区分 USE_DISPATCH_SOURCE_FOR_TIMERS 和 USE_DISPATCH_SOURCE_FOR_TIMERS。
```c++
#if DEPLOYMENT_TARGET_MACOSX

// 在 macOS 下则同时支持使用 dispatch_source 和 MK_TIMER 来构建定时器
#define USE_DISPATCH_SOURCE_FOR_TIMERS 1
#define USE_MK_TIMER_TOO 1

#else

// 其他平台则只支持 MK_TIMER 
#define USE_DISPATCH_SOURCE_FOR_TIMERS 0
#define USE_MK_TIMER_TOO 1

#endif
```
&emsp;这里我们可以全局搜索 USE_DISPATCH_SOURCE_FOR_TIMERS 然后看到它和 USE_MK_TIMER_TOO 几乎都是前后🦶使用的，且都是为了同一个目标为 CFRunLoopTimerRef 而唤醒 run loop。

&emsp;注意这里并不是说 macOS 之外的平台就不支持使用 dispatch_source 了，大家（iOS、macOS）都支持，这里只是针对的是 run loop mode 中用来为 \_timers 数组中的某个 CFRunLoopTimerRef 到达触发时间时唤醒当前 run loop 的方式不同而已。然后在 \__CFRunLoopRun 函数中我们看到所有平台下都是使用 dispatch_source 来构建计时器为 run loop 的运行时间而计时的。

&emsp;（一个题外话：看到这里我们似乎可以得到一些理解和启发，CFRunLoopTimerRef 虽一直被我们称为计时器，但其实它的触发执行是完全依赖 run loop mode 中的 \_timerPort 或者 \_timerSource 来唤醒当前 run loop，然后在当前 run loop 的本次循环中判断本次 run loop 被唤醒的来源，如果是因为 timer ，则执行某个 CFRunLoopTimerRef 的回调事件并更新最近的下次执行时间，所以这里 CFRunLoopTimerRef 虽被称为计时器其实它的计时部分是依靠别人来做的，它本身并不具备计时功能，只是有一个值记录自己的下次触发时间而已。）

&emsp;下面我们看一下 GCD 中使用到 Run Loop 的地方。

&emsp;当调用 dispatch_async(dispatch_get_main_queue(), block) 时，libDispatch 会向主线程的 run loop 发送消息，run loop 会被唤醒，并从消息中取得这个 block，并在回调 \_\_CFRUNLOOP_IS_SERVICING_THE_MAIN_DISPATCH_QUEUE\_\_ 里执行这个 block。但这个逻辑仅限于 dispatch 到主线程，dispatch 到其他线程仍然是由 libDispatch 处理的。

&emsp;上面一段结论我们在梳理 \__CFRunLoopRun 函数流程时已经看的一清二楚了。如函数开始时判断当前是否是主线程来获取主队列的 port 并赋值给 dispatchPort，然后在 run loop 本次循环中判断唤醒来源是 dispatchPort 时，执行添加到主队列中的任务（_dispatch_main_queue_drain）。
```c++
...
        else if (livePort == dispatchPort) {
            CFRUNLOOP_WAKEUP_FOR_DISPATCH();
            __CFRunLoopModeUnlock(rlm);
            __CFRunLoopUnlock(rl);
            
            // TSD 给 __CFTSDKeyIsInGCDMainQ 置为 6 和 下面的置 0 对应，可以理解为一个加锁行为!
            _CFSetTSD(__CFTSDKeyIsInGCDMainQ, (void *)6, NULL);
            
#if DEPLOYMENT_TARGET_WINDOWS
            void *msg = 0;
#endif

            // 内部是调用 static void _dispatch_main_queue_drain(dispatch_queue_main_t dq) 函数，即处理主队列中的任务
            __CFRUNLOOP_IS_SERVICING_THE_MAIN_DISPATCH_QUEUE__(msg);
            
            _CFSetTSD(__CFTSDKeyIsInGCDMainQ, (void *)0, NULL);
            
            __CFRunLoopLock(rl);
            __CFRunLoopModeLock(rlm);
            
            sourceHandledThisLoop = true;
            didDispatchPortLastTime = true;
        }
...
```
&emsp;到这里 GCD 和 Run Loop 的相互使用就看完了，下面我们看一下屏幕 FPS 相关的内容。
## FPS
&emsp;
```c++

```
### CADisplayLink
&emsp;CADisplayLink 是一个和屏幕刷新率一致的定时器（但实际实现原理更复杂，和 NSTimer 并不一样，其内部实际是操作了一个 Source）。如果在两次屏幕刷新之间执行了一个长任务，那其中就会有一帧被跳过去（和 NSTimer 相似），造成界面卡顿的感觉。在快速滑动 TableView 时，即使一帧的卡顿也会让用户有所察觉。Facebook 开源的 AsyncDisplayLink 就是为了解决界面卡顿的问题，其内部也用到了 Run Loop。下面我们首先看一下 CADisplayLink 的文档。

> &emsp;A timer object that allows your application to synchronize its drawing to the refresh rate of the display.

&emsp;CADisplayLink 表示一个绑定到显示 vsync 的计时器的类。（其中 CA 表示的是 Core Animation（核心动画） 首字母缩写，CoreAnimation.h 是 QuartzCore 框架中的一个包含 QuartzCore 框架所有头文件的文件）
```c++
/** Class representing a timer bound to the display vsync. **/

API_AVAILABLE(ios(3.1), watchos(2.0), tvos(9.0)) API_UNAVAILABLE(macos)
@interface CADisplayLink : NSObject {
@private
  void *_impl;
}
```
&emsp;在应用程序中初始化一个新的 display link 对象时使用 displayLinkWithTarget:selector: 函数，此函数提供了一个 target 对象和一个在屏幕更新时要调用的 selector。为了使 display loop 与 display 同步，需要使用 addToRunLoop:forMode: 函数将 display link 对象添加到指定 run loop 的指定 mode 下。

&emsp;一旦 display link 与 run loop 相关联，当需要更新屏幕内容时，就会调用 target 上的 selector。target 可以读取 display link 的 timestamp 属性，以检索上一帧显示的时间。例如，播放电影的应用程序可能使用 timestamp 来计算下一个要显示的视频帧。执行自己的动画的应用程序可能会使用 timestamp 来确定在下一帧中显示对象的位置和方式。

&emsp;duration 属性以 maximumFramesPerSecond（屏幕每秒可显示的最大帧数：60）提供帧之间的时间量。要计算实际的帧时长（frame duration），请使用 targetTimestamp - timestamp。你可以在应用程序中使用实际的 frame duration 来计算显示器的帧率、下一帧的大概显示时间、并调整绘图行为（drawing behavior），以便及时准备下一帧以供显示。

&emsp;应用程序可以通过将 paused 属性设置为 YES 来禁用通知。另外，如果你的应用程序无法在提供的时间内提供帧，你可能需要选择较慢的帧速率。对于用户来说，帧速率较慢但一致的应用程序比跳过帧的应用程序更平滑。通过设置 preferredFramesPerSecond 属性，可以定义每秒帧数。

&emsp;当你的应用程序完成 display link 时，应调用 invalidate 将其从所有 run loop 中删除，并将其与目标解除关联。

&emsp;CADisplayLink 不应被子类化。
#### displayLinkWithTarget:selector:
&emsp;返回一个新建的 display link 对象。
```c++
// 为 main display 创建一个新的 display link 对象。它将在 'target' 上调用名为 'sel' 的方法，该方法具有 '(void)selector:(CADisplayLink *)sender' 的签名。
+ (CADisplayLink *)displayLinkWithTarget:(id)target selector:(SEL)sel;
```
&emsp;`target`：当屏幕应该更新时要通知的对象。`sel`：在 `target` 上调用的方法。

&emsp;要在 `target` 上调用的 selector 必须是具有以下签名的方法：
```c++
- (void) selector:(CADisplayLink *)sender;
```
&emsp;其中 sender 是 displayLinkWithTarget:selector: 返回的 display link 对象。

&emsp;新建的 display link 对象 retain 了 `target`。
#### addToRunLoop:forMode:
&emsp;
```c++
/* Adds the receiver to the given run-loop and mode. Unless paused, it will fire every vsync until removed. Each object may only be added to a single run-loop, but it may be added in multiple modes at once. While added to a run-loop it will implicitly be retained. */

- (void)addToRunLoop:(NSRunLoop *)runloop forMode:(NSRunLoopMode)mode;
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
+ [iOS底层原理 RunLoop 基础总结和随心所欲掌握子线程 RunLoop 生命周期 --(9)](http://www.cocoachina.com/articles/28800)
+ [从NSRunLoop说起](https://zhuanlan.zhihu.com/p/63184073)
+ [runloop 与autorelase对象、Autorelease Pool 在什么时候释放](https://blog.csdn.net/leikezhu1981/article/details/51246684)
+ [内存管理：autoreleasepool与runloop](https://www.jianshu.com/p/d769c1653347)
+ [Objective-C的AutoreleasePool与Runloop的关联](https://blog.csdn.net/zyx196/article/details/50824564)
+ [iOS开发-Runloop中自定义输入源Source](https://blog.csdn.net/shengpeng3344/article/details/104518051)
+ [IOHIDFamily](http://iphonedevwiki.net/index.php/IOHIDFamily)
+ [iOS卡顿监测方案总结](https://juejin.cn/post/6844903944867545096)
+ [iOS 保持界面流畅的技巧](https://blog.ibireme.com/2015/11/12/smooth_user_interfaces_for_ios/)

