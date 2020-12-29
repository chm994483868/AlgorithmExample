# iOS 从源码解析Run Loop (八)：Run Loop 应用场景总结

> &emsp;本篇学习我们日常开发中涉及到 run loop 的一些知识点，我们使用它们的时候可能不会想到这些知识点的背后其实都是 run loop 在做支撑的。


**子线程保活、在 run loop 循环过程中执行自动释放池的 push 和 pop、延迟回调、触摸事件、屏幕刷新等功能。下面我们就对涉及到 run loop 的各个功能点进行详细的学习。**


## 回顾 run loop mode item
&emsp;我们首先再次回顾一下 Source/Timer/Observer，因为 run loop 正是通过这些 run loop mode item 来向外提供功能支持的。

1. CFRunLoopSourceRef 是事件产生的地方。Source 有两个版本：Source0 和 Source1。
+ Source0 只包含了一个回调（函数指针），它并不能主动触发事件。使用时，你需要先调用 CFRunLoopSourceSignal(source)，将这个 Source 标记为待处理，然后手动调用 CFRunLoopWakeUp(runloop) 来唤醒 RunLoop，让其处理这个事件。
+ Source1 包含了一个 mach_port 和一个回调（函数指针），被用于通过内核和其他线程相互发送消息（mach_msg），这种 Source 能主动唤醒 RunLoop 的线程。

&emsp;下面看一下它们相关的数据结构，CFRunLoopSourceContext 和 CFRunLoopSourceContext1 具有一些相同的字段和不同字段。
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
&emsp;version、info、retain 函数、release 函数、描述字符串的函数、判断 source 对象是否相等的函数、哈希函数，是 CFRunLoopSourceContext 和 CFRunLoopSourceContext1 的基础内容双方完成等同，两者的区别主要在下面，它们表示了 source0 和 source1 的不同功能。
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
&emsp;可看到 Source0 中仅有一些回调函数会在 run loop 的本次循环中执行，而 Source1 中有 mach port 可用来主动唤醒 run loop。

2. CFRunLoopTimerRef 是基于时间的触发器，它和 NSTimer 是 toll-free bridged 的，可以混用。其包含一个时间长度和一个回调（函数指针）。当其加入到 run loop 时，run loop 会注册对应的时间点，当时间点到时，run loop会被唤醒以执行那个回调。
3. CFRunLoopObserverRef 是观察者，每个 Observer 都包含了一个回调（函数指针），当 run loop 的状态发生变化时，观察者就能通过这个回调接收到。
## 观察 run loop 的状态变化/观察 run loop mode 的切换
&emsp;下面是观察主线程 run loop 的状态变化以及当前 run loop mode 切换（kCFRunLoopDefaultMode 和 UITrackingRunLoopMode 的切换）的部分示例代码，其中在 ViewController 上添加一个能滚动的 tableView 的代码可自行添加:
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
        CFRelease(observer);
    }
}

int count = 0; // 定义全局变量来计算一个 mode 中状态切换的统计数据
void mainRunLoopActivitie(CFRunLoopObserverRef observer, CFRunLoopActivity activity, void *info) {
    // observer：上面 viewDidLoad 函数中添加到 main run loop 的 CFRunLoopObserverRef 实例
    // activity：本次的状态变化：kCFRunLoopEntry、kCFRunLoopBeforeTimers、kCFRunLoopBeforeSources、kCFRunLoopBeforeWaiting、kCFRunLoopAfterWaiting、kCFRunLoopExit、（kCFRunLoopAllActivities）
    // info： 上面 viewDidLoad 函数中 CFRunLoopObserverContext 实例的 info 成员变量，上面是 (__bridge void *)(self)
    
    ++count;
    switch (activity) {
        case kCFRunLoopEntry:
            count = 0;
            NSLog(@"🤫 - %d kCFRunLoopEntry 即将进入: %@", count, CFRunLoopCopyCurrentMode(CFRunLoopGetCurrent()));
            break;
        case kCFRunLoopBeforeTimers:
            NSLog(@"🤫 - %d kCFRunLoopBeforeTimers 即将处理 timers", count);
            break;
        case kCFRunLoopBeforeSources:
            NSLog(@"🤫 - %d kCFRunLoopBeforeSources 即将处理 sources", count);
            break;
        case kCFRunLoopBeforeWaiting:
            count = 0; // 每次 run loop 即将进入休眠时，count 置为 0，可表示一轮 run loop 循环结束
            NSLog(@"🤫 - %d kCFRunLoopBeforeWaiting 即将进入休眠", count);
            break;
        case kCFRunLoopAfterWaiting:
            NSLog(@"🤫 - %d kCFRunLoopAfterWaiting 即将从休眠中醒来", count);
            break;
        case kCFRunLoopExit:
            count = 0;
            NSLog(@"🤫 - %d kCFRunLoopExit 即将退出: %@", count, CFRunLoopCopyCurrentMode(CFRunLoopGetCurrent()));
            break;
        case kCFRunLoopAllActivities:
            NSLog(@"🤫 kCFRunLoopAllActivities");
            break;
    }
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    NSLog(@"%s",__func__);
}

