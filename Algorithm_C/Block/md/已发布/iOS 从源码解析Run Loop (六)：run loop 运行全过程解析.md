# iOS 从源码解析Run Loop (六)：run loop 运行全过程解析

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
+ `kCFRunLoopRunFinished` 运行循环模式没有源或计时器。（当 run loop 对象被标记为正在销毁时也会返回 kCFRunLoopRunFinished）
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
        // 调用 CFRunLoopRunSpecific 函数，以 kCFRunLoopDefaultMode 启动当前线程的 run loop，运行时间传入的是 10^10 秒（2777777 个小时），
        // returnAfterSourceHandled 参数传入的是 false，指示 run loop 是在处理一个源之后不退出并持续处理事件。
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
&emsp;`CFRunLoopRunSpecific` 函数内部会调用 `__CFRunLoopRun` 函数，然后可以把 `result = __CFRunLoopRun(rl, currentMode, seconds, returnAfterSourceHandled, previousMode);` 此行的调用看作一个分界线。行前是，则是首先判断 `rl` 是否被标记为正在销毁，如果是的话则直接返回 kCFRunLoopRunFinished，否则继续往下执行，会根据 `modeName` 从 `rl` 的 `_modes` 中找到其对应的 `CFRunLoopModeRef`，如果未找到或者 `CFRunLoopModeRef` 的 sources/timers 为空，则也是直接返回  kCFRunLoopRunFinished。然后是修改 `rl` 的 `_perRunData` 和 `_currentMode` 同时还会记录之前的旧值，此时一切准备就绪，在调用之前会根据 `rl` 的 `_currentMode` 的 `_observerMask` 判断是否需要回调 run loop observer 观察者来告诉它们 run loop 要进入 kCFRunLoopEntry 状态了，然后调用 `__CFRunLoopRun` 函数正式启动 run loop。

&emsp;`__CFRunLoopRun` 函数返回后则是，首先根据 `rl` 的 `_currentMode` 的 `_observerMask` 判断是否需要回调 run loop observer 观察者来告诉它们 run loop 要进入 kCFRunLoopExit 状态了。然后是把 run loop 对象恢复到之前的 `_perRunData` 和 `_currentMode`（处理 run loop 的嵌套）。

