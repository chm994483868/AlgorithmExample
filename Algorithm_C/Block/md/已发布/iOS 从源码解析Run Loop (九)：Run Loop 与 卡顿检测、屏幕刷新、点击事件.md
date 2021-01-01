# iOS 从源码解析Run Loop (九)：Run Loop 与 卡顿检测、屏幕刷新、点击事件

> &emsp;上一篇我们主要分析了 CFRunLoopTimerRef 相关的内容和部分 CFRunLoopObserverRef 相关的内容，本篇我们详细分析 CFRunLoopSourceRef 相关的内容。

&emsp;在开始之前我们再详细区分一下 CFRunLoopSourceRef 的 source0 和 source1 两个版本。
## source0 和 source1 的区别
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
&emsp;source0 仅包含一个回调函数（perform），它并不能主动唤醒 run loop（进入休眠的 run loop 仅能通过 mach port 和 mach_msg 来唤醒）。使用时，你需要先调用 CFRunLoopSourceSignal(rls) 将这个 source 标记为待处理，然后手动调用 CFRunLoopWakeUp(rl) 来唤醒 run loop（CFRunLoopWakeUp 函数内部是通过 run loop 实例的 \_wakeUpPort 成员变量来唤醒 run loop 的），唤醒后的 run loop 继续执行 \__CFRunLoopRun 函数内部的外层 do while 循环来执行 timers（执行到达执行时间点的 timer 以及更新下次最近的时间点） 和 sources 以及 observer 回调 run loop 状态，其中通过调用 \__CFRunLoopDoSources0 函数来执行 source0 事件，执行过后的 source0 会被 \__CFRunLoopSourceUnsetSignaled(rls) 标记为已处理，后续 run loop 循环中不会再执行标记为已处理的 source0。source0 不同于不重复执行的 timer 和 run loop 的 block 链表中的 block 节点，source0 执行过后不会自己主动移除，不重复执行的 timer 和 block 执行过后会自己主动移除，执行过后的 source0 可手动调用 CFRunLoopRemoveSource(CFRunLoopGetCurrent(), rls, kCFRunLoopDefaultMode) 来移除。

&emsp;source0 具体执行时的函数如下，info 做参数执行 perform 函数。
```c++
__CFRUNLOOP_IS_CALLING_OUT_TO_A_SOURCE0_PERFORM_FUNCTION__(rls->_context.version0.perform, rls->_context.version0.info); // perform(info)
```
&emsp;下面是我们手动创建 source0 的示例代码，创建好的 CFRunLoopSourceRef 必须调用 CFRunLoopSourceSignal 函数把其标记为待处理，否则即使 run loop 正常循环，这里的 rls 也得不到执行，由于 thread 线程中的计时器存在所以这里也可以不用手动调用 CFRunLoopWakeUp 唤醒 run loop，run loop 已是唤醒状态，rls 能在 run loop 的一个循环中正常得到执行，然后是其中的三个断点，当执行到断点时我们在控制台打印 po [NSRunLoop currentRunLoop] 可在 kCFRunLoopDefaultMode 的 sources0 哈希表中看到 rls，以及它的 signalled 标记的值，通过源码可知在 rls 的 perform 待执行之前就会先调用 \__CFRunLoopSourceUnsetSignaled(rls) 把其标记为已经处理，且处理过的 rls 并不会主动移除，它依然被保存在 kCFRunLoopDefaultMode 的 sources0 哈希表中，我们可以使用 CFRunLoopRemoveSource 函数手动移除。source0 不同于不重复执行的 timer 和 run loop 的 block 链表中的 block 节点，source0 执行过后不会自己主动移除，不重复执行的 timer 和 block 执行过后自己会主动移除。