// 从 App 静止状态点击屏幕空白区域可看到如下打印:
 🤫 - 1 kCFRunLoopAfterWaiting 即将从休眠中醒来
 🤫 - 2 kCFRunLoopBeforeTimers 即将处理 timers
 🤫 - 3 kCFRunLoopBeforeSources 即将处理 sources
 🤫 - 0 kCFRunLoopBeforeWaiting 即将进入休眠 // run loop 1⃣️ 组循环结束
 
 🤫 - 1 kCFRunLoopAfterWaiting 即将从休眠中醒来
 🤫 - 2 kCFRunLoopBeforeTimers 即将处理 timers
 🤫 - 3 kCFRunLoopBeforeSources 即将处理 sources
 -[ViewController touchesBegan:withEvent:] // 由 App 静止状态点击屏幕开始，上面是固定的循环两次才进入 touche 事件
 🤫 - 4 kCFRunLoopBeforeTimers 即将处理 timers
 🤫 - 5 kCFRunLoopBeforeSources 即将处理 sources
 🤫 - 0 kCFRunLoopBeforeWaiting 即将进入休眠 // run loop 2⃣️ 组循环结束
 
 🤫 - 1 kCFRunLoopAfterWaiting 即将从休眠中醒来
 🤫 - 2 kCFRunLoopBeforeTimers 即将处理 timers
 🤫 - 3 kCFRunLoopBeforeSources 即将处理 sources
 🤫 - 4 kCFRunLoopBeforeTimers 即将处理 timers
 🤫 - 5 kCFRunLoopBeforeSources 即将处理 sources
 🤫 - 0 kCFRunLoopBeforeWaiting 即将进入休眠 // run loop 3⃣️ 组循环结束
 
 🤫 - 1 kCFRunLoopAfterWaiting 即将从休眠中醒来
 🤫 - 2 kCFRunLoopBeforeTimers 即将处理 timers
 🤫 - 3 kCFRunLoopBeforeSources 即将处理 sources
 🤫 - 4 kCFRunLoopBeforeTimers 即将处理 timers
 🤫 - 5 kCFRunLoopBeforeSources 即将处理 sources
 🤫 - 0 kCFRunLoopBeforeWaiting 即将进入休眠 // run loop 4⃣️ 组循环结束
 // 下面则是固定的循环两次后 App 进入静止状态。
 
 🤫 - 1 kCFRunLoopAfterWaiting 即将从休眠中醒来
 🤫 - 2 kCFRunLoopBeforeTimers 即将处理 timers
 🤫 - 3 kCFRunLoopBeforeSources 即将处理 sources
 🤫 - 0 kCFRunLoopBeforeWaiting 即将进入休眠 // run loop 5⃣️ 组循环结束
 
 🤫 - 1 kCFRunLoopAfterWaiting 即将从休眠中醒来
 🤫 - 2 kCFRunLoopBeforeTimers 即将处理 timers
 🤫 - 3 kCFRunLoopBeforeSources 即将处理 sources
 🤫 - 0 kCFRunLoopBeforeWaiting 即将进入休眠 // run loop 6⃣️ 组循环结束
 // 此后 run loop 进入长久休眠