&emsp;上面描述的可能不太清晰，看下面的代码和注释已经极其清晰了。
```c++
SInt32 CFRunLoopRunSpecific(CFRunLoopRef rl,
                            CFStringRef modeName,
                            CFTimeInterval seconds,
                            Boolean returnAfterSourceHandled) {     /* DOES CALLOUT */
    CHECK_FOR_FORK();
    
    // 从 rl 的 _cfinfo 字段中取 rl 是否正在销毁的标记值，如果是的话，则直接返回 kCFRunLoopRunFinished
    if (__CFRunLoopIsDeallocating(rl)) return kCFRunLoopRunFinished;
    
    // CFRunLoop 加锁
    __CFRunLoopLock(rl);
    
    // 调用 __CFRunLoopFindMode 函数从 rl 的 _modes 中找到名字是 modeName 的 run loop mode，
    // 如果找不到的话第三个参数传的是 false，则直接返回 NULL。 
    //（CFRunLoopMode 加锁）
    CFRunLoopModeRef currentMode = __CFRunLoopFindMode(rl, modeName, false);
    
    // 如果 currentMode 为 NULL 或者 currentMode 里面是空的不包含 source/timer（block）则 return 
    if (NULL == currentMode || __CFRunLoopModeIsEmpty(rl, currentMode, rl->_currentMode)) {
        Boolean did = false;
        
        // 如果 currentMode 存在，则进行 CFRunLoopMode 解锁，
        // 对应了上面 __CFRunLoopFindMode(rl, modeName, false) 调用内部的 CFRunLoopMode 加锁 
        if (currentMode) __CFRunLoopModeUnlock(currentMode);
        
        // CFRunLoop 解锁
        __CFRunLoopUnlock(rl);
        
        // 返回 kCFRunLoopRunFinished
        return did ? kCFRunLoopRunHandledSource : kCFRunLoopRunFinished;
    }
    
    // __CFRunLoopPushPerRunData 函数内部是修改 rl 的 _perRunData 字段的各成员变量的值，并返回之前的 _perRunData，
    //（函数内部修改 _perRunData 的值其实是在标记 run loop 不同状态）
    //（这里的 previousPerRun 是用于下面的 __CFRunLoopRun 函数调用返回后，当前的 run loop 对象要回到之前的 _perRunData）。
    volatile _per_run_data *previousPerRun = __CFRunLoopPushPerRunData(rl);
    
    // previousMode 记录 rl 当前的 run loop mode，相比入参传入的 modeName 取得的 run loop mode 而言，它是之前的 run loop mode，
    // 这个 previousMode 主要用于下面的那行 __CFRunLoopRun 函数调用返回后，当前的 run loop 对象要回到之前的 run loop mode。
    //（同上面的 previousPerRun 数据，也要把当前的 run loop 对象回到之前的 _perRunData 数据的状态）
    CFRunLoopModeRef previousMode = rl->_currentMode;
    
    // 更新 rl 的 _currentMode 为入参 modeName 对应的 run loop mode 
    rl->_currentMode = currentMode;
    
    // 临时变量 result，用于当函数返回时记录 run loop 不同的退出原因
    int32_t result = kCFRunLoopRunFinished;
    
    // 判断如果 currentMode 的 _observerMask 字段中包含 kCFRunLoopEntry 的值（_observerMask 内记录了需要观察 run loop 哪些状态变化），
    // 则告诉 currentMode 的 run loop observer 发生了一个 run loop 即将进入循环的状态变化。 
    if (currentMode->_observerMask & kCFRunLoopEntry ) __CFRunLoopDoObservers(rl, currentMode, kCFRunLoopEntry);
    
    // 启动 run loop，__CFRunLoopRun 函数超长，可能是看源码以来最长的一个函数，下面会逐行进行细致的分析
    // ♻️♻️♻️♻️♻️♻️
    result = __CFRunLoopRun(rl, currentMode, seconds, returnAfterSourceHandled, previousMode);
    
    // ⬆️⬆️⬆️ __CFRunLoopRun 函数好像也是不会返回的，当它返回时就代表当前的 run loop 要退出了。 
    
    // 同上的 kCFRunLoopEntry 进入循环的回调，这里则是退出 run loop 的回调。
    // 如果 currentMode 的 _observerMask 中包含 kCFRunLoopExit 的值，
    // 即 run loop observer 需要观察 run loop 的 kCFRunLoopExit 退出状态切换
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
&emsp;这里需要注意的一个点是 `CFRunLoopRunSpecific` 函数最后又把之前的 `previousPerRun` 和 `previousMode` 重新赋值给 run loop 的 `_perRunData` 和 `_currentMode`，它们正是用来处理 run loop 的嵌套运行的。下面看一下 `CFRunLoopRunSpecific` 函数内部调用的一些函数。
#### \__CFRunLoopIsDeallocating
&emsp;`__CFRunLoopIsDeallocating` 函数用于判断 `rl` 是否被标记为正在销毁。该值记录在 `_cfinfo` 字段中。
```c++
CF_INLINE Boolean __CFRunLoopIsDeallocating(CFRunLoopRef rl) {
    return (Boolean)__CFBitfieldGetValue(((const CFRuntimeBase *)rl)->_cfinfo[CF_INFO_BITS], 2, 2);
}
```
#### \__CFRunLoopModeIsEmpty
&emsp;`__CFRunLoopModeIsEmpty` 函数用于判断 `rlm` 中是否没有 sources/timers。在 `CFRunLoopRunSpecific` 函数内部调用 `__CFRunLoopModeIsEmpty` 函数时这里的三个参数要区分一下：`rl` 是 run loop 对象指针，然后 `rlm` 是 `rl` 即将要用此 `rlm` 启动，然后 `previousMode` 则是 `rl` 当前的 `_currentMode` 字段的值。
```c++
// expects rl and rlm locked 进入 __CFRunLoopModeIsEmpty 函数调用前 rl 和 rlm 的 _lock 都已经加锁了
static Boolean __CFRunLoopModeIsEmpty(CFRunLoopRef rl, CFRunLoopModeRef rlm, CFRunLoopModeRef previousMode) {
    CHECK_FOR_FORK();
    
    // 如果 rlm 为 NULL 则直接返回 true
    if (NULL == rlm) return true;
    
#if DEPLOYMENT_TARGET_WINDOWS
    if (0 != rlm->_msgQMask) return false;
#endif
    
    // pthread_main_np() 是判断当前是否是主线程，主线程的 run loop 是程序启动时就启动了，
    // 这些事情是系统自己处理的，我们开发者能做的是控制自己创建的子线程的 run loop，所以当我们自己调用 __CFRunLoopModeIsEmpty 函数时，
    // 一定是在我们自己的子线程内，此时 libdispatchQSafe 的值就一定都是 false 的。
    
    // #define HANDLE_DISPATCH_ON_BASE_INVOCATION_ONLY 0
    Boolean libdispatchQSafe = pthread_main_np() && 
                               ((HANDLE_DISPATCH_ON_BASE_INVOCATION_ONLY && NULL == previousMode) ||
                               (!HANDLE_DISPATCH_ON_BASE_INVOCATION_ONLY && 0 == _CFGetTSD(__CFTSDKeyIsInGCDMainQ)));
                               
    // 在主线程，rl 的 _commonModes 包含 rlm->_name，则返回 false，表示 rlm 不是空的，rl 在此 mode 下可以运行
    if (libdispatchQSafe && (CFRunLoopGetMain() == rl) && CFSetContainsValue(rl->_commonModes, rlm->_name)) return false; // represents the libdispatch main queue
    
    // 下面三条分别判断 rlm 的 _sources0 集合不为空、_sources1 集合不为空、_timers 数组不为空，
    // 都可以直接表示 rlm 不是空的，rl 可以在此 mode 下运行。
    if (NULL != rlm->_sources0 && 0 < CFSetGetCount(rlm->_sources0)) return false;
    if (NULL != rlm->_sources1 && 0 < CFSetGetCount(rlm->_sources1)) return false;
    if (NULL != rlm->_timers && 0 < CFArrayGetCount(rlm->_timers)) return false;
    
    // 下面还有一点判断 run loop mode 不为空的依据，判断 rl 的 block 链表中包含的 block 的 _mode 是否和入参的 rlm 相同。
    // 这里是一个新知识点，前面我们说过无数次如果 run loop mode 的 source/timer 为空时 run loop 则不能在此 mode 下运行，
    // 下面涉及到了一个新的点，还有一种情况下，此情况对应了 run loop observer。
    
    // 这里要注意一下: _mode 的值可能是一个字符串也可能是一个集合，当是一个字符串时表示一个 run loop mode 的 name，
    // 当是一个集合时包含的是一组 run loop mode 的 name。
    
    // struct _block_item {
    //     struct _block_item *_next; // 下一个节点
    //     CFTypeRef _mode; // CFString or CFSet 可表示在一个 mode 下执行或者在多种 mode 下都可以执行
    //     void (^_block)(void); // 当前的 block 
    // };
    
    // 取得 rl 的 block 链表的头节点
    struct _block_item *item = rl->_blocks_head;
    
    // 开始遍历 block 的链表，但凡找到一个可在 rlm 下执行的 block 节点，都表示 rlm 不为空，run loop 可在此 mode 下运行
    while (item) {
        struct _block_item *curr = item;
        item = item->_next;
        Boolean doit = false;
        
        // curr 的 _mode 是字符串或者集合
        if (CFStringGetTypeID() == CFGetTypeID(curr->_mode)) {
            // 是字符串时，rlm 的 _name 是否和它相等，或者 curr 的 _mode 是 kCFRunLoopCommonModes，
            // 判断 rlm 的 _name 是否被包含在 rl 的 _commonModes 中
            doit = CFEqual(curr->_mode, rlm->_name) || 
                   (CFEqual(curr->_mode, kCFRunLoopCommonModes) &&
                    CFSetContainsValue(rl->_commonModes, rlm->_name));
        } else {
            // 是集合时，同上判断 curr 的 _mode 集合内是否包含 rlm 的 _name，或者 curr 的 _mode 集合包含 kCFRunLoopCommonModes，
            // 那么判断 rl 的 _commonModes 是否包含 rlm 的 _name
            doit = CFSetContainsValue((CFSetRef)curr->_mode, rlm->_name) || 
                   (CFSetContainsValue((CFSetRef)curr->_mode, kCFRunLoopCommonModes) &&
                    CFSetContainsValue(rl->_commonModes, rlm->_name));
        }
        
        // 如果 doit 为真，即 rl 的 block 链表中的 block 可执行的模式包含 rlm。
        if (doit) return false;
    }
    
    return true;
}
```
&emsp;`__CFRunLoopModeIsEmpty` 函数内部主要用于判断 souces/timers 是否为空，同时还有判断 rl  的 block 链表中包含的 block 是否能在指定的 rlm 下执行。（其中 block 链表的知识点我们后面会详细接触分析）

&emsp;`__CFRunLoopPushPerRunData` 和 `__CFRunLoopPopPerRunData` 函数我们前面已经看过了，这里不再重复展开了。
##### pthread_main_np()
&emsp;`pthread_main_np` 是一个宏定义，它最终是调用 `_NS_pthread_main_np` 函数，判断当前线程是否是主线程。（主线程全局只有一条，应该是一个全局变量）
```c++
#define pthread_main_np _NS_pthread_main_np