&emsp;话说是执行 source0 时需要手动调用 CFRunLoopWakeUp 来唤醒 run loop，实际觉得好像大部分场景下其它事件都会导致 run loop 正常进行着循环，只要 run loop 进行循环则标记为待处理的 source0 就能得到执行，好像并不需要我们刻意的手动调用 CFRunLoopWakeUp 来唤醒当前的 run loop。 
```c++
- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    NSThread *thread = [[NSThread alloc] initWithBlock:^{
        NSLog(@"🧗‍♀️🧗‍♀️ ....");
        
        // 构建下下文，这里只有三个参数有值，0 是 version 值代表是 source0，info 则直接传的 self 即当前的 vc，schedule 和 cancel 偷懒了传的 NULL，它们分别是
        // 执行 CFRunLoopAddSource 添加 rls 和 CFRunLoopRemoveSource 移除 rls 时调用的，大家可以自己试试，
        // 然后最后是执行函数 perform 传了 runLoopSourcePerformRoutine。
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
&emsp;针对 timers/sources（0/1） 的执行流程（暂时忽略 run loop 休眠和 main run loop 执行，其实极极极大部分情况我们都是在使用主线程的 run loop，这里为了分析 timers/sources 暂时假装是在子线程的 run loop 中）我们这里再回顾一下 \__CFRunLoopRun 函数，从 \__CFRunLoopRun 函数的外层 do while 循环开始，首先进来会连着回调 kCFRunLoopBeforeTimers 和 kCFRunLoopBeforeSources 两个 run loop 的活动变化，然后接下来就是调用 \__CFRunLoopDoSources0(rl, rlm, stopAfterHandle) 来执行 source0，如果有 source0 被执行了，则 sourceHandledThisLoop 为 True，就不会回调 kCFRunLoopBeforeWaiting 和 kCFRunLoopAfterWaiting 两个活动变化。接着是根据当前 run loop 的本次循环被某个 mach port 唤醒的（\__CFRunLoopServiceMachPort(waitSet, &msg, sizeof(msg_buffer), &livePort, poll ? 0 : TIMEOUT_INFINITY, &voucherState, &voucherCopy) 唤醒本次 run loop 的 mach port 会被赋值到 livePort 中）来处理具体的内容，假如是 rlm->_timerPort（或 modeQueuePort 它两等同只是针对不同的平台不同的 timer 使用方式）唤醒的则调用 \__CFRunLoopDoTimers(rl, rlm, mach_absolute_time()) 来执行 timer 的回调，如果还有其它 timer 或者 timer 重复执行的话会调用 \__CFArmNextTimerInMode(rlm, rl) 来更新注册下次最近的 timer 的触发时间。  最后的话就是 source1 的端口了，首先通过 CFRunLoopSourceRef rls = __CFRunLoopModeFindSourceForMachPort(rl, rlm, livePort)（内部是 CFRunLoopSourceRef found = rlm->_portToV1SourceMap ? (CFRunLoopSourceRef)CFDictionaryGetValue(rlm->_portToV1SourceMap, (const void *)(uintptr_t)port) : NULL;，即从 rlm 的 \_portToV1SourceMap 字典中以 livePort 为 Key 找到对应的 CFRunLoopSourceRef）来找到 livePort 所对应的具体的 rls（source1），然后是调用 \__CFRunLoopDoSource1(rl, rlm, rls, msg, msg->msgh_size, &reply) 来执行 rls 的回调，内部具体的执行是 \_\_CFRUNLOOP_IS_CALLING_OUT_TO_A_SOURCE1_PERFORM_FUNCTION\_\_(rls->_context.version1.perform, msg, size, reply, rls->_context.version1.info) 即同样是 info 做参数执行 perform 函数，且在临近执行前同样会调用 \__CFRunLoopSourceUnsetSignaled(rls) 把 source1 标记为已处理，且同 soure0 一样也不会主动从 rlm 的 sources1 哈希表中主动移除。（source1 系统会自动 signaled）

&emsp; Source1 包含了一个 mach port（由 CFRunLoopSourceRef 创建时的 CFRunLoopSourceContext1 传入） 和一个回调（CFRunLoopSourceContext1 的 perform 函数指针），被用于通过内核和其它线程相互发送消息（mach_msg），这种 Source 能主动唤醒 run loop 的线程。

&emsp;Source1 包含的 mach port 来自于创建 source1 时 CFRunLoopSourceContext1 的 info 成员变量，CFRunLoopSourceRef 通过 \_context  的 info 持有 mach port，同时以 CFRunLoopSourceRef 为 Key，以 mach port 为 Value 保存在 rlm 的 \_portToV1SourceMap 中，并且会把该 mach port 插入到 rlm 的 \_portSet 中。如下代码摘录自 CFRunLoopAddSource 函数中：
```c++
...
} else if (1 == rls->_context.version0.version) {
    // 把 rls 添加到 rlm 的 _sources1 集合中
    CFSetAddValue(rlm->_sources1, rls);
    
    // 以 info 为参，调用 rls->_context.version1.getPort 函数读出 mach port
    // 基于 CFMachPort 创建的 CFRunLoopSourceRef 其 context 的 getPort 指针被赋值为 __CFMachPortGetPort 函数（iOS 下仅能使用 CFMachPort，不能使用 CFMessagePort）
    // 基于 CFMessagePort 创建的 CFRunLoopSourceRef 其 context 的 getPort 指针被赋值为 __CFMessagePortGetPort 函数（macOS 下可用 CFMessagePort）
    __CFPort src_port = rls->_context.version1.getPort(rls->_context.version1.info);
    
    if (CFPORT_NULL != src_port) {
        // 把 rls 和 src_port 保存在 rlm 的 _portToV1SourceMap 字典中
        CFDictionarySetValue(rlm->_portToV1SourceMap, (const void *)(uintptr_t)src_port, rls);
        // 把 src_port 插入到 rlm 的 _portSet 中
        __CFPortSetInsert(src_port, rlm->_portSet);
    }
}
...
```
&emsp;可看到 source0 中仅有一些回调函数（perform 函数指针）会在 run loop 的本次循环中执行，而 source1 中有 mach port 可用来主动唤醒 run loop 后执行 source1 中的回调函数（perform 函数指针），即 source1 创建时会有 mach port 传入，然后当通过 mach_msg 函数向这个 mach port 发消息时，当前的 run loop 就会被唤醒来执行这个 source1 事件，但是 source0 则是依赖于由 “别人” 来唤醒 run loop，例如由开发者手动调用 CFRunLoopWakeUp 函数来唤醒 run loop，或者由 source1 唤醒 run loop 后，在当前 run loop 的本次循环中被标记为待处理的 source0 也趁机得到执行。 

&emsp;source1 由 run loop 和内核管理，mach port 驱动。 source0 则偏向应用层一些，如 Cocoa 里面的 UIEvent 处理，会以 source0 的形式发送给 main run loop。

&emsp;翻看了几篇博客后发现手动唤醒 run loop 适用的场景可以是在主线程中唤醒休眠中的子线程。只要能拿到子线程的 run loop 对象就能通过调用 CFRunLoopWakeUp 函数来唤醒指定的子线程，唤醒的方式是调用 mach_msg 函数向子线程的 run loop 对象的 \_weakUpPort 发送消息即可。下面我们看一下挺简短的源码。

&emsp;CFRunLoopWakeUp 函数定义如下，只需要一个我们想要唤醒的线程的 run loop 对象。
```c++
void CFRunLoopWakeUp(CFRunLoopRef rl) {
    CHECK_FOR_FORK();
    // This lock is crucial to ignorable wakeups, do not remove it.
    
    // CFRunLoopRef 加锁
    __CFRunLoopLock(rl);
    
    // 如果 rl 已经被标记为 "忽略唤醒"，则直接解锁 return，
    // 其实当 rl 有这个 "忽略唤醒" 的标记时代表的是 rl 此时已经是唤醒状态了，所以本次唤醒操作可以忽略。
    // 全局搜索 __CFRunLoopSetIgnoreWakeUps 设置 "忽略唤醒" 标记的函数，
    // 可发现其调用都是在 __CFRunLoopRun 函数中 run loop 唤醒之前，用来标记 run loop 此时是唤醒状态。 
    if (__CFRunLoopIsIgnoringWakeUps(rl)) {
        __CFRunLoopUnlock(rl);
        return;
    }
    
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
    kern_return_t ret;
    
    /* We unconditionally try to send the message, since we don't want to lose a wakeup,
    but the send may fail if there is already a wakeup pending, since the queue length is 1. */
    
    // __CFSendTrivialMachMessage 函数内部正是调用 mach_msg 向 rl->_wakeUpPort 端口发送消息
    ret = __CFSendTrivialMachMessage(rl->_wakeUpPort, 0, MACH_SEND_TIMEOUT, 0);
    // 发送不成功且不是超时，则 crash
    if (ret != MACH_MSG_SUCCESS && ret != MACH_SEND_TIMED_OUT) CRASH("*** Unable to send message to wake up port. (%d) ***", ret);
    
#elif DEPLOYMENT_TARGET_WINDOWS
    SetEvent(rl->_wakeUpPort);
#endif
    // CFRunLoopRef 解锁
    __CFRunLoopUnlock(rl);
}
```
&emsp;如此，主线程通过调用 CFRunLoopWakeUp(rl) 来唤醒子线程的 run loop，那么添加到子线程中的标记为待处理的 source0 就能得到执行了。

&emsp;Cocoa Foundation 和 Core Foundation 为使用与端口相关的对象和函数创建基于端口的输入源（source1）提供内置支持。例如，在 Cocoa Foundation 中，我们根本不需要直接创建 source1，只需创建一个端口对象，并使用 NSRunLoop  的实例方法将该端口添加到 run loop 中。port 对象会处理所需 source1 的创建和配置。如下代码在子线程中:
```c++
NSPort *port = [NSPort port];
[[NSRunLoop currentRunLoop] addPort:port forMode:NSDefaultRunLoopMode];
```
&emsp;即可在当前 run loop 的 NSDefaultRunLoopMode 模式的 sources1 集合中添加一个 source1，此时只要在主线程中能拿到 port 我们就可以实现主线和子线的通信（唤醒子线程）。

&emsp;在上面示例代码中打一个断点，然后在控制台执行 po [NSRunLoop currentRunLoop]，可看到 kCFRunLoopDefaultMode 模式的 sources1 哈希表中多了一个 source1: 
```c++
...
sources1 = <CFBasicHash 0x28148ebe0 [0x20e729430]>{type = mutable set, count = 1,
entries =>
    2 : <CFRunLoopSource 0x282fd9980 [0x20e729430]>{signalled = No, valid = Yes, order = 200, context = <CFMachPort 0x282ddca50 [0x20e729430]>{valid = Yes, port = a20b, source = 0x282fd9980, callout = __NSFireMachPort (0x1df1ee1f0), context = <CFMachPort context 0x28148ec70>}}
}
...
```

&emsp;在 Core Foundation 中则必须手动创建端口及其 source1。在这两种情况下，都使用与端口不透明类型（CFMachPortRef、CFMessagePortRef 或 CFSocketRef）相关联的函数来创建适当的对象。

## 事件响应
> &emsp;在 com.apple.uikit.eventfetch-thread 线程下苹果注册了一个 Source1 (基于 mach port 的) 用来接收系统事件，其回调函数为 \__IOHIDEventSystemClientQueueCallback()，HID 是 Human Interface Devices “人机交互” 的首字母缩写。
> 
> &emsp;当一个硬件事件(触摸/锁屏/摇晃等)发生后，首先由 IOKit.framework 生成一个 IOHIDEvent 事件并由 SpringBoard 接收。这个过程的详细情况可以参考[这里](http://iphonedevwiki.net/index.php/IOHIDFamily)。SpringBoard 只接收按键(锁屏/静音等)，触摸，加速，接近传感器等几种 Event，随后用 mach port 转发给需要的 App 进程。随后苹果注册的那个 Source1 就会触发回调，并调用 \_UIApplicationHandleEventQueue() 进行应用内部的分发。
> 
> &emsp;\_UIApplicationHandleEventQueue() 会把 IOHIDEvent 处理并包装成 UIEvent 进行处理或分发，其中包括识别 UIGesture/处理屏幕旋转/发送给 UIWindow 等。通常事件比如 UIButton 点击、touchesBegin/Move/End/Cancel 事件都是在这个回调中完成的。[深入理解RunLoop](https://blog.ibireme.com/2015/05/18/runloop/)

&emsp;我们在程序中添加一个 \__IOHIDEventSystemClientQueueCallback 的符号断点，运行程序后触摸屏幕会进入该断点，然后 bt 打印当前的函数调用堆栈如下，可看到目前是在 com.apple.uikit.eventfetch-thread 线程，此时主线程是休眠状态，系统正是通过 com.apple.uikit.eventfetch-thread 来唤醒主线程。
```c++
(lldb) bt
* thread #6, name = 'com.apple.uikit.eventfetch-thread', stop reason = breakpoint 2.1
  * frame #0: 0x00000001dea0745c IOKit`__IOHIDEventSystemClientQueueCallback // ⬅️ （mp 是 CFMachPortRef）mp->_callout(mp, msg, size, context_info);
    frame #1: 0x00000001de6ea990 CoreFoundation`__CFMachPortPerform + 188 // ⬅️ CFMachPort 端口的回调函数
    frame #2: 0x00000001de711594 CoreFoundation`__CFRUNLOOP_IS_CALLING_OUT_TO_A_SOURCE1_PERFORM_FUNCTION__ + 56 // ⬅️ 
    frame #3: 0x00000001de710ce0 CoreFoundation`__CFRunLoopDoSource1 + 440 // ⬅️ 可看到触摸事件确实是 source1 事件
    frame #4: 0x00000001de70bb04 CoreFoundation`__CFRunLoopRun + 2096
    frame #5: 0x00000001de70afb4 CoreFoundation`CFRunLoopRunSpecific + 436
    frame #6: 0x00000001df0d995c Foundation`-[NSRunLoop(NSRunLoop) runMode:beforeDate:] + 300
    frame #7: 0x00000001df0d97ec Foundation`-[NSRunLoop(NSRunLoop) runUntilDate:] + 96
    frame #8: 0x000000020b052754 UIKitCore`-[UIEventFetcher threadMain] + 136
    frame #9: 0x00000001df2064a0 Foundation`__NSThread__start__ + 984
    frame #10: 0x00000001de39d2c0 libsystem_pthread.dylib`_pthread_body + 128
    frame #11: 0x00000001de39d220 libsystem_pthread.dylib`_pthread_start + 44
    frame #12: 0x00000001de3a0cdc libsystem_pthread.dylib`thread_start + 4
