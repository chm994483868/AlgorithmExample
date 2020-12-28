# iOS 从源码解析Run Loop (八)：Run Loop 应用场景总结

> &emsp;苹果使用 run loop 实现了：子线程保活、控制自动释放池的 push 和 pop、


延迟回调、触摸事件、屏幕刷新等功能。下面我们就对涉及到 run loop 的各个功能点进行详细的学习。⛽️⛽️

&emsp;我们首先再次回顾一下 Source/Timer/Observer。

1. CFRunLoopSourceRef 是事件产生的地方。Source 有两个版本：Source0 和 Source1。
+ Source0 只包含了一个回调（函数指针），它并不能主动触发事件。使用时，你需要先调用 CFRunLoopSourceSignal(source)，将这个 Source 标记为待处理，然后手动调用 CFRunLoopWakeUp(runloop) 来唤醒 RunLoop，让其处理这个事件。
+ Source1 包含了一个 mach_port 和一个回调（函数指针），被用于通过内核和其他线程相互发送消息。这种 Source 能主动唤醒 RunLoop 的线程。

&emsp;从它们的数据结构中也能看出区别，CFRunLoopSourceContext 和 CFRunLoopSourceContext1 具有一些相同的部分和不同部分。
```c++
typedef struct {
    CFIndex version;
    void * info; // source 的信息
    const void *(*retain)(const void *info); // retain 函数
    void (*release)(const void *info); // release 函数
    CFStringRef (*copyDescription)(const void *info); // 返回描述字符串的函数
    Boolean (*equal)(const void *info1, const void *info2); // 判断 source 对象是否相等的函数
    CFHashCode (*hash)(const void *info); // 哈希函数
} CFRunLoopSourceContext/1;
```
&emsp;version、info、retain 函数、release 函数、描述字符串的函数、判断 source 对象是否相等的函数、哈希函数，是 CFRunLoopSourceContext 和 CFRunLoopSourceContext1 的基础内容双方完成等同，两者的区别主要在下面，同时它们也表示了 source0 和 source1 的不同功能。
```c++
typedef struct {
    ...
    void (*schedule)(void *info, CFRunLoopRef rl, CFStringRef mode); // 当 source0 加入到 run loop 时触发的回调函数（在 CFRunLoopAddSource 函数中可看到其被调用）
    void (*cancel)(void *info, CFRunLoopRef rl, CFStringRef mode); // 当 source0 从 run loop 中移除时触发的回调函数
    void (*perform)(void *info); // source0 要执行的任务块，当 source0 事件被触发时的回调, 使用 CFRunLoopSourceSignal 函数触发
} CFRunLoopSourceContext;
```
```c++
typedef struct {
    ...
#if (TARGET_OS_MAC && !(TARGET_OS_EMBEDDED || TARGET_OS_IPHONE)) || (TARGET_OS_EMBEDDED || TARGET_OS_IPHONE)
    mach_port_t (*getPort)(void *info); // getPort 函数指针，用于当 source1 被添加到 run loop 中的时候，从该函数中获取具体的 mach_port_t 对象，用来唤醒 run loop。
    void * (*perform)(void *msg, CFIndex size, CFAllocatorRef allocator, void *info); // perform 函数指针即指向 run loop 被唤醒后 source1 要执行的回调函数
#else
    // 其它平台
    void * (*getPort)(void *info);
    void (*perform)(void *info);
#endif
} CFRunLoopSourceContext1;
```
&emsp;可看到 Source0 中仅有一些回调函数，而 Source1 中有 mach port 可用来唤醒 run loop。