static pthread_t __initialPthread = { NULL, 0 };
CF_EXPORT int _NS_pthread_main_np() {
    // 取得当前线程
    pthread_t me = pthread_self();
    
    // __initialPthread 是一个静态全局变量，
    // 此函数第一次调用应该是在主线程里调用，然后给 __initialPthread 赋值以后，__initialPthread 就固定表示主线程了。
    if (NULL == __initialPthread.p) {
        __initialPthread.p = me.p;
        __initialPthread.x = me.x;
    }
    
    // 判断线程是否相等
    return (pthread_equal(__initialPthread, me));
}
```
#### \__CFRunLoopDoObservers
&emsp;`__CFRunLoopDoObservers` 函数是一个极重要的函数，它用于回调 run loop 发生了状态变化。

&emsp;当 run loop 的状态将要（注意这里是将要、将要、将要... kCFRunLoopExit 则除外，退出回调是真的退出完成以后的回调）发生变化时，首先根据 run loop 当前的 run loop mode 的 `_observerMask` 是否包含了此状态的变化，那么就可以调用 `__CFRunLoopDoObservers` 函数执行 run loop 状态变化的回调，我们在此状态变化里面可以做很多重要的事情，后面学习 run loop 的使用场景时我们再详细学习。（这里回顾一下前面看过的 run loop 都有哪些状态变化：即将进入 run loop、即将处理 source 事件、即将处理 timer 事件、即将休眠、休眠即将结束、run loop 退出）
```c++
// CFRunLoopRunSpecific 函数内回调了 kCFRunLoopEntry 和 kCFRunLoopExit 两个状态变化
// if (currentMode->_observerMask & kCFRunLoopEntry ) __CFRunLoopDoObservers(rl, currentMode, kCFRunLoopEntry);
// if (currentMode->_observerMask & kCFRunLoopExit ) __CFRunLoopDoObservers(rl, currentMode, kCFRunLoopExit);

/* rl is locked, rlm is locked on entrance and exit */ 

/* 
 * 进入 __CFRunLoopDoObservers 函数前 rl 和 rlm 的 _lock 都已经加锁了，
 * 在 __CFRunLoopDoObservers 函数内部当需要执行回调时，会对 rl 和 rlm 进行解锁。
 * 然后在回调函数执行完成后，在 __CFRunLoopDoObservers 函数即将返回之前会重新对 rl 和 rlm 进行加锁。
 */

// 声明
static void __CFRunLoopDoObservers() __attribute__((noinline));
// 实现
static void __CFRunLoopDoObservers(CFRunLoopRef rl, CFRunLoopModeRef rlm, CFRunLoopActivity activity) {    /* DOES CALLOUT */
    CHECK_FOR_FORK();

    // 取出 rlm 的 _observers 数组中的元素数量
    CFIndex cnt = rlm->_observers ? CFArrayGetCount(rlm->_observers) : 0;
    
    // 如果 run loop observer 数量小于 1，则直接返回
    if (cnt < 1) return;

    /* Fire the observers */
    
    // #define STACK_BUFFER_DECL(T, N, C) T N[C]
    // CFRunLoopObserverRef buffer[cnt]，即申请一个长度是 cnt/1 的 CFRunLoopObserverRef 数组 
    STACK_BUFFER_DECL(CFRunLoopObserverRef, buffer, (cnt <= 1024) ? cnt : 1);
    
    // 如果 cnt 小于等于 1024，则 collectedObservers 是一个 CFRunLoopObserverRef buffer[cnt]，
    // 否则 collectedObservers = (CFRunLoopObserverRef *)malloc(cnt * sizeof(CFRunLoopObserverRef))。
    CFRunLoopObserverRef *collectedObservers = (cnt <= 1024) ? buffer : (CFRunLoopObserverRef *)malloc(cnt * sizeof(CFRunLoopObserverRef));
    
    // obs_cnt 用于记录 collectedObservers 收集了多少个 CFRunLoopObserverRef
    CFIndex obs_cnt = 0;
    
    // 遍历 rlm 的 _observers，把能触发的 CFRunLoopObserverRef 都收集在 collectedObservers 中。
    for (CFIndex idx = 0; idx < cnt; idx++) {
        // 取出 rlm 的 _observers 中指定下标的 CFRunLoopObserverRef
        CFRunLoopObserverRef rlo = (CFRunLoopObserverRef)CFArrayGetValueAtIndex(rlm->_observers, idx);
        
        // 1. 判断 rlo 观察的状态 _activities 中包含入参 activity
        // 2. rlo 是有效的
        // 3. rlo 的 _cfinfo 字段中的位，当前不是正在执行回调的状态
        // 同时满足上面三个条件时，把 rlo 收集在 collectedObservers 数组中，用了 CFRetain(rlo)，即 collectedObservers 持有 rlo
        
        if (0 != (rlo->_activities & activity) && __CFIsValid(rlo) && !__CFRunLoopObserverIsFiring(rlo)) {
            collectedObservers[obs_cnt++] = (CFRunLoopObserverRef)CFRetain(rlo);
        }
    }
    
    // 执行 run loop observer 的回调函数前，需要把 rlm 和 rl 解锁
    __CFRunLoopModeUnlock(rlm);
    __CFRunLoopUnlock(rl);
    
    // 遍历 collectedObservers 执行每个 CFRunLoopObserverRef 的回调函数
    for (CFIndex idx = 0; idx < obs_cnt; idx++) {
        // 根据下标取出 CFRunLoopObserverRef
        CFRunLoopObserverRef rlo = collectedObservers[idx];
        
        // CFRunLoopObserver 加锁
        __CFRunLoopObserverLock(rlo);
        
        // 如果 rlo 是有效的，则进入 if 执行回调，否则 rlo 解锁，进入下次循环
        if (__CFIsValid(rlo)) {
        
            // 取出 rlo 是否重复观察 run loop 的状态变化的标记
            Boolean doInvalidate = !__CFRunLoopObserverRepeats(rlo);
            
            // 设置 rlo 的为正在执行状态
            __CFRunLoopObserverSetFiring(rlo);
            
            // CFRunLoopObserver 解锁
            __CFRunLoopObserverUnlock(rlo);
            
            // 执行回调函数，函数名超长，而且都是大写，其中的 OBSERVER 标记这是一个 rlo 的回调，
            // 不过其内部实现很简单，就是带着参数调用 rlo 的 _callout 函数
            __CFRUNLOOP_IS_CALLING_OUT_TO_AN_OBSERVER_CALLBACK_FUNCTION__(rlo->_callout,
                                                                          rlo,
                                                                          activity,
                                                                          rlo->_context.info);
            
            // 如果 rlo 是仅观察 run loop 状态变化一次的话，此时观察完毕了，则需要把 rlo 作废，
            //（CFRunLoopObserverInvalidate 内部有有一系列的 rlo 的成员变量的释放操作）
            if (doInvalidate) {
                CFRunLoopObserverInvalidate(rlo);
            }
            
            // 设置 rlo 的已经结束正在执行状态
            __CFRunLoopObserverUnsetFiring(rlo);
        } else {
            // CFRunLoopObserver 解锁
            __CFRunLoopObserverUnlock(rlo);
        }
        
        // 释放 rlo，这里的释放对应了上面收集时的 CFRetain 
        CFRelease(rlo);
    }
    
    // 执行完 run loop observer 的回调函数后，需要再把 rlm 和 rl 加锁
    __CFRunLoopLock(rl);
    __CFRunLoopModeLock(rlm);

    // 如果 collectedObservers 是调用 malloc 申请的，则调用 free 释放其内存空间
    if (collectedObservers != buffer) free(collectedObservers);
}
```
&emsp;run loop observer 的回调函数。
```c++
static void __CFRUNLOOP_IS_CALLING_OUT_TO_AN_OBSERVER_CALLBACK_FUNCTION__() __attribute__((noinline));
static void __CFRUNLOOP_IS_CALLING_OUT_TO_AN_OBSERVER_CALLBACK_FUNCTION__(CFRunLoopObserverCallBack func,
                                                                          CFRunLoopObserverRef observer,
                                                                          CFRunLoopActivity activity,
                                                                          void *info) {
    // 就是简单的带着参数调用 func 函数                                                                      
    if (func) {
        func(observer, activity, info);
    }
    
    asm __volatile__(""); // thwart tail-call optimization
}
```
&emsp;`__CFRunLoopDoObservers` 函数至此就分析完毕了，注释已经极其清晰了，这里就不总结了。

&emsp;现在 `CFRunLoopRunSpecific` 函数内部调用的其它函数就只剩下 `__CFRunLoopRun` 函数了...超长...!
### \__CFRunLoopRun
&emsp;`__CFRunLoopRun` 函数是 run loop 真正的运行函数，超长（并且里面包含了一些在 windows 平台下的代码）。因为其是 run loop 最最核心的函数，下面我们就一行一行看一下吧。
```c++
/* rl, rlm are locked on entrance and exit */
// 同上面的 __CFRunLoopDoObservers 函数