(lldb) 
```
&emsp;在控制台打印 po [NSRunLoop currentRunLoop]，看一下当前线程的 run loop，此时应在模拟器中运行，可能由于真机的访问控制有关，如果使用真机的话无法看到 sources 的具体的回调函数名，用模拟器可以看到。由于内容太多，这里我们只摘录出只有一个 kCFRunLoopDefaultMode 模式的 sources1 中的一个 source1：
```c++
...
sources1 = <CFBasicHash 0x600000cf0210 [0x7fff80617cb0]>{type = mutable set, count = 3,
entries =>
    ...
    1 : <CFRunLoopSource 0x6000037a8780 [0x7fff80617cb0]>{signalled = No, valid = Yes, order = 0, context = <CFMachPort 0x6000035a0580 [0x7fff80617cb0]>{valid = Yes, port = 3307, source = 0x6000037a8780, callout = __IOHIDEventSystemClientQueueCallback (0x7fff25e91d1d), context = <CFMachPort context 0x7fbc69e007f0>}}
    ...
}
...
```
&emsp;这里确实如大佬所说，包含一个回调函数是 \__IOHIDEventSystemClientQueueCallback 的 source1。

&emsp;下面我们看一下另一位大佬关于事件响应更详细一点的分析：[iOS RunLoop完全指南](https://blog.csdn.net/u013378438/article/details/80239686)

> &emsp;iOS 设备的事件响应，是有 RunLoop 参与的。提起 iOS 设备的事件响应，相信大家都会有一个大概的了解: (1) 用户触发事件 -> (2) 系统将事件转交到对应 APP 的事件队列 -> (3) APP 从消息队列头取出事件 -> (4) 交由 Main Window 进行消息分发 -> (5) 找到合适的 Responder 进行处理，如果没找到，则会沿着 Responder chain 返回到 APP 层，丢弃不响应该事件。
> 
> &emsp;这里涉及到两个问题，(3) 到 (5) 步是由进程内处理的，而 (1) 到 (2) 步则涉及到设备硬件，iOS 操作系统，以及目标 APP 之间的通信，通信的大致步骤是什么样的呢？当我们的 APP 在接收到任何事件请求之前，main RunLoop 都是处于 mach_msg_trap 休眠状态中的，那么，又是谁唤醒它的呢？（com.apple.uikit.eventfetch-thread）

&emsp;首先我们在控制台用 po [NSRunLoop currentRunLoop] 打印出主线程的 run loop 的内容，这里内容超多，我们只摘录和我们分析相关的内容，可看到当前 main run loop 有 4 种 mode，这里我们只看 kCFRunLoopDefaultMode 和 UITrackingRunLoopMode 以及 kCFRunLoopCommonModes，它们三者下均有一个 source0 事件：
```c++
...
current mode = kCFRunLoopDefaultMode,
common modes = <CFBasicHash 0x60000014a400 [0x7fff80617cb0]>{type = mutable set, count = 2,
entries =>
    0 : <CFString 0x7fff867f6c40 [0x7fff80617cb0]>{contents = "UITrackingRunLoopMode"}
    2 : <CFString 0x7fff8062b0a0 [0x7fff80617cb0]>{contents = "kCFRunLoopDefaultMode"}
}

