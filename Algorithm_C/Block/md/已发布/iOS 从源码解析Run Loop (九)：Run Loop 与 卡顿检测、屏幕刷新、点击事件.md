# iOS 从源码解析Run Loop (九)：Run Loop 与 卡顿检测、屏幕刷新、点击事件

> &emsp;上一篇我们主要分析了 CFRunLoopTimerRef 相关的内容和部分 CFRunLoopObserverRef 相关的内容，本篇我们详细分析 CFRunLoopSourceRef 相关的内容。

&emsp;在开始之前我们再详细区分一下 CFRunLoopSourceRef 中的 source0/source1。
## source0/source1
&emsp;首先我们从代码层面对 source0 和 source1 版本的 CFRunLoopSourceRef 进行区分，struct \__CFRunLoopSource 通过其内部的 \_context 来进行区分 source0 和 source1。
```c++
struct __CFRunLoopSource {
    ...
    union {
        CFRunLoopSourceContext version0;   
        CFRunLoopSourceContext1 version1;
    } _context;
};
```
&emsp;其中 version0、version1 分别对应 source0 和 source1，下面我们再看一下 CFRunLoopSourceContext 和 CFRunLoopSourceContext1 的定义：
```c++
typedef struct {
    ...
    void * info; // 作为 perform 函数的参数
    ...
    void (*schedule)(void *info, CFRunLoopRef rl, CFStringRef mode); // 当 source0 加入到 run loop 时触发的回调函数（在 CFRunLoopAddSource 函数中可看到其被调用）
    void (*cancel)(void *info, CFRunLoopRef rl, CFStringRef mode); // 当 source0 从 run loop 中移除时触发的回调函数
    
    // source0 要执行的任务块，当 source0 事件被触发时的回调, 调用 __CFRUNLOOP_IS_CALLING_OUT_TO_A_SOURCE0_PERFORM_FUNCTION__ 函数来执行 perform(info)
    void (*perform)(void *info);
} CFRunLoopSourceContext;
```
```c++
typedef struct {
    ...
    void * info; // 作为 perform 函数的参数
    ...
#if (TARGET_OS_MAC && !(TARGET_OS_EMBEDDED || TARGET_OS_IPHONE)) || (TARGET_OS_EMBEDDED || TARGET_OS_IPHONE)

    // getPort 函数指针，用于当 source1 被添加到 run loop 中的时候，从该函数中获取具体的 mach_port_t 对象，用来唤醒 run loop
    mach_port_t (*getPort)(void *info);
    
    // perform 函数指针即指向 run loop 被唤醒后 source1 要执行的回调函数，调用 __CFRUNLOOP_IS_CALLING_OUT_TO_A_SOURCE1_PERFORM_FUNCTION__ 函数来执行
    void * (*perform)(void *msg, CFIndex size, CFAllocatorRef allocator, void *info);
#else
    // 其它平台
    void * (*getPort)(void *info);
    void (*perform)(void *info);
#endif
} CFRunLoopSourceContext1;
```
+ source0 仅包含一个回调函数（perform），它并不能主动唤醒 run loop。使用时，你需要先调用 CFRunLoopSourceSignal(rls) 将这个 source 标记为待处理，然后手动调用 CFRunLoopWakeUp(rl) 来唤醒 run loop（ CFRunLoopWakeUp 函数内部是通过 run loop 的 \_wakeUpPort 端口来唤醒 run loop），唤醒后的 run loop 继续执行 \__CFRunLoopRun 函数内部的外层 do while 循环来处理 timer/source 事件，其中通过调用 \__CFRunLoopDoSources0 函数来执行 source0 事件，执行过后的 source0 会被 \__CFRunLoopSourceUnsetSignaled(rls) 标记为已处理，后续 run loop 循环中不会再执行标记为已处理的 source0。source0 不同于不重复执行的 timer 和 run loop 的 block 链表中的 block 节点，source0 执行过后不会自己主动移除，不重复执行的 timer 和 block 执行过后自己会主动移除。