/* 
* 进入 __CFRunLoopRun 函数前 rl 和 rlm 的 _lock 都已经加锁了，
* 在 __CFRunLoopRun 函数内部当需要执行回调时，会对 rl 和 rlm 进行解锁，
* 然后在回调函数执行完成后，会重新对 rl 和 rlm 进行加锁。
*/
static int32_t __CFRunLoopRun(CFRunLoopRef rl,
                              CFRunLoopModeRef rlm,
                              CFTimeInterval seconds,
                              Boolean stopAfterHandle,
                              CFRunLoopModeRef previousMode) {
                              
    // mach_absolute_time 返回一个基于系统启动后的时钟嘀嗒数，是一个 CPU/总线 依赖函数。
    // 在 macOS 上可以确保它的行为，并且它包含系统时钟所拥有的全部时间区域，精度达到纳秒级。
    // 时钟嘀嗒数在每次手机重启后，都会重新开始计数，而且 iPhone 锁屏进入休眠之后，tick 也会暂停计数
    uint64_t startTSR = mach_absolute_time();
    
    // 判断 rl 是否已停止，(rl->_perRunData->stopped) ? true : false;
    // rl->_perRunData->stopped 的值为 0x53544F50/0x0，
    // 当值是 0x53544F50 时表示 rl 已经停止，是 0x0 时表示非停止。
    
    // 如果 rl 是 stop 标记，则把它置为非 stop，然后返回 kCFRunLoopRunStopped，
    // 如果 rlm 是 stop 标记，则把它置为非 stop，然后返回 kCFRunLoopRunStopped。
    //（这里把 rl 和 rlm 的 stop 状态置为相反的状态，应该是一个伏笔）
    
    if (__CFRunLoopIsStopped(rl)) {
        // 设置 rl->_perRunData->stopped = 0x0 表示未设置停止标记的状态，即表示 rl 是非停止状态。
        __CFRunLoopUnsetStopped(rl);
        
        // 然后直接返回 kCFRunLoopRunStopped
        return kCFRunLoopRunStopped;
    } else if (rlm->_stopped) { // 判断 rlm 的 _stopped 是否标记为 true
        // 如果 _stopped 是 true，则把 _stopped 置为 false，表示 flm 是非停止状态。
        rlm->_stopped = false;
        
        // 然后直接返回 kCFRunLoopRunStopped
        return kCFRunLoopRunStopped;
    }
    
    // 声明一个 mach_port_name_t 类型的局部变量 dispatchPort
    mach_port_name_t dispatchPort = MACH_PORT_NULL;
    
    // #define HANDLE_DISPATCH_ON_BASE_INVOCATION_ONLY 0
    // 当前是主线程并且从当前线程的 TSD 中获取 __CFTSDKeyIsInGCDMainQ 得到的是 0 的话 libdispatchQSafe 的值为 true
    Boolean libdispatchQSafe = pthread_main_np() &&
                               ((HANDLE_DISPATCH_ON_BASE_INVOCATION_ONLY && NULL == previousMode) ||
                               (!HANDLE_DISPATCH_ON_BASE_INVOCATION_ONLY && 0 == _CFGetTSD(__CFTSDKeyIsInGCDMainQ)));
                               
    // 1. libdispatchQSafe 为真 2. 入参 rl 是 main run loop 3. 入参 rlm->_name 被 rl->_commonModes 包含
    // 以上三个条件都是真的话，则把主线程主队列的端口号赋值给 dispatchPort 变量。
    
    if (libdispatchQSafe && (CFRunLoopGetMain() == rl) && CFSetContainsValue(rl->_commonModes, rlm->_name)) 
        // _dispatch_get_main_queue_port_4CF 用于获取主线程主队列的端口号，然后赋值给 dispatchPort
        dispatchPort = _dispatch_get_main_queue_port_4CF();
   
#if USE_DISPATCH_SOURCE_FOR_TIMERS
    // 如果 rlm 中可使用 dispatch_source 构建的 timer
    mach_port_name_t modeQueuePort = MACH_PORT_NULL;
    
    // run loop mode 创建时，会对 _queue 字段赋初值
    // rlm->_queue = _dispatch_runloop_root_queue_create_4CF("Run Loop Mode Queue", 0);
    
    if (rlm->_queue) {
        // 获取 rlm->_queue 的 port 
        modeQueuePort = _dispatch_runloop_root_queue_get_port_4CF(rlm->_queue);
        
        if (!modeQueuePort) {
            // 如果获取端口失败，则 carsh 描述信息是：无法获取运行循环模式队列的端口
            CRASH("Unable to get port for run loop mode queue (%d)", -1);
        }
    }
#endif
    // GCD timer 是依赖于内核的，所以非常精准，不受 run loop 影响。
    
    // 由 dispatch_suorce 构建计时器
    dispatch_source_t timeout_timer = NULL;
    
    // struct __timeout_context {
    //     dispatch_source_t ds;
    //     CFRunLoopRef rl;
    //     uint64_t termTSR;
    // };
    
    // 为计时器参数 timeout_context 申请内存空间
    struct __timeout_context *timeout_context = (struct __timeout_context *)malloc(sizeof(*timeout_context));
    
    if (seconds <= 0.0) { // instant timeout 立即超时
        // 如果 run loop 运行时间 seconds 参数小于等于 0.0，则立即超时
        seconds = 0.0;
        timeout_context->termTSR = 0ULL;
    } else if (seconds <= TIMER_INTERVAL_LIMIT) { // 我们自己输入的小于 504911232.0 的 run loop 运行时间，其它情况的 seconds 的话都表示永不超时
        // #define TIMER_INTERVAL_LIMIT   504911232.0
        // 如果 run loop 运行时间 seconds 参数小于等于 504911232.0
        
        // 如果当前是主线程，则 queue = dispatch_get_global_queue(qos_class_main(), DISPATCH_QUEUE_OVERCOMMIT)，
        // 否则 queue = dispatch_get_global_queue(QOS_CLASS_UTILITY, DISPATCH_QUEUE_OVERCOMMIT)。
        // 当 DEPLOYMENT_TARGET_IPHONESIMULATOR 下运行时，#define qos_class_main() (QOS_CLASS_UTILITY)，
        // 即不管当前是主线程还是子线程，queue 都表示是一个全局并发队列。
        // 这个队列主要用来执行 run loop 的休眠的计时器用的，所以理论上只要是一个主队列之外的并发队列即可。
        
        dispatch_queue_t queue = pthread_main_np() ? __CFDispatchQueueGetGenericMatchingMain() : __CFDispatchQueueGetGenericBackground();
        
        // 指定 dispatch_source 为 DISPATCH_SOURCE_TYPE_TIMER 类型，即构建一个计时器类型的 dispatch_source，赋值给 timeout_timer
        timeout_timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue);
        
        // retain timeout_timer 计时器
        dispatch_retain(timeout_timer);
        
        // 设置 __timeout_context 的三个成员变量
        //（在 __CFRunLoopTimeoutCancel 回调函数中 __timeout_context 以及其 rl、ds 成员变量会进行释放）
        
        // ds 是 dispatch_source 的首字母缩写，
        //（timer 的回调参数 timeout_context 会携带 timeout_timer 计时器本身）
        timeout_context->ds = timeout_timer; 
        
        // timeout_context 持有 rl，
        //（timer 的回调参数 timeout_context 会携带 timeout_timer 计时器所处的 run loop）
        timeout_context->rl = (CFRunLoopRef)CFRetain(rl);
        
        // termTSR 是当前时间加上 run loop 运行时间的参数 seconds
        timeout_context->termTSR = startTSR + __CFTimeIntervalToTSR(seconds);
        
        // 设置 timeout_timer 计数器的上下文，即计时器回调函数的参数
        dispatch_set_context(timeout_timer, timeout_context); // source gets ownership of context
        
        // 设置 timeout_timer 计时器的执行的回调函数 __CFRunLoopTimeout
        // __CFRunLoopTimeout 函数内部会调用 CFRunLoopWakeUp(context->rl) 用于唤醒 run loop
        dispatch_source_set_event_handler_f(timeout_timer, __CFRunLoopTimeout);
        
        // 设置 timeout_timer 计时器取消时的回调函数，对 timeout_timer 调用 dispatch_source_cancel 函数后，会触发此回调
        dispatch_source_set_cancel_handler_f(timeout_timer, __CFRunLoopTimeoutCancel);
        
        // 换算秒数
        // * 1000000000ULL 是把纳秒转化为秒
        uint64_t ns_at = (uint64_t)((__CFTSRToTimeInterval(startTSR) + seconds) * 1000000000ULL);
        
        // 计时器 timeout_timer 的执行时间间隔是 DISPATCH_TIME_FOREVER，第一次触发时间是 dispatch_time(1, ns_at) 后
        //（时间间隔为 DISPATCH_TIME_FOREVER，因此不会再次触发）
        dispatch_source_set_timer(timeout_timer, dispatch_time(1, ns_at), DISPATCH_TIME_FOREVER, 1000ULL);
        
        // dispatch_resume 恢复调度对象上块的调用，这里的作用是启动 timeout_timer 计时器
        dispatch_resume(timeout_timer);
        
        // 综上计时器 timeout_timer 是用来为 run loop 运行超时计时用的，当运行了 dispatch_time(1, ns_at) 后会触发此计时器执行
    } else { // infinite timeout 无效超时，永不超时
        seconds = 9999999999.0;
        timeout_context->termTSR = UINT64_MAX;
    }
    
    Boolean didDispatchPortLastTime = true;
    // 返回值
    int32_t retVal = 0;
    
    // 进入这个外层 do while 循环，这个 do while 循环超长几乎包含了剩下的所有函数内容，
    // 中间还嵌套了一个较短的 do while 循环用于处理 run loop 的休眠和唤醒。
    do {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
        // macOS 下
        // 
        voucher_mach_msg_state_t voucherState = VOUCHER_MACH_MSG_STATE_UNCHANGED;
        voucher_t voucherCopy = NULL;
#endif
        // 3072
        uint8_t msg_buffer[3 * 1024];
        
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
        // macOS 下，申请两个局部变量 msg 和 livePort
        mach_msg_header_t *msg = NULL;
        mach_port_t livePort = MACH_PORT_NULL;
#elif DEPLOYMENT_TARGET_WINDOWS
        HANDLE livePort = NULL;
        Boolean windowsMessageReceived = false;
#endif
        // 取得 rlm 的端口集合 _portSet
        __CFPortSet waitSet = rlm->_portSet;
        
        // 设置 rl->_perRunData->ignoreWakeUps = 0x0，表示未设置 IgnoreWakeUps 标记位，
        // rl->_perRunData->ignoreWakeUps = 0x57414B45/0x0，
        // 当值是 0x57414B45 时表示忽略唤醒（IgnoreWakeUps） 
        __CFRunLoopUnsetIgnoreWakeUps(rl);
        
        // 下面连续回调了 rl 的两个状态变化，kCFRunLoopBeforeTimers 和 kCFRunLoopBeforeSources
        //（__CFRunLoopDoObservers 内部会对 rl rlm 解锁执行回调事件，然后执行结束再给 rl rlm 加锁）
        
        if (rlm->_observerMask & kCFRunLoopBeforeTimers) __CFRunLoopDoObservers(rl, rlm, kCFRunLoopBeforeTimers);
        if (rlm->_observerMask & kCFRunLoopBeforeSources) __CFRunLoopDoObservers(rl, rlm, kCFRunLoopBeforeSources);
        
        // 遍历 rl 的 block 链表中的指定在 rlm 下的 block 执行，
        // 会首先把 rl 的 _blocks_head 和 _blocks_tail 置为 NULL，然后每个 block 执行完毕后会调用 Block_release 函数。
        //（block 执行时调用的是 __CFRUNLOOP_IS_CALLING_OUT_TO_A_BLOCK__ 函数）
        //（我们开始收集这种名字大写的函数，在 run loop 学习过程中我们会遇到多个这种命名方式的函数，当我们都收集完了，那么 run loop 的学习就很熟悉了）
        
        // 目前我们收集到两个：
        // __CFRUNLOOP_IS_CALLING_OUT_TO_AN_OBSERVER_CALLBACK_FUNCTION__
        // __CFRUNLOOP_IS_CALLING_OUT_TO_A_BLOCK__
        
        __CFRunLoopDoBlocks(rl, rlm);
        
        // 处理 rlm 的 _sources0 中的所的 source
        
        // 目前我们收集到三个：
        // __CFRUNLOOP_IS_CALLING_OUT_TO_AN_OBSERVER_CALLBACK_FUNCTION__
        // __CFRUNLOOP_IS_CALLING_OUT_TO_A_BLOCK__
        // __CFRUNLOOP_IS_CALLING_OUT_TO_A_SOURCE0_PERFORM_FUNCTION__
        
        Boolean sourceHandledThisLoop = __CFRunLoopDoSources0(rl, rlm, stopAfterHandle);
        
        // sourceHandledThisLoop 的值表示在 __CFRunLoopDoSources0 函数内部是否对 rlm 的 _sources0 中的 CFRunLoopSourceRef 进行了处理，
        // void (*perform)(void *info) 函数指针指向 _source0 要执行的任务块，当 _source0 事件被触发时的回调
        
        // 如果为真则遍历 rl 的 block 链表中的指定在 rlm 下的 block 执行，
        // 会首先把 rl 的 _blocks_head 和 _blocks_tail 置为 NULL，然后每个 block 执行完毕后会调用 Block_release 函数。
        if (sourceHandledThisLoop) {
            __CFRunLoopDoBlocks(rl, rlm);
        }
        
        // 如果 rlm 的 _sources0 中有 CFRunLoopSourceRef 进行了处理或者 timeout_context->termTSR 等于 0，则 poll 的值为 true 否则为 false
        Boolean poll = sourceHandledThisLoop || (0ULL == timeout_context->termTSR);
        
        // 如果当前是主线程并且 dispatchPort 不为空且 didDispatchPortLastTime 为 false
        if (MACH_PORT_NULL != dispatchPort && !didDispatchPortLastTime) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
            // macOS 下执行
            msg = (mach_msg_header_t *)msg_buffer;
            if (__CFRunLoopServiceMachPort(dispatchPort,
                                           &msg,
                                           sizeof(msg_buffer),
                                           &livePort,
                                           0,
                                           &voucherState,
                                           NULL)) {
                // 执行 handle_msg
                goto handle_msg;
            }
#elif DEPLOYMENT_TARGET_WINDOWS
            if (__CFRunLoopWaitForMultipleObjects(NULL, &dispatchPort, 0, 0, &livePort, NULL)) {
                goto handle_msg;
            }
#endif
        }
        
        // didDispatchPortLastTime 置为 false
        didDispatchPortLastTime = false;
        
        // 若需要 poll 为假，（则需要上面 sourceHandledThisLoop 为假即 rlm 的 _sources0 中没有 source 需要处理且 0ULL == timeout_context->termTSR）
        // 则调用 __CFRunLoopDoObservers 函数回调 rl 切换到 kCFRunLoopBeforeWaiting，即 rl 即将进入休眠状态
        if (!poll && (rlm->_observerMask & kCFRunLoopBeforeWaiting)) __CFRunLoopDoObservers(rl, rlm, kCFRunLoopBeforeWaiting);
        
        // 设置 __CFBitfieldSetValue(((CFRuntimeBase *)rl)->_cfinfo[CF_INFO_BITS], 1, 1, 1)，标记 rl 进入休眠状态
        __CFRunLoopSetSleeping(rl);
        
        // do not do any user callouts after this point (after notifying of sleeping)
        // 在此之后（通知睡眠之后）不进行任何用户标注
        
        // Must push the local-to-this-activation ports in on every loop iteration, 
        // as this mode could be run re-entrantly and we don't want these ports to get serviced.
        // 必须在每次循环迭代中都将 local-to-this-activation 端口推入，因为此模式可以重新进入运行，我们不希望为这些端口提供服务。
        
        // 把 dispatchPort 插入到 rlm 的 _portSet 中（waitSet）
        __CFPortSetInsert(dispatchPort, waitSet);
        
        // CFRunLoopMode 解锁
        __CFRunLoopModeUnlock(rlm);
        // CFRunLoop 解锁
        __CFRunLoopUnlock(rl);
        
        // sleepStart 用于记录睡眠开始的时间
        CFAbsoluteTime sleepStart = poll ? 0.0 : CFAbsoluteTimeGetCurrent();
        
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
        // 在 macOS 下
