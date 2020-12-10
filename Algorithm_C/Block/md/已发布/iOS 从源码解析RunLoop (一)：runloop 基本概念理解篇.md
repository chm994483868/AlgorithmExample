# iOS 从源码解析Run Loop (一)：run loop 基本概念理解篇

> &emsp;Run loops 是与 threads 关联的基本基础结构的一部分。Run loop 是一个 event processing loop （**事件处理循环**），可用于计划工作并协调收到的事件的接收。Run loop 的目的是让 thread 在有工作要做时保持忙碌，而在没有工作时让 thread 进入睡眠状态。（官方解释初次看时显的过于生涩，不过我们仍然可以抓住一些关键点，原本我们的 thread 执行完任务后就要释放销毁了，但是在 run loop 的加持下，线程不再自己主动去销毁而是处于待命状态等待着我们再交给它任务，换句话说就是 run loop 使我的线程保持了活性，下面我们试图对 run loop 的概念进行理解。）

## runloop 概念
&emsp;一般来讲，一个线程一次只能执行一个任务，执行完成后线程就会退出。如果我们需要一个机制，让线程能随时处理事件但并不退出，这种模型通常被称作 Event Loop。 Event Loop 在很多系统和框架里都有实现，比如 Node.js 的事件处理，比如 Windows 程序的消息循环，再比如 OSX/iOS 里的 Run Loop。实现这种模型的关键点在于基于消息机制：管理事件/消息，让线程在没有消息需时休眠以避免资源占用、在有消息到来时立刻被唤醒执行任务。

&emsp;那什么是 run loop？顾名思义，run loop 就是在 “跑圈”，run loop 运行的核心代码是一个有状态的 **do while 循环**，每循环一次就相当于跑了一圈，线程就会对当前这一圈里面产生的事件进行处理，do while 循环我们可能已经写过无数次，当然我们日常在函数中写的都是会明确结束的循环，并且循环的内容是我们一开始就编写好的，我们并不能动态的改变或者插入循环的内容，只要不是超时或者故意退出状态下 run loop 就会一直执行 do while 循环，所以可以保证线程不退出，并且可以让我们根据自己需要向线程中添加任务。