```
&emsp;首先运行模式切换相关，当我们从静止状态滚动 tableView 的时候，会看到 `🤫 - 0 kCFRunLoopExit 即将退出: kCFRunLoopDefaultMode` 和 `🤫 - 0 kCFRunLoopEntry 即将进入: UITrackingRunLoopMode`，当滑动停止的时候又会看到 `🤫 - 0 kCFRunLoopExit 即将退出: UITrackingRunLoopMode` 和 `🤫 - 0 kCFRunLoopEntry 即将进入: kCFRunLoopDefaultMode`。即从 Default 退出进入 UITracking，然后滑动停止后是退出 UITracking 再进入 Default。

&emsp;状态切换的话是，从程序静止状态时，点击屏幕空白区域，则是固定的 `AfterWaiting -> BeforeTimers -> BeforeSources` 然后进入休眠 `BeforeWaiting`，然后是再来一次 `AfterWaiting -> BeforeTimers -> BeforeSources` 后才会执行 `touchesBegan:withEvent:` 回调，即 run loop 唤醒之后不是立马处理 touch 事件的，而是看看 timer 有没有事情，然后是 sources（这里是一个 source0），且第一轮是不执行 touch 事件回调，第二轮才会执行 touch 事件回调，然后是固定循环两轮后程序进入长久休眠状态。

&emsp;当 main run loop 的状态发生变化时会调用 mainRunLoopActivitie 函数，我们可以在其中根据 activity 做想要的处理。具体详细的 CFRunLoopObserverCreate 和 CFRunLoopAddObserver 函数的实现过程在前面都已经分析过，可以参考前面 [iOS 从源码解析Run Loop (四)：Source、Timer、Observer 创建以及添加到 mode 的过程](https://juejin.cn/post/6908639874857828366)
## 线程保活
&emsp;线程为什么需要保活？性能其实很大的瓶颈是在于空间的申请和释放，当我们执行一个任务的时候创建了一个线程，任务结束就释放该线程，如果任务频率比较高，那么一个一直活跃的线程来执行我们的任务就省去申请和释放空间的时间和性能。前面已经讲过了 run loop 需要有 source0/source1/timer/block（\__CFRunLoopModeIsEmpty 函数前面详细分析过） 才能不退出，总不可能直接让他执行 while(1) 吧，这种方法明显不对的，由源码得知，当有监测端口（mach port）的时候（即有 source1 时），也不会退出，也不会影响性能，所以在线程初始化的时候可以使用 `[[NSRunLoop currentRunLoop] addPort:[NSPort port] forMode:NSRunLoopCommonModes];` 来保证 run loop 启动后保活。（CFRunLoopRunSpecific 函数内调用 \__CFRunLoopModeIsEmpty 函数返回 ture 的话，会直接返回 kCFRunLoopRunFinished）

&emsp;如果想让子线程永久保持活性那么就在子线程内调用其 run loop 实例的 run 函数，如果想自由控制线程 run loop 结束时机的话则使用一个变量控制 do while 循环，在循环内部调用子线程的 run loop 实例的 runMode: beforeDate: 函数，当需要停止子线程的 run loop 时则在子线程内调用 `CFRunLoopStop(CFRunLoopGetCurrent());` 并结束 do while 循环，详细内容可参考前面 [iOS 从源码解析Run Loop (一)：run loop 基本概念理解与 NSRunLoop 文档](https://juejin.cn/post/6904921175546298375)
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
2. 如果是 run loop 自动添加的 autoreleasePool，那么在每一次 run loop 循环结束时，autoreleasePool 执行 pop 操作 释放这次循环中所有的自动释放对象。在 run loop 循环开启时再 push 新的自动释放池，保证 run loop 的每次循环中的对象都能得到释放。
## NSTimer 实现过程
&emsp;NSTimer.h 中提供了一组 NSTimer 的创建方法，其中不同构造函数的 NSInvocation、SEL、block 类型的参数分别代表 NSTimer 对象的不同的回调方式。其中 block  的回调形式是 iOS 10.0 后新增的，可以帮助我们避免 NSTimer 对象和其 target 的循环引用问题，`timerWithTimeInterval...` 和 `initWithFireDate` 返回的 NSTimer 对象还需要我们手动添加到当前线程的 run loop 中，`scheduledTimerWithTimeInterval...` 构建的 NSTimer 对象则是默认添加到当前线程的 run loop 的 NSDefaultRunLoopMode 模式下的。

&emsp;block 回调的形式都有一个 `API_AVAILABLE(macosx(10.12), ios(10.0), watchos(3.0), tvos(10.0));`。
### NSTimer 创建函数
&emsp;下面五个方法返回的 NSTimer 对象需要手动调用 NSRunLoop 的 `-(void)addTimer:(NSTimer *)timer forMode:(NSRunLoopMode)mode;` 函数添加到指定线程的指定 mode 下。
```c++
+ (NSTimer *)timerWithTimeInterval:(NSTimeInterval)ti invocation:(NSInvocation *)invocation repeats:(BOOL)yesOrNo;
+ (NSTimer *)timerWithTimeInterval:(NSTimeInterval)ti target:(id)aTarget selector:(SEL)aSelector userInfo:(nullable id)userInfo repeats:(BOOL)yesOrNo;
+ (NSTimer *)timerWithTimeInterval:(NSTimeInterval)interval repeats:(BOOL)repeats block:(void (^)(NSTimer *timer))block API_AVAILABLE(macosx(10.12), ios(10.0), watchos(3.0), tvos(10.0));
- (instancetype)initWithFireDate:(NSDate *)date interval:(NSTimeInterval)interval repeats:(BOOL)repeats block:(void (^)(NSTimer *timer))block API_AVAILABLE(macosx(10.12), ios(10.0), watchos(3.0), tvos(10.0));
- (instancetype)initWithFireDate:(NSDate *)date interval:(NSTimeInterval)ti target:(id)t selector:(SEL)s userInfo:(nullable id)ui repeats:(BOOL)rep NS_DESIGNATED_INITIALIZER;
```
&emsp;下面三个方法返回的 NSTimer 对象会被自动添加到当前线程的 run loop 的 default mode 下。
```c++
+ (NSTimer *)scheduledTimerWithTimeInterval:(NSTimeInterval)ti invocation:(NSInvocation *)invocation repeats:(BOOL)yesOrNo;
+ (NSTimer *)scheduledTimerWithTimeInterval:(NSTimeInterval)ti target:(id)aTarget selector:(SEL)aSelector userInfo:(nullable id)userInfo repeats:(BOOL)yesOrNo;
+ (NSTimer *)scheduledTimerWithTimeInterval:(NSTimeInterval)interval repeats:(BOOL)repeats block:(void (^)(NSTimer *timer))block API_AVAILABLE(macosx(10.12), ios(10.0), watchos(3.0), tvos(10.0));
```
&emsp;如果使用 `scheduledTimerWithTimeInterval...` 则需要注意 run loop 的 mode 切换到 UITrackingRunLoopMode 模式时，计时器会停止回调，当滑动停止 run loop 切回到 kCFRunLoopDefaultMode 模式时计时器又开始正常回调，当手动添加到 run loop 时则尽量添加到  NSRunLoopCommonModes 模式下可保证 run loop 的 mode 切换不影响计时器的回调。

&emsp;还有一个知识点需要注意一下，添加到 run loop 指定 mode 下的 NSTimer 会被 retain，因为它会被加入到 run loop mode 的 \_timers 中去，如果 mode 是 NSRunLoopCommonModes 的话，同时还会被加入到 run loop 的 \_commonModeItems 中。所以  NSTimer 最终必须调用 invalidate 函数把它从指定的集合中移除。
### NSTimer 执行流程
&emsp;CFRunLoopTimerRef 与 NSTimer 是可以 toll-free bridged（免费桥接转换）的。当 timer 加到 run loop 的时候，run loop 会注册对应的触发时间点，时间到了，run loop 若处于休眠则会被唤醒，执行 timer 对应的回调函数。下面我们沿着 CFRunLoopTimerRef 的源码来完整分析一下计时器的流程。
#### CFRunLoopTimerRef 创建
&emsp;首先是 CFRunLoopTimerRef 的创建函数：(详细分析可参考前面的：[iOS 从源码解析Run Loop (四)：Source、Timer、Observer 创建以及添加到 mode 的过程](https://juejin.cn/post/6908639874857828366))
```c++
CFRunLoopTimerRef CFRunLoopTimerCreate(CFAllocatorRef allocator,
                                       CFAbsoluteTime fireDate,
                                       CFTimeInterval interval,
                                       CFOptionFlags flags,
                                       CFIndex order,
                                       CFRunLoopTimerCallBack callout,
                                       CFRunLoopTimerContext *context);