#if USE_DISPATCH_SOURCE_FOR_TIMERS
        // 如果 rlm 使用 dispatch_source 构建的计时器
        
        // 这个内层的 do while 循环主要是用于 "阻塞" rl 的睡眠状态的，直到需要被唤醒了才会跳出这个 do while 循环，
        // 
        //
        //
        // 
        do {
            if (kCFUseCollectableAllocator) {
                // objc_clear_stack(0);
                // <rdar://problem/16393959>
                
                // 把以 msg_buffer 为起点长度为 sizeof(msg_buffer) 的内存置为 0
                memset(msg_buffer, 0, sizeof(msg_buffer));
            }
            
            // 
            msg = (mach_msg_header_t *)msg_buffer;
            
            // MachPort
            __CFRunLoopServiceMachPort(waitSet,
                                       &msg,
                                       sizeof(msg_buffer),
                                       &livePort,
                                       poll ? 0 : TIMEOUT_INFINITY,
                                       &voucherState,
                                       &voucherCopy);
            
            // modeQueuePort = _dispatch_runloop_root_queue_get_port_4CF(rlm->_queue) 来自于 rlm 的 _queue 队列端口
            if (modeQueuePort != MACH_PORT_NULL && livePort == modeQueuePort) {
                // Drain the internal queue. If one of the callout blocks sets the timerFired flag, break out and service the timer.
                // 清空内部队列。如果其中一个标注块设置了timerFired标志，请中断并为计时器提供服务。
                
                while (_dispatch_runloop_root_queue_perform_4CF(rlm->_queue));
                
                // _timerFired 首先赋值为 false，然后在 timer 的回调函数执行的时候会赋值为 true
                // rlm->_timerFired = false;
                // 当 _timerSource（计时器）回调时会执行这个 block，block 内部是把 _timerFired 修改为 true
                // __block Boolean *timerFiredPointer = &(rlm->_timerFired);
                // dispatch_source_set_event_handler(rlm->_timerSource, ^{
                //     *timerFiredPointer = true;
                // });
            
                if (rlm->_timerFired) {
                    // Leave livePort as the queue port, and service timers below
                    // 将 livePort 保留为队列端口，并在下面保留服务计时器。 
                    
                    // rlm 的 _timerSource 计时器回调后 run loop 会结束休眠
                    rlm->_timerFired = false;
                    break;
                } else {
                    if (msg && msg != (mach_msg_header_t *)msg_buffer) free(msg);
                }
            } else {
                // Go ahead and leave the inner loop.
                // 继续并离开内循环。
                
                break;
            }
        } while (1);