> &emsp;那么为什么线程要有 run loop 呢？其实我们的 APP 可以理解为是靠 event 驱动的（包括 iOS 和 Android 应用）。我们触摸屏幕、网络回调等都是一个个的 event，也就是事件。这些事件产生之后会分发给我们的 APP，APP 接收到事件之后分发给对应的线程。通常情况下，如果线程没有 run loop，那么一个线程一次只能执行一个任务，执行完成后线程就会退出。要想 APP 的线程一直能够处理事件或者等待事件（比如异步事件），就要保活线程，也就是不能让线程早早的退出，此时 run loop 就派上用场了，其实也不是必须要给线程指定一个 run loop，如果需要我们的线程能够持续的处理事件，那么就需要给线程绑定一个 run loop。也就是说，run loop 能够保证线程一直可以处理事件。[一份走心的runloop源码分析](https://cloud.tencent.com/developer/article/1630860)

&emsp;通常情况下，事件并不是永无休止的产生，所以也就没必要让线程永无休止的运行，run loop 可以在无事件处理时进入休眠状态，避免无休止的 do while 跑空圈，看到这里我们注意到线程和 run loop 都是能进入休眠状态的，这里为了便于理解概念我们看一些表示 run loop 运行状态的代码：
```c++
/* Run Loop Observer Activities */
typedef CF_OPTIONS(CFOptionFlags, CFRunLoopActivity) {
    kCFRunLoopEntry = (1UL << 0), // 进入 Run Loop 循环 (这里其实还没进入)
    kCFRunLoopBeforeTimers = (1UL << 1), // Run Loop 即将开始处理 Timer
    kCFRunLoopBeforeSources = (1UL << 2), // Run Loop 即将开始处理 Source
    kCFRunLoopBeforeWaiting = (1UL << 5), // Run Loop 即将进入休眠
    kCFRunLoopAfterWaiting = (1UL << 6), // Run Loop 从休眠状态唤醒
    kCFRunLoopExit = (1UL << 7), // Run Loop 退出（和 kCFRunLoop Entry 对应）
    kCFRunLoopAllActivities = 0x0FFFFFFFU
};
```
&emsp;run loop 与线程 的关系了：一个线程对应一个 run loop，程序运行是主线程的 main run loop 默认启动了，所以我们的程序才不会退出，子线程的 run loop 按需启动（调用 run 方法）。run loop 是线程的事件管理者，或者说是线程的事件管家，它会按照顺序管理线程要处理的事件，决定哪些事件在什么时候提交给线程处理。

&emsp;看到这里我们大概也明白了 run loop 和线程大概是个怎么回事了，其实这里最想搞明白的是：run loop 是如何进行状态切换的，例如它是怎么进入休眠怎样被唤醒的？还有它和线程之间是怎么进行信息传递的？怎么让线程保持活性的？等等，搜集到的资料看到是 run loop 内部是基于内核基于 mach port 进行工作的，涉及的太深奥了，这里暂时先进行上层的学习，等我们把基础应用以及一些源码实现搞明白了再深入学习它的底层内容。⛽️⛽️

&emsp;下面我们开始从代码层面对 run loop 进行学习，而学习的主线则是 run loop 是如何作用与线程使其保持活性的？
### main run loop 启动
&emsp;前面我们学习线程时，多次提到主线程主队列都是在 app 启动时默认创建的，而恰恰主线程的 main run loop 也是在 app 启动时默认跟着创建并启动的，那么我们从 main.m 文件中找出一些端倪，使用 Xcode 创建一个 OC 语言的 Single View App 时会自动生成如下的 main.m 文件：
```c++
#import <UIKit/UIKit.h>
#import "AppDelegate.h"

int main(int argc, char * argv[]) {
    NSString * appDelegateClassName;
    @autoreleasepool {
        // Setup code that might create autoreleased objects goes here.
        appDelegateClassName = NSStringFromClass([AppDelegate class]);
        NSLog(@"🏃🏃‍♀️🏃🏃‍♀️..."); // 这里插入一行打印语句
    }
    return UIApplicationMain(argc, argv, nil, appDelegateClassName);
    // return 0;
    
    // 把上面的 return UIApplicationMain(argc, argv, nil, appDelegateClassName); 语句拆开如下：
    // int result = UIApplicationMain(argc, argv, nil, appDelegateClassName);
    // return result; // ⬅️ 在此行打一个断点，执行程序会发现此断点是无效的，因为 main 函数根本不会执行到这里
}
```
&emsp;`main` 函数最后一行 `return` 语句是返回 `UIApplicationMain` 函数的执行结果，我们把此行注释，然后添加一行 `return 0;`，运行程序后会看到执行 `NSLog` 语句后程序就结束了直接回到了手机桌面，而最后一行是 `return UIApplicationMain(argc, argv, nil, appDelegateClassName);` 的话运行程序后就进入了 app 的首页而并不会结束程序，那么我们大概想到了这个 `UIApplicationMain` 函数是不会返回的，它不会返回，所以 `main` 函数也就不会返回了，`main` 函数不会返回，所以我们的 app 就不会自己主动结束运行回到桌面了（当然这里的函数不会返回是不同于我们线程学习时看到的线程被阻塞甚至死锁时的函数不返回）。下面看一下 `UIApplicationMain` 函数的声明，看到是一个返回值是 int 类型的函数。
```c++
UIKIT_EXTERN int UIApplicationMain(int argc, char * _Nullable argv[_Nonnull], NSString * _Nullable principalClassName, NSString * _Nullable delegateClassName);
```
> &emsp;**UIApplicationMain**
> &emsp;Creates the application object and the application delegate and sets up the event cycle.
> &emsp;
> &emsp;**Return Value**
> &emsp;Even though an integer return type is specified, this function never returns. When users exits an iOS app by pressing the Home button, the application moves to the background.
> &emsp;即使指定了整数返回类型，此函数也从不返回。当用户通过按 Home 键退出 iOS 应用时，该应用将移至后台。
> &emsp;**Discussion**
> &emsp;... It also sets up the main event loop, including the application’s run loop, and begins processing events. ... Despite the declared return type, this function never returns.
> &emsp;... 它还设置 main event loop，包括应用程序的 run loop（main run loop），并开始处理事件。... 尽管声明了返回类型，但此函数从不返回。

&emsp;在开发者文档中查看 `UIApplicationMain` 函数，摘要告诉我们 `UIApplicationMain` 函数完成：**创建应用程序对象和应用程序代理并设置 event cycle**，看到 Return Value 一项 Apple 已经明确告诉我们 `UIApplicationMain` 函数是不会返回的，并且在 Discussion 中也告诉我们 `UIApplicationMain` 函数启动了 main run loop 并开始着手为我们处理事件。

&emsp;`main` 函数是我们应用程序的启动入口，然后调用 `UIApplicationMain` 函数其内部帮我们开启了 main run loop，换个角度试图理解为何我们的应用程序不退出时，是不是可以理解为我们的应用程序自启动开始就被包裹在 main run loop 的 **do while 循环** 中呢？

&emsp;那么根据上面 `UIApplicationMain` 函数的功能以及我们对 runloop 概念的理解，大概可以书写出如下 runloop 的伪代码:
```c++
int main(int argc, char * argv[]) {
    @autoreleasepool {
        int retVal = 0;
        do {
            // 在睡眠中等待消息
            int message = sleep_and_wait();
            // 处理消息
            retVal = process_message(message);
        } while (retVal == 0);
        return 0;
    }
}
```

&emsp;添加一个`CFRunLoopGetMain` 的符号断点运行程序，然后在控制台使用 bt 命令打印线程的堆栈信息可看到在 `UIApplicationMain` 函数中启动了 main run loop。
```c++
(lldb) bt 
* thread #1, queue = 'com.apple.main-thread', stop reason = breakpoint 1.1 // ⬅️ 'com.apple.main-thread' 当前是我们的主线程
  * frame #0: 0x00000001de70a26c CoreFoundation`CFRunLoopGetMain // ⬅️ CFRunLoopGetMain 获取主线程
    frame #1: 0x000000020af6d864 UIKitCore`UIApplicationInitialize + 84 
    frame #2: 0x000000020af6ce30 UIKitCore`_UIApplicationMainPreparations + 416
    frame #3: 0x000000020af6cc04 UIKitCore`UIApplicationMain + 160 // ⬅️ UIApplicationMain 函数
    frame #4: 0x00000001008ba1ac Simple_iOS`main(argc=1, argv=0x000000016f54b8e8) at main.m:20:12 // ⬅️ main 函数
    frame #5: 0x00000001de1ce8e0 libdyld.dylib`start + 4 // ⬅️ 加载 dyld 和动态库
(lldb) 
```
### 如何对子线程进行保活
&emsp;首先对 “一般来讲，一个线程一次只能执行一个任务，执行完成后线程就会退出。” 进行证明。这里我们使用 NSThread 作为线程对象，首先创建一个继承自 NSThread 的 CommonThread 类，然后重写它的 `dealloc` 函数（之所以不直接在一个 NSThread 的分类中重写 dealloc 函数，是因为 app 内部的 NSThread 对象的创建和销毁会影响我们的观察） 。
```c++
// CommonThread 定义

// CommonThread.h 
#import <Foundation/Foundation.h>
NS_ASSUME_NONNULL_BEGIN
@interface CommonThread : NSThread
@end
NS_ASSUME_NONNULL_END

// CommonThread.m
#import "CommonThread.h"
@implementation CommonThread
- (void)dealloc {
    NSLog(@"🍀🍀🍀 %@ CommonThread dealloc...", self);
}
@end
```
&emsp;然后我们在根控制器的 viewDidLoad 函数中编写如下测试代码:
```c++
NSLog(@"🔞 START: %@", [NSThread currentThread]);
{
    CommonThread *commonThread = [[CommonThread alloc] initWithBlock:^{
        NSLog(@"🏃‍♀️🏃‍♀️ %@", [NSThread currentThread]);
    }];
    [commonThread start];
}
NSLog(@"🔞 END: %@", [NSThread currentThread]);

// 控制台打印:
🔞 START: <NSThread: 0x282801a40>{number = 1, name = main}
🔞 END: <NSThread: 0x282801a40>{number = 1, name = main}
🏃‍♀️🏃‍♀️ <CommonThread: 0x282850a80>{number = 5, name = (null)} // 子线程
🍀🍀🍀 <CommonThread: 0x282850a80>{number = 5, name = (null)} CommonThread dealloc... // commonThread 线程对象被销毁（线程退出）
```
&emsp;根据控制台打印我们可以看到在 `commonThread` 线程中的任务执行完毕后，`commonThread` 线程就被释放销毁了（线程退出）。那么下面我们试图使用 run loop 让 `commonThread` 不退出，同时为了便于观察 run loop 的退出（NSRunLoop 对象的销毁），我们添加一个 NSRunLoop 的分类并在分类中重写 `dealloc` 函数（这里之所以直接用 NSRunLoop 类的分类是因为，app 除了 main run loop 外是不会自己主动为线程开启 run loop 的，所以这里我们不用担心 app 内部的 NSRunLoop 对象对我们的影响）。那么我们在上面的代码基础上为线程添加 run loop 的获取和 run。
```c++
NSLog(@"🔞 START: %@", [NSThread currentThread]);
{
    CommonThread *commonThread = [[CommonThread alloc] initWithBlock:^{
        NSLog(@"🏃‍♀️🏃‍♀️ %@", [NSThread currentThread]);
        
        // 为当前线程获取 run loop
        NSRunLoop *commonRunLoop = [NSRunLoop currentRunLoop];
        [commonRunLoop run]; // 不添加任何事件直接 run 

        NSLog(@"♻️♻️ %p %@", commonRunLoop, commonRunLoop);
    }];
    [commonThread start];
}
NSLog(@"🔞 END: %@", [NSThread currentThread]);

// 控制台打印:
🔞 START: <NSThread: 0x282efdac0>{number = 1, name = main}
🔞 END: <NSThread: 0x282efdac0>{number = 1, name = main}
🏃‍♀️🏃‍♀️ <CommonThread: 0x282ea3600>{number = 5, name = (null)} // 子线程
♻️♻️ 0x281ffa940 <CFRunLoop 0x2807ff500 [0x20e729430]>{wakeup port = 0x9b03, stopped = false, ignoreWakeUps = true, 
current mode = (none),
common modes = <CFBasicHash 0x2835b32d0 [0x20e729430]>{type = mutable set, count = 1,
entries =>
    2 : <CFString 0x20e75fc78 [0x20e729430]>{contents = "kCFRunLoopDefaultMode"}
}
,
common mode items = (null),
modes = <CFBasicHash 0x2835b3360 [0x20e729430]>{type = mutable set, count = 1,
entries =>
    2 : <CFRunLoopMode 0x2800fca90 [0x20e729430]>{name = kCFRunLoopDefaultMode, port set = 0x9a03, queue = 0x2815f2880, source = 0x2815f3080 (not fired), timer port = 0x9803, 
    sources0 = (null), // ⬅️ 空
    sources1 = (null), // ⬅️ 空
    observers = (null), // ⬅️ 空
    timers = (null), // ⬅️ 空
    currently 629287011 (5987575088396) / soft deadline in: 7.68614087e+11 sec (@ -1) / hard deadline in: 7.68614087e+11 sec (@ -1)
},

}
}

🍀🍀🍀 0x281ffa940 NSRunLoop dealloc... // commonRunLoop run loop 对象被销毁（run loop 退出）
🍀🍀🍀 <CommonThread: 0x282ea3600>{number = 5, name = (null)} CommonThread dealloc... // commonThread 线程对象被销毁（线程退出）
```
&emsp;运行程序后，我们的 `commonThread` 线程还是退出了，`commonRunLoop` 也退出了。其实是这里涉及到一个知识点，当 run loop 当前运行的 mode 中没有任何需要处理的事件时，run loop 会退出。正如上面控制台中的打印: sources0、sources1、observers、timers 四者都是 `(null)`，所以我们需要创建一个事件让 run loop 来处理，这样 run loop 才不会退出。我们在上面示例代码中的 `[commonRunLoop run];` 行上面添加如下两行：
```c++
[commonRunLoop addPort:[[NSPort alloc] init] forMode:NSDefaultRunLoopMode];
NSLog(@"♻️ %p %@", commonRunLoop, commonRunLoop);
```
&emsp;运行程序发现我们的 `commonThread` 和 `commonRunLoop` 都没有打印 `dealloc`，即它们都没有退出，此时 `commonThread` 线程对应的 run loop 就被启动了。同时仔细观察控制台的话看到 `[commonRunLoop run];` 行下面的 `NSLog(@"♻️♻️ %p %@", commonRunLoop, commonRunLoop);` 行没有得到执行，即使我们在此行打一个断点，发现代码也不会执行到这里，这和我们上面 `main` 函数中由于 `UIApplicationMain` 函数不返回并开启了 main run loop，所以最后的 `return 0;` 行得不到执行的结果是一致的，而这里则是由于 `[commonRunLoop run];` 开启了当前线程的 run loop，自此 `commonThread` 线程进入永久循环，`[commonRunLoop run];` 行可以被看作一个界限，它下面的代码都不会再执行了。同 `UIApplicationMain` 函数一样，这里的 `run` 函数也是不会返回的。下面我们通过开发文档对 NSRunLoop 的 run 函数进行学习。

#### run














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