```
&emsp;`allocator` 是 CF 下为新对象分配内存的分配器，可传 NULL 或 kCFAllocatorDefault。

&emsp;`fireDate` 是计时器第一次触发回调的时间点，然后后续沿着 `interval` 间隔时间连续回调。

&emsp;`interval` 是计时器的连续回调的时间间隔，如果为 0 或负数，计时器将触发一次，然后自动失效。

&emsp;`order` 优先级索引，指示 CFRunLoopModeRef 的 _timers 中不同计时器的回调执行顺序。当前忽略此参数，传递 0。

&emsp;`callout` 计时器触发时调用的回调函数。

&emsp;`context` 保存计时器的上下文信息的结构。该函数将信息从结构中复制出来，因此上下文所指向的内存不需要在函数调用之后继续存在。如果回调函数不需要上下文的信息指针来跟踪状态，则可以为 NULL。其中的 void * info 字段内容是 `callout` 函数执行时的参数。

&emsp;CFRunLoopTimerCreate 函数中比较重要的是对触发时间的设置：
```c++
...
// #define TIMER_DATE_LIMIT    4039289856.0
// 如果入参 fireDate 过大，则置为 TIMER_DATE_LIMIT
if (TIMER_DATE_LIMIT < fireDate) fireDate = TIMER_DATE_LIMIT;