&emsp;source0 具体的执行函数如下，info 做参数执行 perform 函数。
```c++
__CFRUNLOOP_IS_CALLING_OUT_TO_A_SOURCE0_PERFORM_FUNCTION__(rls->_context.version0.perform, rls->_context.version0.info);
```
&emsp;下面是我们手动创建 source0 的示例代码，创建好的 CFRunLoopSourceRef 必须调用 CFRunLoopSourceSignal 函数把其标记为待处理，否则即使 run loop 正常循环，这里的 rls 也得不到执行，由于 thread 线程中的计时器存在所以这里也可以不用手动调用 CFRunLoopWakeUp 唤醒 run loop，run loop 已是唤醒状态，rls 能在 run loop 的一个循环中正常得到执行，然后是其中的三个断点，当执行到断点时我们在控制台打印 po [NSRunLoop currentRunLoop] 可在 kCFRunLoopDefaultMode 的 sources0 哈希表中看到 rls，以及它的 signalled 标记的值，通过源码可知在 rls 的 perform 待执行之前就会先调用 \__CFRunLoopSourceUnsetSignaled(rls) 把其标记为已经处理，且处理过的 rls 并不会主动移除，它依然被保存在 kCFRunLoopDefaultMode 的 sources0 哈希表中，我们可以使用 CFRunLoopRemoveSource 函数手动移除。source0 不同于不重复执行的 timer 和 run loop 的 block 链表中的 block 节点，source0 执行过后不会自己主动移除，不重复执行的 timer 和 block 执行过后自己会主动移除。

&emsp;话说是执行 source0 时需要手动调用 CFRunLoopWakeUp 来唤醒 run loop，实际觉得好像大部分场景下其它事件都会导致 run loop 正常进行着循环，只要 run loop 进行循环则都能执行 source0 事件，好像并不需要我们刻意的手动调用 CFRunLoopWakeUp 来唤醒 run loop。 
```c++
- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    NSThread *thread = [[NSThread alloc] initWithBlock:^{
        NSLog(@"🧗‍♀️🧗‍♀️ ....");
        
        CFRunLoopSourceContext context = {0, (__bridge void *)(self), NULL, NULL, NULL, NULL, NULL, NULL, NULL, runLoopSourcePerformRoutine};
        CFRunLoopSourceRef rls = CFRunLoopSourceCreate(kCFAllocatorDefault, 0, &context);
        CFRunLoopAddSource(CFRunLoopGetCurrent(), rls, kCFRunLoopDefaultMode);

        // 创建好的 rls 必须手动标记为待处理，否则即使 run loop 正常循环也不会执行此 rls
        CFRunLoopSourceSignal(rls); // ⬅️ 断点 1
        
        // 由于计时器一直在循环执行，所以这里可不需要我们手动唤醒 run loop 
        CFRunLoopWakeUp(CFRunLoopGetCurrent()); // ⬅️ 断点 2

        [NSTimer scheduledTimerWithTimeInterval:1 repeats:YES block:^(NSTimer * _Nonnull timer) {
            NSLog(@"⏰⏰⏰ timer 回调...");
            CFRunLoopRemoveSource(CFRunLoopGetCurrent(), rls, kCFRunLoopDefaultMode); // ⬅️ 断点 4（这里执行一次计时器回调再打断点）
        }];

        [[NSRunLoop currentRunLoop] run]; 
    }];
    
    [thread start];
}

void runLoopSourcePerformRoutine (void *info) {
    NSLog(@"👘👘 %@", [NSThread currentThread]); // ⬅️ 断点 3
}
```
&emsp;初始创建完成的 rls 的 signalled 值为 NO，如果接下来不执行 CFRunLoopSourceSignal(rls) 的话，rls 是不会被 run loop 执行的。
```c++
// ⬅️ 断点 1
...
sources0 = <CFBasicHash 0x282aa55f0 [0x20e729430]>{type = mutable set, count = 1,
entries =>
    1 : <CFRunLoopSource 0x2811f6580 [0x20e729430]>{signalled = No, valid = Yes, order = 0, context = <CFRunLoopSource context>{version = 0, info = 0x139d1c2e0, callout = runLoopSourcePerformRoutine (0x100e929ec)}}
}
...
```
&emsp;CFRunLoopSourceSignal(rls) 执行后，看到 rls 的 signalled 置为 Yes，在 run loop 循环中调用 \__CFRunLoopDoSources0 函数时 rls 会得到执行。
```c++
// ⬅️ 断点 2
...
sources0 = <CFBasicHash 0x282aa55f0 [0x20e729430]>{type = mutable set, count = 1,
entries =>
    1 : <CFRunLoopSource 0x2811f6580 [0x20e729430]>{signalled = Yes, valid = Yes, order = 0, context = <CFRunLoopSource context>{version = 0, info = 0x139d1c2e0, callout = runLoopSourcePerformRoutine (0x100e929ec)}}
}
...
```
&emsp;通过 \__CFRunLoopDoSources0 函数的源码可知在 rls 的 perform 函数执行之前 \__CFRunLoopSourceUnsetSignaled(rls) 已经把 rls 标记为已处理。
```c++
// ⬅️ 断点 3
...
sources0 = <CFBasicHash 0x282aa55f0 [0x20e729430]>{type = mutable set, count = 1,
entries =>
    1 : <CFRunLoopSource 0x2811f6580 [0x20e729430]>{signalled = No, valid = Yes, order = 0, context = <CFRunLoopSource context>{version = 0, info = 0x139d1c2e0, callout = runLoopSourcePerformRoutine (0x100e929ec)}}
}
...
}
```
&emsp;CFRunLoopRemoveSource(CFRunLoopGetCurrent(), rls, kCFRunLoopDefaultMode) 执行过后看到 rls 已经被移除，这里 source0 不同于不重复执行的 timer 和 run loop 的 block 链表中的 block 节点，source0 执行过后不会自己主动移除，timer 和 block 执行过后自己会主动移除。
```c++
// ⬅️ 断点 4
...
sources0 = <CFBasicHash 0x282aa55f0 [0x20e729430]>{type = mutable set, count = 0,
entries =>
}
...
```