// UITrackingRunLoopMode
2 : <CFRunLoopMode 0x6000034500d0 [0x7fff80617cb0]>{name = UITrackingRunLoopMode, port set = 0x3003, queue = 0x60000215c500, source = 0x60000215c600 (not fired), timer port = 0x3203, 
sources0 = <CFBasicHash 0x600000167cc0 [0x7fff80617cb0]>{type = mutable set, count = 4,
entries =>
    ...
    4 : <CFRunLoopSource 0x600003a58780 [0x7fff80617cb0]>{signalled = No, valid = Yes, order = -1, context = <CFRunLoopSource context>{version = 0, info = 0x600003a5c180, callout = __handleEventQueue (0x7fff48126d97)}}
    ...
}

// kCFRunLoopDefaultMode
4 : <CFRunLoopMode 0x60000345c270 [0x7fff80617cb0]>{name = kCFRunLoopDefaultMode, port set = 0x2103, queue = 0x600002150c00, source = 0x600002150d00 (not fired), timer port = 0x2a03, 
sources0 = <CFBasicHash 0x600000167d20 [0x7fff80617cb0]>{type = mutable set, count = 4,
entries =>
    ...
    4 : <CFRunLoopSource 0x600003a58780 [0x7fff80617cb0]>{signalled = No, valid = Yes, order = -1, context = <CFRunLoopSource context>{version = 0, info = 0x600003a5c180, callout = __handleEventQueue (0x7fff48126d97)}}
    ...
}
...
```
> &emsp;此 source0 的回调函数是 \__handleEventQueue，APP 就是通过这个回调函数来处理事件队列的。
>
> &emsp;但是，我们注意到了，\__handleEventQueue 所对应的 source 类型是 0，也就是说它本身不会唤醒休眠的 main RunLoop, main 线程自身在休眠状态中也不可能自己去唤醒自己，那么，系统肯定还有一个子线程，用来接收事件并唤醒 main thread，并将事件传递到 main thread上。
&emsp;确实还一个子线程，我们将 APP 暂停，就会看到，除了主线程外，系统还为我们自动创建了几个子线程，通过 Xcode 左侧 Debug 导航可看到一个名字比较特殊的线程 com.apple.uikit.eventfetch-thread(7) 

&emsp;看线程的名字就知道，它是 UIKit 所创建的用于接收 event 的线程(以下我们简称为 event fetch thread)。我们打印出 com.apple.uikit.eventfetch-thread 的 RunLoop。


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
+ [IOHIDFamily](http://iphonedevwiki.net/index.php/IOHIDFamily)