#else
        if (kCFUseCollectableAllocator) {
            // objc_clear_stack(0);
            // <rdar://problem/16393959>
            memset(msg_buffer, 0, sizeof(msg_buffer));
        }
        msg = (mach_msg_header_t *)msg_buffer;
        __CFRunLoopServiceMachPort(waitSet,
                                   &msg,
                                   sizeof(msg_buffer),
                                   &livePort,
                                   poll ? 0 : TIMEOUT_INFINITY,
                                   &voucherState,
                                   &voucherCopy);
#endif
        
#elif DEPLOYMENT_TARGET_WINDOWS
        // Here, use the app-supplied message queue mask. They will set this if they are interested in having this run loop receive windows messages.
        __CFRunLoopWaitForMultipleObjects(waitSet,
                                          NULL,
                                          poll ? 0 : TIMEOUT_INFINITY,
                                          rlm->_msgQMask,
                                          &livePort,
                                          &windowsMessageReceived);
#endif
        
        // CFRunLoop 加锁
        __CFRunLoopLock(rl);
        // CFRunLoopMode 加锁
        __CFRunLoopModeLock(rlm);
        
        // 统计 rl 的休眠时间，CFAbsoluteTimeGetCurrent() 当前时间减去 sleepStart
        rl->_sleepTime += (poll ? 0.0 : (CFAbsoluteTimeGetCurrent() - sleepStart));
        
        // Must remove the local-to-this-activation ports in on every loop iteration, 
        // as this mode could be run re-entrantly and we don't want these ports to get serviced. 
        // Also, we don't want them left in there if this function returns.
        // 必须在每次循环迭代中都删除本地激活端口，因为此模式可以重新进入，并且我们不希望为这些端口提供服务。另外，如果此函数返回，我们不希望它们留在那里。
        
        // 从 waitSet 中移除 dispatchPort 
        __CFPortSetRemove(dispatchPort, waitSet);
        
        // 设置 rl 忽略唤醒
        // rl->_perRunData->ignoreWakeUps = 0x57414B45; // 'WAKE'
        __CFRunLoopSetIgnoreWakeUps(rl);
        
        // user callouts now OK again
        // __CFBitfieldSetValue(((CFRuntimeBase *)rl)->_cfinfo[CF_INFO_BITS], 1, 1, 0);
        // 标记 rl 为非休眠状态
        __CFRunLoopUnsetSleeping(rl);
        
        // 调用 __CFRunLoopDoObservers 函数，回调 rl 切换到 kCFRunLoopAfterWaiting 状态了 
        if (!poll && (rlm->_observerMask & kCFRunLoopAfterWaiting)) __CFRunLoopDoObservers(rl, rlm, kCFRunLoopAfterWaiting);
        
    handle_msg:;
        // rl->_perRunData->ignoreWakeUps = 0x57414B45
        // 设置 rl 忽略唤醒
        __CFRunLoopSetIgnoreWakeUps(rl);
        
        // 一大段 windows 平台下的代码，可忽略