2. CFRunLoopTimerRef 是基于时间的触发器，它和 NSTimer 是 toll-free bridged 的，可以混用。其包含一个时间长度和一个回调（函数指针）。当其加入到 run loop 时，run loop 会注册对应的时间点，当时间点到时，run loop会被唤醒以执行那个回调。
3. CFRunLoopObserverRef 是观察者，每个 Observer 都包含了一个回调（函数指针），当 run loop 的状态发生变化时，观察者就能通过回调接受到这个变化。
## 观察 run loop 状态变化
&emsp;下面是观察主线程 run loop 的状态变化的示例代码:
```c++
- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    
    // 只给 void * info 字段传入了初始值，它会作为下面 mainRunLoopActivitie 回调函数的 info 参数
    CFRunLoopObserverContext context = {0, (__bridge void *)(self), NULL, NULL, NULL};
    
    // kCFRunLoopAllActivities 参数表示观察 run loop 的所有状态变化 
    // YES 表示重复观察 run lop 的状态变化
    // mainRunLoopActivitie 对应于 __CFRunLoopObserver 结构体的 _callout 字段，是 run loop 状态变化时的回调函数
    // 0 是对应 __CFRunLoopObserver 的 _order 字段，当一个 run loop 添加的了多个 run loop observer 时，_order 会作为它们的调用顺序的依据，_order 值越小优先级越高，
    // context 是上下文，这里主要用来传递 info 了。
    CFRunLoopObserverRef observer = CFRunLoopObserverCreate(kCFAllocatorDefault, kCFRunLoopAllActivities, YES, 0, mainRunLoopActivitie, &context);
    if (observer) {
        // 把 observer 添加到 main run loop 的 kCFRunLoopCommonModes 模式下
        CFRunLoopAddObserver(CFRunLoopGetMain(), observer, kCFRunLoopCommonModes);
    }
}

void mainRunLoopActivitie(CFRunLoopObserverRef observer, CFRunLoopActivity activity, void *info) {
    // observer：上面 viewDidLoad 函数中添加到 main run loop 的 CFRunLoopObserverRef 实例
    // activity：本次的状态变化：kCFRunLoopEntry、kCFRunLoopBeforeTimers、kCFRunLoopBeforeSources、kCFRunLoopBeforeWaiting、kCFRunLoopAfterWaiting、kCFRunLoopExit、（kCFRunLoopAllActivities）
    // info： 上面 viewDidLoad 函数中 CFRunLoopObserverContext 实例的 info 成员变量，上面是 (__bridge void *)(self)
}
```
&emsp;当 main run loop 的状态发生变化时会调用 mainRunLoopActivitie 函数，我们可以在其中根据 activity 做想要的处理。具体详细的 CFRunLoopObserverCreate 和 CFRunLoopAddObserver 函数的实现在前面都已经分析过，可以参考前面 [iOS 从源码解析Run Loop (四)：Source、Timer、Observer 创建以及添加到 mode 的过程](https://juejin.cn/post/6908639874857828366)
## 线程保活
&emsp;如果想让子线程永久保持活性那么就在子线程内调用其 run loop 实例的 run 函数，如果想自由控制线程 run loop 结束时机的话则使用一个变量控制 do while 循环，在循环内部调用子线程的 run loop 实例的 runMode: beforeDate: 函数，当需要停止子线程的 run loop 时则在子线程内调用 CFRunLoopStop(CFRunLoopGetCurrent()); 并结束 do while 循环，详细内容可参考前面 [iOS 从源码解析Run Loop (一)：run loop 基本概念理解与 NSRunLoop 文档](https://juejin.cn/post/6904921175546298375)
## 控制自动释放池的 push 和 pop
&emsp;自动释放池什么时候执行 pop 操作把池中的对象的都执行一次 release  呢？这里要分两种情况：
+ 一种是我们手动以 `@autoreleasepool {...}`  的形式添加的自动释放池，使用 clang -rewrite-objc 转换为 C++ 后其实是
```c++
struct __AtAutoreleasePool {
  __AtAutoreleasePool() {atautoreleasepoolobj = objc_autoreleasePoolPush();}
  ~__AtAutoreleasePool() {objc_autoreleasePoolPop(atautoreleasepoolobj);}
  void * atautoreleasepoolobj;
};

/* @autoreleasepool */ 
{ 
    // 直接构建了一个 __AtAutoreleasePool 实例，
    // 构造函数调用了 AutoreleasePoolPage 的 push 函数，构建了一个自动释放池。
    __AtAutoreleasePool __autoreleasepool;
    // ...
}
```
&emsp;可看到 `__autoreleasepool` 是被包裹在对 `{}` 之中的，当出了右边花括号时自动释放池便会执行 pop 操作，也可理解为如下代码:
```c++
void *pool = objc_autoreleasePoolPush();
// {}中的代码
objc_autoreleasePoolPop(pool);
```
&emsp;在原始 main 函数中，打一个断点，并开启 Debug Workflow 的 Always Show Disassembly 可看到对应的汇编代码：
```c++
int main(int argc, char * argv[]) {
    NSString * appDelegateClassName;
    @autoreleasepool {
        // Setup code that might create autoreleased objects goes here.
        // appDelegateClassName = NSStringFromClass([AppDelegate class]);
    } // ⬅️ 在这里打一个断点
    return UIApplicationMain(argc, argv, nil, appDelegateClassName);
}
```
&emsp;由于上面代码中自动释放池什么也没有放，Push 完便接着 Pop 了。
```c++
...
0x101319b78 <+32>:  bl     0x101319eb8               ; symbol stub for: objc_autoreleasePoolPush
0x101319b7c <+36>:  bl     0x101319eac               ; symbol stub for: objc_autoreleasePoolPop
...
```
+ 一种是由 run loop 创建的自动释放池。ibireme 大佬如是说:
> &emsp;App 启动后，苹果在主线程 RunLoop 里注册了两个 Observer，其回调都是 _wrapRunLoopWithAutoreleasePoolHandler()。
> &emsp;第一个 Observer 监视的事件是 Entry(即将进入Loop)，其回调内会调用 _objc_autoreleasePoolPush() 创建自动释放池。其 order 是-2147483647，优先级最高，保证创建释放池发生在其他所有回调之前。
> &emsp;第二个 Observer 监视了两个事件： BeforeWaiting(准备进入休眠) 时调用_objc_autoreleasePoolPop() 和 _objc_autoreleasePoolPush() 释放旧的池并创建新池；Exit(即将退出Loop) 时调用 _objc_autoreleasePoolPop() 来释放自动释放池。这个 Observer 的 order 是 2147483647，优先级最低，保证其释放池子发生在其他所有回调之后。
> &emsp;在主线程执行的代码，通常是写在诸如事件回调、Timer回调内的。这些回调会被 RunLoop 创建好的 AutoreleasePool 环绕着，所以不会出现内存泄漏，开发者也不必显示创建 Pool 了。[深入理解RunLoop](https://blog.ibireme.com/2015/05/18/runloop/)
> &emsp;关于自动释放池的知识点可以参考前面的文章: [iOS 从源码解析Runtime (六)：AutoreleasePool实现原理解读](https://juejin.cn/post/6877085831647985677)

&emsp;下面我们试着验证一下上面的结论，在 application:didFinishLaunchingWithOptions: 函数中添加一个断点，在控制台打印 po [NSRunLoop mainRunLoop]，可看到当前 main run loop 在 kCFRunLoopDefaultMode 模式下运行，然后在 kCFRunLoopDefaultMode 模式有 6 个 observers，这里我们只看其中大佬提到的最高优先级和最低优先级的 CFRunLoopObserver:
```c++
    observers = (
    "<CFRunLoopObserver 0x282638640 [0x20e729430]>{valid = Yes, activities = 0x1, repeats = Yes, order = -2147483647, callout = <redacted> (0x20af662ec), context = <CFArray 0x28197def0 [0x20e729430]>{type = mutable-small, count = 1, values = (\n\t0 : <0x1006ec048>\n)}}",
    ...
    "<CFRunLoopObserver 0x2826385a0 [0x20e729430]>{valid = Yes, activities = 0xa0, repeats = Yes, order = 2147483647, callout = <redacted> (0x20af662ec), context = <CFArray 0x28197def0 [0x20e729430]>{type = mutable-small, count = 1, values = (\n\t0 : <0x1006ec048>\n)}}"
)
```
&emsp;order 是 -2147483647 的 CFRunLoopObserver 优先级最高，会在其它所有 CFRunLoopObserver 之前回调，然后它的 activities 是 0x1，对应 kCFRunLoopEntry = (1UL << 0)，即只观察 kCFRunLoopEntry 状态，回调函数的话只能看到地址 callout = <redacted> (0x20af662ec)，添加一个 `_wrapRunLoopWithAutoreleasePoolHandler` 符号断点，添加一个 `objc_autoreleasePoolPush` 符号断点，运行程序，并在控制台 bt 打印函数堆栈，确实能看到如下的函数调用：
```c++
(lldb) bt
* thread #1, queue = 'com.apple.main-thread', stop reason = breakpoint 3.1
  * frame #0: 0x00000001dd971864 libobjc.A.dylib`objc_autoreleasePoolPush // push 构建自动释放池
    frame #1: 0x00000001de78d61c CoreFoundation`_CFAutoreleasePoolPush + 16
    frame #2: 0x000000020af66324 UIKitCore`_wrapRunLoopWithAutoreleasePoolHandler + 56
    frame #3: 0x00000001de7104fc CoreFoundation`__CFRUNLOOP_IS_CALLING_OUT_TO_AN_OBSERVER_CALLBACK_FUNCTION__ + 32 // 执行 run loop observer 回调函数，
    frame #4: 0x00000001de70b224 CoreFoundation`__CFRunLoopDoObservers + 412
    frame #5: 0x00000001de70af9c CoreFoundation`CFRunLoopRunSpecific + 412
    frame #6: 0x00000001e090c79c GraphicsServices`GSEventRunModal + 104
    frame #7: 0x000000020af6cc38 UIKitCore`UIApplicationMain + 212
    frame #8: 0x0000000100a75b90 Simple_iOS`main(argc=1, argv=0x000000016f38f8e8) at main.m:77:12
    frame #9: 0x00000001de1ce8e0 libdyld.dylib`start + 4
(lldb) 
```
&emsp;在主线程中确实看到了 `__CFRUNLOOP_IS_CALLING_OUT_TO_AN_OBSERVER_CALLBACK_FUNCTION__` 执行 CFRunLoopObserver 的回调函数调用了 `_wrapRunLoopWithAutoreleasePoolHandler` 函数接着调用了 `objc_autoreleasePoolPush` 创建自动释放池。

&emsp;order 是 2147483647 的 CFRunLoopObserver 优先级最低，会在其它所有 CFRunLoopObserver 之后回调，然后它的 activities 是 0xa0（0b10100000），对应 kCFRunLoopBeforeWaiting = (1UL << 5) 和 kCFRunLoopExit = (1UL << 7)，即观察 run loop 的即将进入休眠和 run loop 退出的两个状态变化，回调函数的话只能看到地址 callout = <redacted> (0x20af662ec)，我们再添加一个 `objc_autoreleasePoolPop` 符号断点，此时需要我们添加一些测试代码，我们添加一个 main run loop 的观察者，然后再添加一个主线程的 main run loop 的计时器，程序启动后我们可看到控制台如下循环打印:
```c++
 🎯... kCFRunLoopAfterWaiting
 ⏰⏰⏰ timer 回调...
 🎯... kCFRunLoopBeforeTimers
 🎯... kCFRunLoopBeforeSources
 🎯... kCFRunLoopBeforeWaiting
 🎯... kCFRunLoopAfterWaiting
 ⏰⏰⏰ timer 回调...
```
&emsp;主线程进入了一种 “休眠--被 timer 唤醒执行回调--休眠” 的循环之中，此时我们打开 `_wrapRunLoopWithAutoreleasePoolHandler` 断点发现程序进入，然后再打开 `objc_autoreleasePoolPop` 断点，然后点击 Continue program execution 按钮，此时会进入 `objc_autoreleasePoolPop` 断点，在控制台 bt 打印函数调用栈：
```c++
(lldb) bt
* thread #1, queue = 'com.apple.main-thread', stop reason = breakpoint 1.1
  * frame #0: 0x00000001dd9718f8 libobjc.A.dylib`objc_autoreleasePoolPop
    frame #1: 0x00000001de78cba0 CoreFoundation`_CFAutoreleasePoolPop + 28
    frame #2: 0x000000020af66360 UIKitCore`_wrapRunLoopWithAutoreleasePoolHandler + 116
    frame #3: 0x00000001de7104fc CoreFoundation`__CFRUNLOOP_IS_CALLING_OUT_TO_AN_OBSERVER_CALLBACK_FUNCTION__ + 32
    frame #4: 0x00000001de70b224 CoreFoundation`__CFRunLoopDoObservers + 412
    frame #5: 0x00000001de70b7a0 CoreFoundation`__CFRunLoopRun + 1228
    frame #6: 0x00000001de70afb4 CoreFoundation`CFRunLoopRunSpecific + 436
    frame #7: 0x00000001e090c79c GraphicsServices`GSEventRunModal + 104
    frame #8: 0x000000020af6cc38 UIKitCore`UIApplicationMain + 212
    frame #9: 0x0000000100bc9b2c Simple_iOS`main(argc=1, argv=0x000000016f23b8e8) at main.m:76:12
    frame #10: 0x00000001de1ce8e0 libdyld.dylib`start + 4
(lldb)
```
&emsp;确实看到了 `_wrapRunLoopWithAutoreleasePoolHandler` 调用了 `objc_autoreleasePoolPop`。

&emsp;这样整体下来：Entry-->push ---> BeforeWaiting--->pop-->push -->Exit-->pop，按照这样的顺序，保证了，每一次 push 都对应一个 pop。

&emsp;从上面 run loop observer 工作便知，每一次 loop，便会有一次 pop 和 push，因此我们得出：
1. 如果手动添加 autoreleasePool，autoreleasePool 作用域里的自动释放对象会在出 pool 作用域的那一刻释放。
2. 如果非手动添加的，那么自动释放的对象会在每一次 run loop 循环结束时，释放当前这次循环所创建的自动释放对象。
## NSTimer 实现过程
&emsp;涉及到 UITrackingRunLoopMode 和 kCFRunLoopDefaultMode 的切换。





## 监控主线程卡顿
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