// 下次触发的时间
memory->_nextFireDate = fireDate;
memory->_fireTSR = 0ULL;

// 取得当前时间
uint64_t now2 = mach_absolute_time();
CFAbsoluteTime now1 = CFAbsoluteTimeGetCurrent();

if (fireDate < now1) {
    // 如果第一次触发的时间已经过了，则把 _fireTSR 置为当前
    memory->_fireTSR = now2;
} else if (TIMER_INTERVAL_LIMIT < fireDate - now1) {
    // 如果第一次触发的时间点与当前是时间差距超过了 TIMER_INTERVAL_LIMIT，则把 _fireTSR 置为 TIMER_INTERVAL_LIMIT
    memory->_fireTSR = now2 + __CFTimeIntervalToTSR(TIMER_INTERVAL_LIMIT);
} else {
    // 这里则是正常的，如果第一次触发的时间还没有到，则把触发时间设置为当前时间和第一次触发时间点的差值
    memory->_fireTSR = now2 + __CFTimeIntervalToTSR(fireDate - now1);
}
...
```
&emsp;这一部分代码保证计时器第一次触发的时间点正常。下面看一下把创建好的 CFRunLoopModeRef 添加到指定的 run loop 的指定的 run loop mode 下。

#### CFRunLoopAddTimer
&emsp;CFRunLoopAddTimer 函数主要完成把 CFRunLoopTimerRef rlt 插入到 CFRunLoopRef rl 的 CFStringRef modeName 模式下的 \_timer 集合中，如果 modeName 是 kCFRunLoopCommonModes 的话，则把 rlt 插入到 rl 的 \_commonModeItems 中，然后调用 \__CFRunLoopAddItemToCommonModes 函数把 rlt 添加到所有被标记为 common 的 mode 的 \_timer 中，同时也会把 modeName 添加到 rlt 的 \_rlModes 中，记录 rlt 都能在那种 run loop mode 下执行。 
```c++
void CFRunLoopAddTimer(CFRunLoopRef rl, CFRunLoopTimerRef rlt, CFStringRef modeName);
```
&emsp;上面添加完成后，会调用 \__CFRepositionTimerInMode 函数，然后调用 \__CFArmNextTimerInMode，再调用 mk_timer_arm 函数把 CFRunLoopModeRef 的 \_timerPort 和一个时间点注册到系统中，等待着 mach_msg 发消息唤醒 run loop 执行到达时间的计时器。
#### \__CFArmNextTimerInMode
&emsp;同一个 run loop mode 下的多个 timer 共享同一个 \_timerPort，这是一个循环的流程：注册 timer(mk_timer_arm)—接收 timer(mach_msg)—根据多个 timer 计算离当前最近的下次 handle 时间—注册 timer(mk_timer_arm)。

&emsp;在使用 CFRunLoopAddTimer 添加 timer 时的调用堆栈如下：
```c++
CFRunLoopAddTimer
__CFRepositionTimerInMode
    __CFArmNextTimerInMode
        mk_timer_arm
```
&emsp;然后 mach_msg 收到 timer 事件时的调用堆栈如下：
```c++
__CFRunLoopRun
__CFRunLoopDoTimers
    __CFRunLoopDoTimer
        __CFRUNLOOP_IS_CALLING_OUT_TO_A_TIMER_CALLBACK_FUNCTION__
__CFArmNextTimerInMode
    mk_timer_arm 
```
&emsp;每次计时器都会调用 \__CFArmNextTimerInMode 函数，注册计时器的下次回调。`__CFRUNLOOP_IS_CALLING_OUT_TO_A_TIMER_CALLBACK_FUNCTION__(rlt->_callout, rlt, context_info);` 则是执行计时器的 \_callout 函数。
### NSTimer 不准时问题
&emsp;通过上面的 NSTimer 执行流程可看到计时器的回调完全依赖 run loop 的正常循环，那就是 NSTimer 不是一种实时机制，以 main run loop 来说它负责了所有的主线程事件，例如 UI 界面的操作，负责的运算使当前 run loop 持续的时间超过了计时器的间隔时间，那么下一次定时就被延后，这样就造成 timer 的不准时，计时器有个属性叫做 Tolerance (宽容度)，标示了当时间点到后，容许有多少最大误差。如果延后时间过长的话会直接导致计时器本次回调被忽略。


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