#if DEPLOYMENT_TARGET_WINDOWS
        if (windowsMessageReceived) {
            // These Win32 APIs cause a callout, so make sure we're unlocked first and relocked after
            __CFRunLoopModeUnlock(rlm);
            __CFRunLoopUnlock(rl);
            
            if (rlm->_msgPump) {
                rlm->_msgPump();
            } else {
                MSG msg;
                if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE | PM_NOYIELD)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
            
            __CFRunLoopLock(rl);
            __CFRunLoopModeLock(rlm);
            sourceHandledThisLoop = true;
            
            // To prevent starvation of sources other than the message queue, we check again to see if any other sources need to be serviced
            // Use 0 for the mask so windows messages are ignored this time. Also use 0 for the timeout, because we're just checking to see if the things are signalled right now -- we will wait on them again later.
            // NOTE: Ignore the dispatch source (it's not in the wait set anymore) and also don't run the observers here since we are polling.
            __CFRunLoopSetSleeping(rl);
            __CFRunLoopModeUnlock(rlm);
            __CFRunLoopUnlock(rl);
            
            __CFRunLoopWaitForMultipleObjects(waitSet, NULL, 0, 0, &livePort, NULL);
            
            __CFRunLoopLock(rl);
            __CFRunLoopModeLock(rlm);            
            __CFRunLoopUnsetSleeping(rl);
            // If we have a new live port then it will be handled below as normal
        }
#endif

        if (MACH_PORT_NULL == livePort) {
            // #define CFRUNLOOP_WAKEUP_FOR_NOTHING() do { } while (0)
            // 如果 livePort 为 NULL，什么也不做
            CFRUNLOOP_WAKEUP_FOR_NOTHING();
            
            // handle nothing
        } else if (livePort == rl->_wakeUpPort) {
            // 如果 rl 的 _wakeUpPort 为 livePort，则在 macOS 下什么也不做
            
            // #define CFRUNLOOP_WAKEUP_FOR_WAKEUP() do { } while (0)
            CFRUNLOOP_WAKEUP_FOR_WAKEUP();
            // do nothing on Mac OS
            
            // windows 平台下
#if DEPLOYMENT_TARGET_WINDOWS
            // Always reset the wake up port, or risk spinning forever
            ResetEvent(rl->_wakeUpPort);
#endif
        }
        // 如果计时器是使用 dispatch_source 实现的
#if USE_DISPATCH_SOURCE_FOR_TIMERS
        else if (modeQueuePort != MACH_PORT_NULL && livePort == modeQueuePort) {
            // 如果 modeQueuePort 不为 NULL，且 modeQueuePort 等于 livePort
            // #define CFRUNLOOP_WAKEUP_FOR_TIMER() do { } while (0)
            CFRUNLOOP_WAKEUP_FOR_TIMER();
            
            if (!__CFRunLoopDoTimers(rl, rlm, mach_absolute_time())) {
                // Re-arm the next timer, because we apparently fired early
                // 执行 rlm 的 _timers 中的 CFRunLoopTimerRef。
                
                // 目前我们收集到四个：
                // __CFRUNLOOP_IS_CALLING_OUT_TO_AN_OBSERVER_CALLBACK_FUNCTION__
                // __CFRUNLOOP_IS_CALLING_OUT_TO_A_BLOCK__
                // __CFRUNLOOP_IS_CALLING_OUT_TO_A_SOURCE0_PERFORM_FUNCTION__
                // __CFRUNLOOP_IS_CALLING_OUT_TO_A_TIMER_CALLBACK_FUNCTION__
                
                __CFArmNextTimerInMode(rlm, rl);
            }
        }
#endif