+ Source1 包含了一个 mach_port 和一个回调（函数指针），被用于通过内核和其他线程相互发送消息（mach_msg），这种 Source 能主动唤醒 run loop 的线程。

&emsp;可看到 source0 中仅有一些回调函数会在 run loop 的本次循环中执行，而 source1 中有 mach port 可用来主动唤醒 run loop。


## 屏幕点击回调事件
&emsp;

```c++

```

## 卡顿监测
&emsp;卡顿的呈现方式大概可以理解为我们触摸屏幕时系统回馈不及时或者连续滑动屏幕时肉眼可见的掉帧，回归到程序层面的话可知这些感知的来源都是主线程，而分析有没有卡顿发生则可以从主线程的 run loop 入手，可以通过监听 main run loop 的状态，就能够发现主线程调用方法是否执行时间过长而导致了 run loop 循环周期被拉长继而发生了卡顿。所以监测卡顿的方案是：**通过监控 main run loop 的状态变化周期来判断是否出现了卡顿。**

```c++
#import "HCCMonitor.h"
#include <mach/mach_time.h>

@interface HCCMonitor () {
    CFRunLoopObserverRef runLoopObserver;
}

@property (nonatomic, assign) int timeoutCount;
@property (nonatomic, strong) dispatch_semaphore_t dispatchSemaphore;
@property (nonatomic, assign) CFRunLoopActivity runLoopActivity;

@end

@implementation HCCMonitor

+ (instancetype)shareInstance {
    static HCCMonitor *instance = nil;
    static dispatch_once_t onceToken;
    
    dispatch_once(&onceToken, ^{
        instance = [[HCCMonitor alloc] init];
    });
    return instance;
}

- (void)beginMonitor {
    if (runLoopObserver) {
        return;
    }
    
    self.dispatchSemaphore = dispatch_semaphore_create(0); // Dispatch Semaphore 保证同步
    // 创建一个观察者
    CFRunLoopObserverContext context = {0, (__bridge void *)self, NULL, NULL};
    runLoopObserver = CFRunLoopObserverCreate(kCFAllocatorDefault, kCFRunLoopAllActivities, YES, 0, &runLoopObserverCallBack, &context);
    // 将观察者添加到主线程 run loop 的 common 模式下的观察中
    CFRunLoopAddObserver(CFRunLoopGetMain(), runLoopObserver, kCFRunLoopCommonModes);
    
    // 创建子线程监控
    dispatch_async(dispatch_get_global_queue(0, 0), ^{
        while (YES) {
            long semaphoreWait = dispatch_semaphore_wait(self.dispatchSemaphore, dispatch_time(DISPATCH_TIME_NOW, 20 * NSEC_PER_MSEC));
            // semaphoreWait 值不为 0，表示 semaphoreWait 等待超时了
            if (semaphoreWait != 0) {
                if (!self->runLoopObserver) {
                    self.timeoutCount = 0;
                    self.dispatchSemaphore = 0;
                    self.runLoopActivity = 0;
                    return ;
                }
                
                if (self.runLoopActivity == kCFRunLoopBeforeSources || self.runLoopActivity == kCFRunLoopAfterWaiting) {
                    if (++self.timeoutCount < 3) {
                        continue;
                    }
                    
                    NSLog(@"🔠🔠🔠 卡顿发生...");
                }
            }
            
            self.timeoutCount = 0;
        } // end while
    });
}

- (void)endMonitor {
    if (!runLoopObserver) {
        return;
    }
    
    CFRunLoopRemoveObserver(CFRunLoopGetMain(), runLoopObserver, kCFRunLoopCommonModes);
    CFRelease(runLoopObserver);
    runLoopObserver = NULL;
}

int count = 0;
static void runLoopObserverCallBack(CFRunLoopObserverRef observer, CFRunLoopActivity activity, void *info) {
    HCCMonitor *lagMonitor = (__bridge HCCMonitor *)info;
    lagMonitor.runLoopActivity = activity;
    
    ++count;
    
    static uint64_t beforeTimersTSR = 0;
    static uint64_t beforeSourcesTSR = 0;
    static uint64_t beforeWaitingTSR = 0;
    static uint64_t afterWaitingTSR = 0;
    
    //    uint64_t ns_at = (uint64_t)((__CFTSRToTimeInterval(beforeTimersTSR)) * 1000000000ULL);
    
    //    NSLog(@"✳️✳️✳️ beforeTimersTSR %llu", beforeTimersTSR);
    
    switch (activity) {
        case kCFRunLoopEntry:
            count = 0;
            NSLog(@"🤫 - %d kCFRunLoopEntry 即将进入: %@", count, CFRunLoopCopyCurrentMode(CFRunLoopGetCurrent()));
            break;
        case kCFRunLoopBeforeTimers:
            NSLog(@"⏳ - %d kCFRunLoopBeforeTimers 即将处理 timers", count);
            beforeTimersTSR = mach_absolute_time();
            
//            NSLog(@"🔂 AfterWaiting~Timer: %llu", beforeTimersTSR - afterWaitingTSR);
            
            break;
        case kCFRunLoopBeforeSources:
            NSLog(@"💦 - %d kCFRunLoopBeforeSources 即将处理 sources", count);
            beforeSourcesTSR = mach_absolute_time();
            
//            NSLog(@"🔂 Timer~Source: %llu", beforeSourcesTSR - beforeTimersTSR);
            
            break;
        case kCFRunLoopBeforeWaiting:
            count = 0; // 每次 run loop 即将进入休眠时，count 置为 0，可表示一轮 run loop 循环结束
            NSLog(@"🛏 - %d kCFRunLoopBeforeWaiting 即将进入休眠", count);
            beforeWaitingTSR = mach_absolute_time();
            
//            NSLog(@"🔂 Source~BeforeWaiting %llu", beforeWaitingTSR - beforeSourcesTSR);
            
            break;
        case kCFRunLoopAfterWaiting:
            NSLog(@"🦍 - %d kCFRunLoopAfterWaiting 即将从休眠中醒来", count);
            afterWaitingTSR = mach_absolute_time();
            
//            NSLog(@"🔂 BeforeWaiting~AfterWaiting: %llu", afterWaitingTSR - beforeWaitingTSR);
            
            break;
        case kCFRunLoopExit:
            count = 0;
            NSLog(@"🤫 - %d kCFRunLoopExit 即将退出: %@", count, CFRunLoopCopyCurrentMode(CFRunLoopGetCurrent()));
            break;
        case kCFRunLoopAllActivities:
            NSLog(@"🤫 kCFRunLoopAllActivities");
            break;
    }
    
    dispatch_semaphore_t semaphore = lagMonitor.dispatchSemaphore;
    dispatch_semaphore_signal(semaphore);
}

@end
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
