# iOS 从源码解析Run Loop (三)：以 CFRunLoop.h 文件为入口深入学习 run loop

> &emsp;CFRunLoop.h 文件是 run loop 在 Core Foundation 下的最重要的头文件，与我们前面学习的 Cocoa Foundation 下的 NSRunLoop.h 文件相对应。NSRunLoop 的内容也正是对 \__CFRunLoop 的面向对象的简单封装，CFRunLoop.h 文件包含更多 run loop 的操作以及 run loop 涉及的部分底层数据结构的声明，\__CFRunLoop 结构则是 run loop 在 Core Foundation 下 C 语言的实现。本篇以 CFRunLoop.h 文件为入口通过 Apple 开源的 CF-1151.16 来深入学习 run loop。⛽️⛽️
## CFRunLoop Overview
&emsp;CFRunLoop 对象监视任务的输入源（sources of input），并在准备好进行处理时调度控制。输入源（input sources）的示例可能包括用户输入设备、网络连接、周期性或延时事件以及异步回调。

&emsp;run loop 可以监视三种类型的对象：sources（CFRunLoopSource）、timers（CFRunLoopTimer）和 observers（CFRunLoopObserver）。要在这些对象需要处理时接收回调，必须首先使用 `CFRunLoopAddSource`、`CFRunLoopAddTimer` 或 `CFRunLoopAddObserver` 将这些对象放入 run loop 中，以后也可以从 run loop 中删除它们（或使其 invalidate）以停止接收其回调。 

&emsp;添加到 run loop 的每个 source、timer 和 observer 必须与一个或多个 run loop modes 相关联。Modes 决定 run loop 在给定迭代期间处理哪些事件。每次 run loop 执行时，它都以特定 mode 执行。在该 mode 下，run loop 只处理与该 mode 关联的 sources、timers 和 observers 关联的事件。你可以将大多数 sources 分配给默认的 run loop mode（由 kCFRunLoopDefaultMode 常量指定），该 mode 用于在应用程序（或线程）空闲时处理事件。然而，系统定义了其它 modes，并且可以在其它 modes 下执行 run loop，以限制处理哪些 sources、timers 和 observers。因为 run loop modes 被简单地指定为字符串，所以你还可以定义自己的自定义 mode 来限制事件的处理。

&emsp;Core Foundation 定义了一种特殊的伪模式（pseudo-mode），称为 common modes，允许你将多个 mode 与给定的 source、timer 或 observer 关联起来。要指定 common modes，请在配置对象时为 mode 使用 kCFRunLoopCommonModes 常量。每个 run loop 都有自己独立的 set of common modes，默认 mode（kCFRunlopDefaultMode）始终是该 set 的成员。要向 set of common modes 添加 mode，请使用 `CFRunLoopAddCommonMode` 函数。

&emsp;每个线程只有一个 run loop。既不能创建（系统帮创建，不需要开发者自己手动创建）也不能销毁线程的 run loop。Core Foundation 会根据需要自动为你创建它。使用 `CFRunLoopGetCurrent` 获取当前线程的 run loop。调用 `CFRunLoopRun` 以默认模式运行当前线程的 run loop，直到 run loop 被 `CFRunLoopStop` 停止。也可以调用 `CFRunLoopRunInMode` 以指定的 mode 运行当前线程的 run loop 一段时间（或直到 run loop 停止）。只有在请求的模式至少有一个要监视的 source 或 timer 时，才能运行 run loop。

&emsp;Run loop 可以递归运行。你可以从任何运行循环 callout 中调用 `CFRunLoopRun` 或 `CFRunLoopRunInMode`，并在当前线程的调用堆栈上创建嵌套的运行循环激活（run loop activations）。在调用中可以运行的 modes 不受限制。你可以创建另一个在任何可用的运行循环模式下运行的运行循环激活，包括调用堆栈中已经运行的任何模式。（You can create another run loop activation running in any available run loop mode, including any modes already running higher in the call stack.）

&emsp;Cocoa 应用程序基于 CFRunLoop 来实现它们自己的高级事件循环（NSRunLoop）。Cocoa 编写应用程序时，可以将 sources、timers 和 observers 添加到它们的 run loop 对象和 modes 中。然后，对象将作为常规应用程序事件循环的一部分进行监视。使用 NSRunLoop 的 `getCFRunLoop` 方法获取相应的 CFRunLoopRef 类型。在 Carbon 应用程序中，使用 `getcfrunloopfrompeventloop` 函数。

&emsp;有关 run loop 的行为的更多信息，请参见 Threading Programming Guide 中的 Run Loops。（即上篇内容）

&emsp;以上是 CFRunLoop 文档的综述，估计对大家而言都是老生常谈的内容了。下面我们则深入源码，看看在代码层面是如何构建 run loop 体系的。