#if USE_MK_TIMER_TOO
        else if (rlm->_timerPort != MACH_PORT_NULL && livePort == rlm->_timerPort) {
            // 如果计时器是使用 MK 实现的
            
            CFRUNLOOP_WAKEUP_FOR_TIMER();
            // On Windows, we have observed an issue where the timer port is set before the time which we requested it to be set. For example, we set the fire time to be TSR 167646765860, but it is actually observed firing at TSR 167646764145, which is 1715 ticks early. The result is that, when __CFRunLoopDoTimers checks to see if any of the run loop timers should be firing, it appears to be 'too early' for the next timer, and no timers are handled.
            // 在 Windows 上，我们发现了一个问题，即在我们要求设置定时器端口之前设置了定时器端口。例如，我们将开火时间设置为 TSR 167646765860，但实际上可以观察到以 TSR 167646764145 开火，这是提早 1715 滴答。结果是，当 __CFRunLoopDoTimers 检查是否应触发任何运行循环计时器时，下一个计时器似乎为时过早，并且不处理任何计时器。
            
            // In this case, the timer port has been automatically reset (since it was returned from MsgWaitForMultipleObjectsEx), and if we do not re-arm it, then no timers will ever be serviced again unless something adjusts the timer list (e.g. adding or removing timers). The fix for the issue is to reset the timer here if CFRunLoopDoTimers did not handle a timer itself. 9308754
            // 在 Windows 上，我们发现了一个问题，即在我们要求设置定时器端口之前设置了定时器端口。例如，我们将开火时间设置为 TSR 167646765860，但实际上可以观察到以 TSR 167646764145开火，这是提早 1715 滴答。结果是，当 __CFRunLoopDoTimers 检查是否应触发任何运行循环计时器时，下一个计时器似乎为时过早，并且不处理任何计时器。
            if (!__CFRunLoopDoTimers(rl, rlm, mach_absolute_time())) {
                // Re-arm the next timer
                __CFArmNextTimerInMode(rlm, rl);
            }
        }
#endif
        else if (livePort == dispatchPort) {
            // 如果 dispatchPort 等于 livePort
            
            // #define   CFRUNLOOP_WAKEUP_FOR_DISPATCH() do { } while (0)
            CFRUNLOOP_WAKEUP_FOR_DISPATCH();
            
            // CFRunLoopMode 解锁
            __CFRunLoopModeUnlock(rlm);
            // CFRunLoop 解锁
            __CFRunLoopUnlock(rl);
            
            // 设置 TSD 中的 __CFTSDKeyIsInGCDMainQ
            _CFSetTSD(__CFTSDKeyIsInGCDMainQ, (void *)6, NULL);
            
#if DEPLOYMENT_TARGET_WINDOWS
            void *msg = 0;
#endif

            // 目前我们收集到五个：
            // __CFRUNLOOP_IS_CALLING_OUT_TO_AN_OBSERVER_CALLBACK_FUNCTION__
            // __CFRUNLOOP_IS_CALLING_OUT_TO_A_BLOCK__
            // __CFRUNLOOP_IS_CALLING_OUT_TO_A_SOURCE0_PERFORM_FUNCTION__
            // __CFRUNLOOP_IS_CALLING_OUT_TO_A_TIMER_CALLBACK_FUNCTION__
            // __CFRUNLOOP_IS_SERVICING_THE_MAIN_DISPATCH_QUEUE__
            
            // 主队类回调事件
            __CFRUNLOOP_IS_SERVICING_THE_MAIN_DISPATCH_QUEUE__(msg);
            
            // 设置 TSD 中的 __CFTSDKeyIsInGCDMainQ
            _CFSetTSD(__CFTSDKeyIsInGCDMainQ, (void *)0, NULL);
            
            // CFRunLoop 加锁
            __CFRunLoopLock(rl);
            // CFRunLoopMode 加锁
            __CFRunLoopModeLock(rlm);
            
            sourceHandledThisLoop = true;
            didDispatchPortLastTime = true;
        } else {
            CFRUNLOOP_WAKEUP_FOR_SOURCE();
            
            // If we received a voucher from this mach_msg, then put a copy of the new voucher into TSD. 
            // CFMachPortBoost will look in the TSD for the voucher. 
            // By using the value in the TSD we tie the CFMachPortBoost to this received mach_msg explicitly 
            // without a chance for anything in between the two pieces of code to set the voucher again.
            // 如果我们收到了来自此 mach_msg 的凭证，则将新凭证的副本放入 TSD。 
            // CFMachPortBoost 将在 TSD 中查找该凭证。通过使用 TSD 中的值，我们将 CFMachPortBoost 明确地绑定到此接收到的 mach_msg 上，
            // 而在这两段代码之间没有任何机会再次设置凭单。
            
            voucher_t previousVoucher = _CFSetTSD(__CFTSDKeyMachMessageHasVoucher, (void *)voucherCopy, os_release);
            
            // Despite the name, this works for windows handles as well
            
            // 从 rlm 的 _portToV1SourceMap 中，根据 livePort 找到其对应的 CFRunLoopSourceRef
            CFRunLoopSourceRef rls = __CFRunLoopModeFindSourceForMachPort(rl, rlm, livePort);
            if (rls) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
                // macOS 下
                mach_msg_header_t *reply = NULL;
                sourceHandledThisLoop = __CFRunLoopDoSource1(rl, rlm, rls, msg, msg->msgh_size, &reply) || sourceHandledThisLoop;
                if (NULL != reply) {
                    (void)mach_msg(reply, MACH_SEND_MSG, reply->msgh_size, 0, MACH_PORT_NULL, 0, MACH_PORT_NULL);
                    CFAllocatorDeallocate(kCFAllocatorSystemDefault, reply);
                }
                
#elif DEPLOYMENT_TARGET_WINDOWS
                sourceHandledThisLoop = __CFRunLoopDoSource1(rl, rlm, rls) || sourceHandledThisLoop;
#endif
            }
            
            // Restore the previous voucher
            _CFSetTSD(__CFTSDKeyMachMessageHasVoucher, previousVoucher, os_release);
            
        } 
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
        // 释放 msg 的内存空间
        if (msg && msg != (mach_msg_header_t *)msg_buffer) free(msg);
#endif
        
        // 执行 rl 的 block 链表中的 block
        __CFRunLoopDoBlocks(rl, rlm);
        
        // 
        if (sourceHandledThisLoop && stopAfterHandle) {
            // 已处理过一个源，继续处理
            retVal = kCFRunLoopRunHandledSource;
        } else if (timeout_context->termTSR < mach_absolute_time()) {
            // 超时
            retVal = kCFRunLoopRunTimedOut;
        } else if (__CFRunLoopIsStopped(rl)) {
            // 停止
            __CFRunLoopUnsetStopped(rl);
            retVal = kCFRunLoopRunStopped;
        } else if (rlm->_stopped) {
            // rlm 停止
            rlm->_stopped = false;
            retVal = kCFRunLoopRunStopped;
        } else if (__CFRunLoopModeIsEmpty(rl, rlm, previousMode)) {
            // rlm 为的  source/timer/ block 为空
            retVal = kCFRunLoopRunFinished;
        }
        
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
        // 在 macOS 下
        voucher_mach_msg_revert(voucherState);
        // 释放 voucherCopy
        os_release(voucherCopy);
#endif
        
    } while (0 == retVal); // 外层的 do while 循环结束的条件是 retVal 不等于 0 时
    
    // 释放 
    if (timeout_timer) {
        // 取消计时器，会在取消的回调函数 __CFRunLoopTimeoutCancel 里面做清理工作 
        dispatch_source_cancel(timeout_timer);
        // 释放 timeout_timer
        dispatch_release(timeout_timer);
    } else {
        // 释放 timeout_context，
        // 对应前面的 timeout_context = (struct __timeout_context *)malloc(sizeof(*timeout_context))，malloc 申请空间
        free(timeout_context);
    }
    
    return retVal;
}
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