&emsp;Core Foundation 中的 CFRunLoop 都是 C API，提供了 run loop 相当丰富的接口，且都是线程安全的，NSRunLoop 是对 CFRunLoopRef 的封装，提供了面向对象的 API，非线程安全的。使用 NSRunLoop 的 `getCFRunLoop` 方法即可获取相应的 `CFRunLoopRef` 类型。

&emsp;下面我们对 Cocoa Foundation 和 Core Foundation 之间区别做一些拓展。

> &emsp;Core Foundation 框架 (CoreFoundation.framework) 是一组 C 语言接口，它们为 iOS 应用程序提供基本数据管理和服务功能。该框架支持进行管理的数据以及可提供的服务：群体数据类型 (数组、集合等)、程序包、字符串管理、日期和时间管理、原始数据块管理、偏好管理、URL及数据流操作、线程和RunLoop、端口和soket通讯。
> &emsp;Core Foundation 框架和 Cocoa Foundation 框架紧密相关，它们为相同功能提供接口，但Cocoa Foundation 框架提供 Objective-C 接口。如果你将 Cocoa Foundation 对象和 Core Foundation 类型掺杂使用，则可利用两个框架之间的 “toll-free bridging”。所谓的 Toll-free bridging 是说你可以在某个框架的方法或函数同时使用 Core Foundatio 和 Cocoa Foundation 框架中的某些类型。很多数据类型支持这一特性，其中包括群体和字符串数据类型。每个框架的类和类型描述都会对某个对象是否为 toll-free bridged，应和什么对象桥接进行说明。
>
> &emsp;下面看一下Objective-C 指针与 Core Foundation 指针之间的转换规则：
>
> &emsp;ARC 仅管理 Objective-C 指针（retain、release、autorelease），不管理 Core Foundation 指针，CF 指针需要我们手动的 CFRetain 和 CFRelease 来管理（对应 MRC 时的 retain/release），CF 中没有 autorelease。
> &emsp;Cocoa Foundation 指针与 Core Foundation指针转换，需要考虑的是**所指向对象所有权的归属**。ARC 提供了 3 个修饰符来管理。
> &emsp;1. \__bridge，什么也不做，仅仅是转换。此种情况下：
> &emsp;    1.1：从 Cocoa 转换到 Core，需要手动 CFRetain，否则，Cocoa 指针释放后，传出去的指针则无效。
> &emsp;    1.2：从 Core 转换到 Cocoa，需要手动 CFRelease，否则，Cocoa 指针释放后，对象引用计数仍为 1，不会被销毁。
> &emsp;2. \__bridge_retained，转换后自动调用 CFRetain，即帮助自动解决上述 1.1 的情形。
> &emsp;（\__bridge_retained or CFBridgingRetain，ARC 把对象所有权转出，需 Core Foundation 处理。）
> &emsp;3. \__bridge_transfer，转换后自动调用 CFRelease，即帮助自动解决上述 1.2 的情形。
> &emsp;（\__bridge_transfer or CFBridgingRelease，Core Foundation 把对象所有权交给 ARC，由 ARC 自动处理。）
> &emsp;[Cocoa Foundation和 Core Foundation之间数据转换（桥接 \__bridge）](https://www.cnblogs.com/qingpeng/p/4568239.html)

## Run Loop 数据结构
&emsp;首先看到 CFRunLoop.h 文件的内容被包裹在 `CF_IMPLICIT_BRIDGING_ENABLED` 和 `CF_IMPLICIT_BRIDGING_DISABLED` 两个宏之间， 它们是一对表示隐式桥接转换的宏。
```c++
#ifndef CF_IMPLICIT_BRIDGING_ENABLED
#if __has_feature(arc_cf_code_audited)
#define CF_IMPLICIT_BRIDGING_ENABLED _Pragma("clang arc_cf_code_audited begin")
#else
#define CF_IMPLICIT_BRIDGING_ENABLED
#endif
#endif
```
```c++
#ifndef CF_IMPLICIT_BRIDGING_DISABLED
#if __has_feature(arc_cf_code_audited)
#define CF_IMPLICIT_BRIDGING_DISABLED _Pragma("clang arc_cf_code_audited end")
#else
#define CF_IMPLICIT_BRIDGING_DISABLED
#endif
#endif
```
&emsp;CF_BRIDGED_MUTABLE_TYPE 宏表示 Cocoa 和 Core Foundation 下对应的免费桥接转换类型。如 id 和 struct \__CFRunLoop，id 和 struct \__CFRunLoopSource，NSTimer 和 struct \__CFRunLoopTimer。 
```c++
#if __has_attribute(objc_bridge)
    #define CF_BRIDGED_MUTABLE_TYPE(T)    __attribute__((objc_bridge_mutable(T)))
#else
    #define CF_BRIDGED_MUTABLE_TYPE(T)
#endif
```
&emsp;然后看到几个重要的 typedef 声明。Core Founction 中 run loop 相关的数据结构有：CFRunLoopRef、CFRunLoopSourceRef、CFRunLoopObserverRef、CFRunLoopTimerRef 等等。
### CFRunLoopRef（struct \__CFRunLoop *）
&emsp;在 Core Foundation 下 \__CFRunLoop 结构是 Run Loop 对应的数据结构，对应 Cocoa 中的 NSRunLoop 类。CFRunLoopRef 则是指向 \__CFRunLoop 结构体的指针。
```c++
typedef struct __CFRunLoop * CFRunLoopRef;

struct __CFRunLoop { 
    CFRuntimeBase _base; // 所有 CF "instances" 都是从这个结构开始的
    pthread_mutex_t _lock; /* locked for accessing mode list */ 锁定以访问模式列表
    
    // typedef mach_port_t __CFPort;
    // 唤醒 run loop 的端口，这个是 run loop 原理的关键所在，可通过 port 来触发 CFRunLoopWakeUp 函数
    __CFPort _wakeUpPort; // used for CFRunLoopWakeUp 手动唤醒 run loop 的端口。初始化 run loop 时设置，仅用于 CFRunLoopWakeUp，CFRunLoopWakeUp 函数会向 _wakeUpPort 发送一条消息
    
    Boolean _unused; // 标记是否使用过
    
    volatile _per_run_data *_perRunData; // reset for runs of the run loop // run loop 运行会重置的一个数据结构
    pthread_t _pthread; // run loop 所对应的线程，二者是一一对应的。（之前在学习线程时并没有在线程的结构体中看到有 run loop 相关的字段，其实线程的 run loop 是存在了 TSD 中，当然如果是线程有获取 run loop 的话）
    uint32_t _winthread;
    
    CFMutableSetRef _commonModes; // 存储字符串（而非 runloopMode 对象）的容器，对应着所有标记为 common 的 mode。
    CFMutableSetRef _commonModeItems; // 存储 modeItem 对象的容器，对应于所有标记为 common 的 mode 下的 Item（即 source、timer、observer）
    CFRunLoopModeRef _currentMode; // 当前运行的 Run Loop Mode 实例的指针（typedef struct __CFRunLoopMode *CFRunLoopModeRef）
    CFMutableSetRef _modes; // 集合，存储的是 CFRunLoopModeRef
    
    // 链表头指针，该链表保存了所有需要被 run loop 执行的 block。
    // 外部通过调用 CFRunLoopPerformBlock 函数来向链表中添加一个 block 节点。
    // run loop 会在 CFRunLoopDoBlock 时遍历该链表，逐一执行 block。
    struct _block_item *_blocks_head;
    
    // 链表尾指针，之所以有尾指针，是为了降低增加 block 节点时的时间复杂度。
    //（例如有新节点插入时，只有头节点的话，要从头节点遍历才能找到尾节点，现在已经有尾节点不需要遍历了，则时间复杂度从 O(n) 降到了 O(1）。)
    struct _block_item *_blocks_tail;
    
    // 绝对时间是自参考日期到参考日期（纪元）为 2001 年 1 月 1 日 00:00:00 的时间间隔。
    // typedef double CFTimeInterval;
    // typedef CFTimeInterval CFAbsoluteTime;
    
    CFAbsoluteTime _runTime; // 运行时间点
    CFAbsoluteTime _sleepTime; // 休眠时间点
    
    // 所有 "CF objects" 的基础 "type" 及其上的多态函数（polymorphic functions）
    // typedef const void * CFTypeRef;
    CFTypeRef _counterpart;
};
```
#### CFRuntimeBase
&emsp;所有 CF "instances" 都是从这个结构开始的。不要直接引用这些字段--它们是供 CF 使用的，可以在没有警告的情况下添加或删除或更改格式。不能保证从一个版本到另一个版本使用此结构的二进制兼容性。
```c++
typedef struct __CFRuntimeBase {
    uintptr_t _cfisa;
    uint8_t _cfinfo[4];
#if __LP64__
    uint32_t _rc;
#endif
} CFRuntimeBase;
```
#### \_per_run_data
&emsp;重置 run loop 时用的数据结构，每次 run loop 运行后都会重置 _perRunData 的值。
```c++
typedef struct _per_run_data {
    uint32_t a;
    uint32_t b;
    uint32_t stopped; // run loop 是否停止
    uint32_t ignoreWakeUps; // run loop 是否已唤醒
} _per_run_data;
```
#### \_block_item
&emsp;需要被 run loop 执行的 block 链表中的节点数据结构。
```c++
struct _block_item {
    struct _block_item *_next; // 指向下一个节点
    
    // typedef const void * CFTypeRef; 
    // CFString 或 CFSet 类型，也就是说一个 block 可能对应单个或多个 mode 
    CFTypeRef _mode;    // CFString or CFSet
    
    void (^_block)(void); // 真正要执行的 block 本体
};
```
&emsp;上面是 CFRunLoopRef 涉及的相关数据结构，特别是其中与 mode 相关的 _modes、_commonModes、_commonModeItems 三个成员变量都是  CFMutableSetRef 可变集合类型，也正对应了前面的一些结论，一个 run loop 对应多个 mode，一个 mode 下可以包含多个 modeItem（更详细的内容在下面的 __CFRunLoopMode 结构中）。既然 run loop 包含多个 mode 那么它定可以在不同的 mode 下运行，run loop 一次只能在一个 mode 下运行，如果想要切换 mode，只能退出 run loop，然后再根据指定的 mode 运行 run loop，这样可以是使不同的 mode 下的 modeItem 相互隔离，不会相互影响。

&emsp;下面看两个超级重要的函数，获取主线程的 run loop 和获取当前线程（子线程）的 run loop。
### CFRunLoopGetMain/CFRunLoopGetCurrent
&emsp;`CFRunLoopGetMain/CFRunLoopGetCurrent` 函数可分别用于获取主线程的 run loop 和获取当前线程（子线程）的 run loop。main run loop 使用一个静态变量 \__main 存储，子线程的 run loop 会保存在当前线程的 TSD 中。两者在第一次获取 run loop 时都会调用 `_CFRunLoopGet0` 函数根据线程的 pthread_t 对象从静态全局变量 \__CFRunLoops（static CFMutableDictionaryRef）中获取，如果获取不到的话则新建 run loop 对象，并根据线程的 pthread_t 保存在静态全局变量 \__CFRunLoops（static CFMutableDictionaryRef）中，方便后续读取。
```c++
CFRunLoopRef CFRunLoopGetMain(void) {
    // 用于检查给定的进程是否被分叉
    CHECK_FOR_FORK();
    // __main 是一个静态变量，只能初始化一次，用于保存主线程关联的 run loop 对象
    static CFRunLoopRef __main = NULL; // no retain needed
    
    // 只有第一个获取 main run loop 时 __main 值为 NULL，
    // 然后从静态全局的 CFMutableDictionaryRef __CFRunLoops 中根据主线程查找 main run loop，
    // 赋值给 __main，以后再获取 main run loop，即直接返回 __main。
    //（主线程和 main run loop 都是全局唯一的，pthread_main_thread_np() 获取主线程）
    if (!__main) __main = _CFRunLoopGet0(pthread_main_thread_np()); // no CAS needed
    
    // 返回 main run loop
    return __main;
}

CFRunLoopRef CFRunLoopGetCurrent(void) {
    // 用于检查给定的进程是否被分叉
    CHECK_FOR_FORK();
    
    // 从当前线程的 TSD 中获取其 run loop，
    // 如果未找到的话则去静态全局的 CFMutableDictionaryRef __CFRunLoops 中根据 pthread_t 查找 run loop
    CFRunLoopRef rl = (CFRunLoopRef)_CFGetTSD(__CFTSDKeyRunLoop);
    if (rl) return rl;
    
    // TSD 中未找到当前线程的 run loop 的话，即是第一次获取当前线程 run loop 的情况，
    // 系统会为当前线程创建一个 run loop，会把它存入当前线程的 TSD 中同时也会存入静态全局变量 __CFRunLoops 中。
    
    // pthread_self() 获取当前线程的 pthread_t。
    
    // 去静态全局的 CFMutableDictionaryRef __CFRunLoops 中根据 pthread_t 查找当前线程的 run loop，
    // 如果找不到的话则会为当前线程进行创建 run loop。
    return _CFRunLoopGet0(pthread_self());
}
```
#### _CFRunLoopGet0
&emsp;`_CFRunLoopGet0` 函数，可以通过当前线程的 pthread_t 来获取其 run loop 对象，如果没有则新创建一个 run loop 对象。创建之后，将 run loop 对象保存在静态全局 \__CFRunLoops 中，同时还会保存在当前线程的 TSD 中。
```c++
// 静态全局的 CFMutableDictionaryRef，key 是 pthread_t，value 是 CFRunLoopRef。
static CFMutableDictionaryRef __CFRunLoops = NULL;

// #if DEPLOYMENT_TARGET_MACOSX
// typedef pthread_mutex_t CFLock_t; 在 macOS 下 CFLock_t 是互斥锁

// 用于访问 __CFRunLoops 时加锁
static CFLock_t loopsLock = CFLockInit;

// should only be called by Foundation
// t==0 is a synonym for "main thread" that always works 
// t 为 0 等同于获取主线程的 run loop

// 外联函数，根据入参 pthread_t t 获取该线程的 run loop
CF_EXPORT CFRunLoopRef _CFRunLoopGet0(pthread_t t) {
    // static pthread_t kNilPthreadT = (pthread_t)0;
    // 如果 t 是 nil，则把 t 赋值为主线程
    if (pthread_equal(t, kNilPthreadT)) {
        t = pthread_main_thread_np();
    }
    
    // macOS 下 __CFLock 是互斥锁加锁
    // #define __CFLock(LP) ({ (void)pthread_mutex_lock(LP); })
    
    // 加锁
    __CFLock(&loopsLock);
    // 第一次调用时，__CFRunLoops 不存在则进行新建，并且会直接为主线程创建一个 run loop，并保存进 __CFRunLoops 中
    if (!__CFRunLoops) {
        // 解锁
        __CFUnlock(&loopsLock);
        // 创建 CFMutableDictionaryRef
        CFMutableDictionaryRef dict = CFDictionaryCreateMutable(kCFAllocatorSystemDefault, 0, NULL, &kCFTypeDictionaryValueCallBacks);
        // 根据主线程的 pthread_t 创建 run loop
        CFRunLoopRef mainLoop = __CFRunLoopCreate(pthread_main_thread_np());
    
        // #define pthreadPointer(a) a
        // 把 mainLoop 根据主线程的 pthread_t 作为 key 保存在 dict 中
        CFDictionarySetValue(dict, pthreadPointer(pthread_main_thread_np()), mainLoop);
        
        // InterlockedCompareExchangePointer 函数是进行原子性的比较和交换指针指向的值。
        // 把 dst 内存地址中存储的值与 oldp 进行比较，如果相等，则用 newp 指向的内存地址与 dst 内存地址中存储的值进行交换。
        // 返回值是 dst 内存地址中存储的值。
        // CF_EXPORT bool OSAtomicCompareAndSwapPtrBarrier(void *oldp, void *newp, void *volatile *dst) 
        // { 
        //     return oldp == InterlockedCompareExchangePointer(dst, newp, oldp);
        // }
        
        // 原子性的比较交换内存空间中值，如果 &__CFRunLoops 存储的值是 NULL 的话，把 dict 指向的内存地址与 &__CFRunLoops 内存中的值进行交换，并返回 True。
        // 当 &__CFRunLoops 内存空间中的值不是 NULL 时，不发生交换，返回 false，此时进入会 if，执行释放 dict 操作。
        if (!OSAtomicCompareAndSwapPtrBarrier(NULL, dict, (void * volatile *)&__CFRunLoops)) {
            // 释放 dict 
            CFRelease(dict);
        }
        
        // 释放 mainLoop，这里 __CFRunLoops 已经持有 mainLoop，这里的 release 并不会导致 mainLoop 对象被销毁
        CFRelease(mainLoop);
        
        // 加锁 
        __CFLock(&loopsLock);
    }
    
    // 根据线程的 pthread_t 从 __CFRunLoops 中获取其对应的 run loop
    CFRunLoopRef loop = (CFRunLoopRef)CFDictionaryGetValue(__CFRunLoops, pthreadPointer(t));
    
    // 解锁
    __CFUnlock(&loopsLock);
    
    // 如果 loop 不存在，则新建 run loop，例如子线程第一次获取 run loop 时都会走到这里，需要为它创建 run loop。
    if (!loop) {
        // 根据线程创建 run loop
        CFRunLoopRef newLoop = __CFRunLoopCreate(t);
        // 加锁
        __CFLock(&loopsLock);
        
        // 再次判断 __CFRunLoops 中是否有线程 t 的 run loop，因为第一次判断后已经解锁了，可能在多线程的场景下已经创建好了该 run loop
        loop = (CFRunLoopRef)CFDictionaryGetValue(__CFRunLoops, pthreadPointer(t));
        if (!loop) {
            // 把 newLoop 根据线程 t 保存在 __CFRunLoops 中
            CFDictionarySetValue(__CFRunLoops, pthreadPointer(t), newLoop);
            // 赋值给 loop
            loop = newLoop;
        }
        
        // don't release run loops inside the loopsLock, because CFRunLoopDeallocate may end up taking it
        // 不要在 loopsLock 内部释放运行循环，因为 CFRunLoopDeallocate 最终可能会占用它
        
        // 解锁
        __CFUnlock(&loopsLock);
        
        // 放入 __CFRunLoops 时，__CFRunLoops 会持有 newLoop，这里的 release 只是对应 __CFRunLoopCreate(t) 时的引用计数 + 1
        CFRelease(newLoop);
    }
    
    // 这里判断入参线程 t 是否就是当前线程，如果是的话则可以直接把 loop 保存在当前线程的 TSD 中。
    if (pthread_equal(t, pthread_self())) {
        // loop 存入 TSD 中，方便 CFRunLoopGetCurrent 中直接读取
        _CFSetTSD(__CFTSDKeyRunLoop, (void *)loop, NULL);
        
        if (0 == _CFGetTSD(__CFTSDKeyRunLoopCntr)) {
            // 注册一个回调 __CFFinalizeRunLoop，当线程销毁时，顺便销毁其 run loop 对象。
            _CFSetTSD(__CFTSDKeyRunLoopCntr, (void *)(PTHREAD_DESTRUCTOR_ITERATIONS-1), (void (*)(void *))__CFFinalizeRunLoop);
        }
    }
    return loop;
}
```
&emsp;


#### CHECK_FOR_FORK
&emsp;Forking is a system call where a process creates a copy of itself. CHECK_FOR_FORK is a boolean value in the code which checks whether the given process was forked.（Forking 是系统调用，其中进程创建其自身的副本。 CHECK_FOR_FORK 是代码中的布尔值，用于检查给定的进程是否被分叉。）[What's the meaning of CHECK_FOR_FORK?](https://stackoverflow.com/questions/47260563/whats-the-meaning-of-check-for-fork)
```c++
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
extern uint8_t __CF120293;
extern uint8_t __CF120290;
extern void __THE_PROCESS_HAS_FORKED_AND_YOU_CANNOT_USE_THIS_COREFOUNDATION_FUNCTIONALITY___YOU_MUST_EXEC__(void);
#define CHECK_FOR_FORK() do { __CF120290 = true; if (__CF120293) __THE_PROCESS_HAS_FORKED_AND_YOU_CANNOT_USE_THIS_COREFOUNDATION_FUNCTIONALITY___YOU_MUST_EXEC__(); } while (0)
#endif

#if !defined(CHECK_FOR_FORK)
#define CHECK_FOR_FORK() do { } while (0)
#endif

CF_PRIVATE void __THE_PROCESS_HAS_FORKED_AND_YOU_CANNOT_USE_THIS_COREFOUNDATION_FUNCTIONALITY___YOU_MUST_EXEC__(void) {
    write(2, EXEC_WARNING_STRING_1, sizeof(EXEC_WARNING_STRING_1) - 1);
    write(2, EXEC_WARNING_STRING_2, sizeof(EXEC_WARNING_STRING_2) - 1);
//    HALT;
}
```
### CFRunLoopModeRef（struct \__CFRunLoopMode *）
&emsp;每次 run loop 开始 run 的时候，都必须指定一个 mode，称为 run loop mode。mode 指定了在这次的 run 中，run loop 可以处理的任务。对于不属于当前 mode 的任务，则需要切换 run loop 至对应 mode 下，再重新调用 run 方法，才能够被处理。
```c++
typedef struct __CFRunLoopMode *CFRunLoopModeRef;

struct __CFRunLoopMode {
    CFRuntimeBase _base; // 所有 CF "instances" 都是从这个结构开始的
    pthread_mutex_t _lock; /* must have the run loop locked before locking this */ 必须在锁定之前将 run loop 锁定，即加锁前需要 run loop 先加锁
    CFStringRef _name; // mode 都会指定一个字符串名称
    Boolean _stopped; // 标记了 run loop 的运行状态，实际上并非如此简单，还有前面的 _per_run_data。
    char _padding[3]; // 
    
    // _sources0、_sources1、_observers、_timers 都是集合类型，里面都是 mode item，即一个 mode 包含多个 mode item
    CFMutableSetRef _sources0; // sources0 事件集合（之所以用集合是为了保证每个元素唯一）
    CFMutableSetRef _sources1; // sources1 事件集合
    CFMutableArrayRef _observers; // run loop observer 观察者数组
    CFMutableArrayRef _timers; // 定时器数组
    
    CFMutableDictionaryRef _portToV1SourceMap; // 存储了 Source1 的 port 与 source 的对应关系，key 是 mach_port_t，value 是 CFRunLoopSourceRef
    __CFPortSet _portSet; // 保存所有需要监听的 port，比如 _wakeUpPort，_timerPort 都保存在这个集合中
    CFIndex _observerMask; // 添加 obsever 时设置 _observerMask 为 observer 的 _activities（CFRunLoopActivity 状态）
    
    // DEPLOYMENT_TARGET_MACOSX 表示是否部署在 maxOS 下
    // #if DEPLOYMENT_TARGET_MACOSX
    //  #define USE_DISPATCH_SOURCE_FOR_TIMERS 1
    //  #define USE_MK_TIMER_TOO 1
    // #else
    //  #define USE_DISPATCH_SOURCE_FOR_TIMERS 0
    //  #define USE_MK_TIMER_TOO 1
    // #endif
    
#if USE_DISPATCH_SOURCE_FOR_TIMERS
    // macOS 下，使用 dispatch_source 表示 timer
    dispatch_source_t _timerSource; // GCD 定时器
    dispatch_queue_t _queue; // 队列
    Boolean _timerFired; // set to true by the source when a timer has fired 计时器触发时由 source 设置为 true
    Boolean _dispatchTimerArmed;
#endif

#if USE_MK_TIMER_TOO
    // iOS 下，使用 MK 表示 timer 
    mach_port_t _timerPort; // MK_TIMER 的 port
    Boolean _mkTimerArmed;
#endif

#if DEPLOYMENT_TARGET_WINDOWS
    DWORD _msgQMask;
    void (*_msgPump)(void);
#endif

    // timer 软临界点
    uint64_t _timerSoftDeadline; /* TSR */
    // timer 硬临界点
    uint64_t _timerHardDeadline; /* TSR */
};
```
### CFRunLoopSourceRef（struct \__CFRunLoopSource *）
&emsp;CFRunLoopSourceRef 是事件源（输入源），通过源码可以发现，其分为 source0 和 source1 两个。
```c++
typedef struct __CFRunLoopSource * CFRunLoopSourceRef;

struct __CFRunLoopSource {
    CFRuntimeBase _base; // 所有 CF "instances" 都是从这个结构开始的
    uint32_t _bits;
    pthread_mutex_t _lock; // 互斥锁
    CFIndex _order; /* immutable */ source 的优先级，值为小，优先级越高
    CFMutableBagRef _runLoops; // run loop 集合
    union {
        CFRunLoopSourceContext version0; /* immutable, except invalidation */
        CFRunLoopSourceContext1 version1; /* immutable, except invalidation */
    } _context;
};
```
&emsp;CFRunLoopSourceContext 
```c++
typedef struct {
    CFIndex    version;
    void *    info; // source 的信息
    
    const void *(*retain)(const void *info); // retain 函数
    void    (*release)(const void *info); // release 函数
    
    CFStringRef    (*copyDescription)(const void *info);
    
    Boolean    (*equal)(const void *info1, const void *info2); // 判断 source 相等的函数
    
    CFHashCode    (*hash)(const void *info);
    void    (*schedule)(void *info, CFRunLoopRef rl, CFStringRef mode);
    void    (*cancel)(void *info, CFRunLoopRef rl, CFStringRef mode);
    
    void    (*perform)(void *info); // source 要执行的任务块
} CFRunLoopSourceContext;
```
&emsp;CFRunLoopSourceContext1
```c++
typedef struct {
    CFIndex    version;
    void *    info; // source 的信息
    
    const void *(*retain)(const void *info); // retain 函数
    void    (*release)(const void *info); // release 函数
    
    CFStringRef    (*copyDescription)(const void *info);
    
    Boolean    (*equal)(const void *info1, const void *info2); // 判断 source 相等的函数
    
    // #if __LLP64__
    //  typedef unsigned long long CFOptionFlags;
    //  typedef unsigned long long CFHashCode;
    //  typedef signed long long CFIndex;
    // #else
    //  typedef unsigned long CFOptionFlags;
    //  typedef unsigned long CFHashCode;
    //  typedef signed long CFIndex;
    // #endif
    
    CFHashCode    (*hash)(const void *info);
#if (TARGET_OS_MAC && !(TARGET_OS_EMBEDDED || TARGET_OS_IPHONE)) || (TARGET_OS_EMBEDDED || TARGET_OS_IPHONE)
    mach_port_t    (*getPort)(void *info);
    void *    (*perform)(void *msg, CFIndex size, CFAllocatorRef allocator, void *info);
#else
    void *    (*getPort)(void *info);
    void    (*perform)(void *info);
#endif
} CFRunLoopSourceContext1;
```
#### CFAllocatorRef
&emsp;在大多数情况下，为创建函数指定分配器时，NULL 参数表示 “使用默认值”；这与使用 kCFAllocatorDefault 或 `CFAllocatorGetDefault()` 的返回值相同。这样可以确保你将使用当时有效的分配器。
```c++
typedef const struct __CFAllocator * CFAllocatorRef;

struct __CFAllocator {
    CFRuntimeBase _base;
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
    // CFAllocator structure must match struct _malloc_zone_t!
    // The first two reserved fields in struct _malloc_zone_t are for us with CFRuntimeBase
    size_t     (*size)(struct _malloc_zone_t *zone, const void *ptr); /* returns the size of a block or 0 if not in this zone; must be fast, especially for negative answers */
    void     *(*malloc)(struct _malloc_zone_t *zone, size_t size);
    void     *(*calloc)(struct _malloc_zone_t *zone, size_t num_items, size_t size); /* same as malloc, but block returned is set to zero */
    void     *(*valloc)(struct _malloc_zone_t *zone, size_t size); /* same as malloc, but block returned is set to zero and is guaranteed to be page aligned */
    void     (*free)(struct _malloc_zone_t *zone, void *ptr);
    void     *(*realloc)(struct _malloc_zone_t *zone, void *ptr, size_t size);
    void     (*destroy)(struct _malloc_zone_t *zone); /* zone is destroyed and all memory reclaimed */
    const char    *zone_name;

    /* Optional batch callbacks; these may be NULL */
    unsigned    (*batch_malloc)(struct _malloc_zone_t *zone, size_t size, void **results, unsigned num_requested); /* given a size, returns pointers capable of holding that size; returns the number of pointers allocated (maybe 0 or less than num_requested) */
    void    (*batch_free)(struct _malloc_zone_t *zone, void **to_be_freed, unsigned num_to_be_freed); /* frees all the pointers in to_be_freed; note that to_be_freed may be overwritten during the process */

    struct malloc_introspection_t    *introspect;
    unsigned    version;
    
    /* aligned memory allocation. The callback may be NULL. */
    void *(*memalign)(struct _malloc_zone_t *zone, size_t alignment, size_t size);
    
    /* free a pointer known to be in zone and known to have the given size. The callback may be NULL. */
    void (*free_definite_size)(struct _malloc_zone_t *zone, void *ptr, size_t size);
#endif
    CFAllocatorRef _allocator;
    CFAllocatorContext _context;
};
```
### CFRunLoopObserverRef（struct \__CFRunLoopObserver *）
&emsp;CFRunLoopObserverRef 是观察者，每个 Observer 都包含了一个回调(函数指针)，当 RunLoop 的状态发生变化时，观察者就能通过回调接受到这个变化。主要是用来向外界报告 Runloop 当前的状态的更改。
```c++
typedef struct __CFRunLoopObserver * CFRunLoopObserverRef;

struct __CFRunLoopObserver {
    CFRuntimeBase _base; // 所有 CF "instances" 都是从这个结构开始的
    pthread_mutex_t _lock; // 互斥锁
    CFRunLoopRef _runLoop; // observer 所观察的 run loop
    CFIndex _rlCount; // observer 观察了多少个 run loop
    CFOptionFlags _activities; /* immutable */ // 所监听的事件，通过位异或，可以监听多种事件，_activities 用来说明要观察 runloop 的哪些状态，一旦指定了就不可变。
    CFIndex _order; /* immutable */ // observer 优先级
    
    // typedef void (*CFRunLoopObserverCallBack)(CFRunLoopObserverRef observer, CFRunLoopActivity activity, void *info);
    CFRunLoopObserverCallBack _callout; /* immutable */ // observer 回调函数，观察到 run loop 状态变化后的回调
    
    CFRunLoopObserverContext _context; /* immutable, except invalidation */ // observer 上下文
};
```
#### CFRunLoopActivity
&emsp;运行循环观察者活动。
```c++
/* Run Loop Observer Activities */
typedef CF_OPTIONS(CFOptionFlags, CFRunLoopActivity) {
    kCFRunLoopEntry = (1UL << 0), // 进入 RunLoop 循环(这里其实还没进入)
    kCFRunLoopBeforeTimers = (1UL << 1), // Run Loop 要处理 timer 了
    kCFRunLoopBeforeSources = (1UL << 2), // Run Loop 要处理 source 了
    kCFRunLoopBeforeWaiting = (1UL << 5), // Run Loop 要休眠了
    kCFRunLoopAfterWaiting = (1UL << 6), // Run Loop 醒了
    kCFRunLoopExit = (1UL << 7), // Run Loop 退出（和 kCFRunLoopEntry 对应，Entry 和 Exit 在每次 Run Loop 循环中仅调用一次，用于表示即将进入循环和退出循环。）
    kCFRunLoopAllActivities = 0x0FFFFFFFU
};
```
#### CFRunLoopObserverContext
```c++
typedef struct {
    CFIndex    version;
    void *    info;
    const void *(*retain)(const void *info);
    void    (*release)(const void *info);
    CFStringRef    (*copyDescription)(const void *info);
} CFRunLoopObserverContext;
```
### CFRunLoopTimerRef（struct \__CFRunLoopTimer *）
&emsp;NSTimer 是与 run loop 息息相关的，CFRunLoopTimerRef 与 NSTimer 是可以 toll-free bridged（免费桥转换）的。当 timer 加到 run loop 的时候，run loop 会注册对应的触发时间点，时间到了，run loop 若处于休眠则会被唤醒，执行 timer 对应的回调函数。
```c++
typedef struct CF_BRIDGED_MUTABLE_TYPE(NSTimer) __CFRunLoopTimer * CFRunLoopTimerRef;

struct __CFRunLoopTimer {
    CFRuntimeBase _base; // 所有 CF "instances" 都是从这个结构开始的
    uint16_t _bits; // 标记 timer 的状态
    pthread_mutex_t _lock; // 互斥锁
    CFRunLoopRef _runLoop; // timer 对应的 run loop
    CFMutableSetRef _rlModes; // timer 对应的 run loop modes
    CFAbsoluteTime _nextFireDate; // timer 的下次触发时机，每次触发后都会再次设置该值
    CFTimeInterval _interval; /* immutable */
    CFTimeInterval _tolerance; /* mutable */ // timer 的允许时间偏差
    uint64_t _fireTSR; /* TSR units */ // timer 本次需要被触发的时间
    CFIndex _order; /* immutable */
    
    // typedef void (*CFRunLoopTimerCallBack)(CFRunLoopTimerRef timer, void *info);
    CFRunLoopTimerCallBack _callout; /* immutable */ // timer 回调
    
    CFRunLoopTimerContext _context; /* immutable, except invalidation */ // timer 上下文
};
```
#### CFRunLoopTimerContext
```c++
typedef struct {
    CFIndex    version;
    void *    info;
    const void *(*retain)(const void *info);
    void    (*release)(const void *info);
    CFStringRef    (*copyDescription)(const void *info);
} CFRunLoopTimerContext;
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
