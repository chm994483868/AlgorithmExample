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
### 如何对子线程进行保活--手动启动线程的 run loop
&emsp;首先对 “一般来讲，一个线程一次只能执行一个任务，执行完成后线程就会退出。” 这个结论进行证明。这里我们使用 NSThread 作为线程对象，首先创建一个继承自 NSThread 的 CommonThread 类，然后重写它的 `dealloc` 函数（之所以不直接在一个 NSThread 的分类中重写 dealloc 函数，是因为 app 内部的 NSThread 对象的创建和销毁会影响我们的观察） 。
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
    NSLog(@"🍀🍀🍀 %@ CommonThread %s", self, __func__);
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
🏃‍♀️🏃‍♀️ <CommonThread: 0x2825b6e00>{number = 5, name = (null)} // 子线程
🍀🍀🍀 <CommonThread: 0x2825b6e00>{number = 5, name = (null)} CommonThread -[CommonThread dealloc] // commonThread 线程对象被销毁（线程退出）
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

🍀🍀🍀 0x2814eb360 NSRunLoop -[NSRunLoop(Common) dealloc] // commonRunLoop run loop 对象被销毁（run loop 退出）
🍀🍀🍀 <CommonThread: 0x2836ddc40>{number = 6, name = (null)} CommonThread -[CommonThread dealloc] // commonThread 线程对象被销毁（线程退出）
```
&emsp;运行程序后，我们的 `commonThread` 线程还是退出了，`commonRunLoop` 也退出了。其实是这里涉及到一个知识点，当 run loop 当前运行的 mode 中没有任何需要处理的事件时，run loop 会退出。正如上面控制台中的打印: sources0、sources1、observers、timers 四者都是 `(null)`，所以我们需要创建一个事件让 run loop 来处理，这样 run loop 才不会退出。我们在上面示例代码中的 `[commonRunLoop run];` 行上面添加如下两行：
```c++
// 往 run loop 里面添加 Source\Timer\Observer
[commonRunLoop addPort:[[NSPort alloc] init] forMode:NSDefaultRunLoopMode];

// 这里要添加如下完整的 NSTimer 对象，只是添加一个 [[NSTimer alloc] init] 会 crash
// NSTimer *time = [NSTimer scheduledTimerWithTimeInterval:1 repeats:YES block:^(NSTimer * _Nonnull timer) {
//     NSLog(@"⏰⏰ %@", timer);
// }];
// [[NSRunLoop currentRunLoop] addTimer:time forMode:NSDefaultRunLoopMode];

NSLog(@"♻️ %p %@", commonRunLoop, commonRunLoop);

// 控制台部分打印:
...
sources1 = <CFBasicHash 0x60000251a4c0 [0x7fff8002e7f0]>{type = mutable set, count = 1,
entries =>
1 : <CFRunLoopSource 0x600001e700c0 [0x7fff8002e7f0]>{signalled = No, valid = Yes, order = 200, context = <CFMachPort 0x600001c7c370 [0x7fff8002e7f0]>{valid = Yes, por 
// ⬆️ CFMachPort 0x600001c7c370 即为我们添加的 NSPort
...
```
&emsp;运行程序发现我们的 `NSPort` 实例被添加到 `commonRunLoop` 的 `sources1` 中，并且 `commonThread` 和 `commonRunLoop` 都没有打印 `dealloc`，表示我们的线程和 run loop 都没有退出，此时 `commonThread` 线程对应的 run loop 就被启动了，同时观察控制台的话看到 `[commonRunLoop run];` 行下面的 `NSLog(@"♻️♻️ %p %@", commonRunLoop, commonRunLoop);` 行没有得到执行，即使我们在此行打一个断点，发现代码也不会执行到这里，这和我们上面 `main` 函数中由于 `UIApplicationMain` 函数开启了 main run loop 使 `UIApplicationMain` 函数本身不再返回，所以最后的 `return 0;` 行得不到执行的结果是一致的，这里则是由于 `[commonRunLoop run];` 开启了当前线程的 run loop 使 `run` 函数本身不再返回，自此 `commonThread` 线程不再退出并保持活性。`[commonRunLoop run];` 行可以被看作一个界限，它下面的代码在 `commonRunLoop` 启动期间不会再执行了，只有当 `commonRunLoop` 退出时才会执行。

&emsp;下面我们首先通过开发文档对 NSRunLoop 的 `run` 函数进行学习，然后再对 `commonThread` 线程的活性进行验证，然后再使 `commonRunLoop` 失去活性让线程和 run loop 在我们的控制下退出。
#### run
&emsp;`run` 是 `NSRunLoop` 类的一个实例方法，它的主要功能是：Puts the receiver（NSRunLoop 对象） into a permanent loop, during which time it processes data from all attached input sources.
```c++
@interface NSRunLoop (NSRunLoopConveniences)

- (void)run;

// ⬇️ 下面还有两个指定 mode 和 limitDate 的 run 函数
- (void)runUntilDate:(NSDate *)limitDate;
- (BOOL)runMode:(NSRunLoopMode)mode beforeDate:(NSDate *)limitDate;
...
@end
```
&emsp;如果没有 input sources 或 timers（NSTimer）附加到 run loop，此方法将立即退出。否则，它将通过重复调用 `runMode:beforeDate:` 在 `NSDefaultRunLoopMode` 模式下运行 receiver（NSRunLoop 对象）。换句话说，此方法有效地开始了一个无限 loop，该 loop 处理来自 run loop 的 input sources  和 timers 的数据。

&emsp;从 run loop 中手动删除所有已知的 input sources 和 timers 并不能保证 run loop 将退出。macOS 可以根据需要安装和删除附加的 input sources，以处理针对 receiver’s thread 的请求。因此，这些 sources 可以阻止 run loop 退出。

&emsp;如果希望 run loop 终止，则不应使用此方法。相反，请使用其他 run 方法之一，并在循环中检查自己的其他任意条件。一个简单的例子是：
```c++
BOOL shouldKeepRunning = YES; // global
NSRunLoop *theRL = [NSRunLoop currentRunLoop];
while (shouldKeepRunning && [theRL runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]]);
```
&emsp;在程序中的其它位置应将 `shouldKeepRunning` 设置为 `NO`。

&emsp;看到 run 函数的注释已经明确告诉我们，run 内部无限重复调用 `runMode:beforeDate:` 函数，在默认模式下运行 run loop，即开启了一个无限 loop，如果我们打算让 run loop 永久运行且对应的线程也永不退出的话我们可以使用 run 函数来启动 run loop 对象，如果想要根据开发场景需要来任意的启动或停止 run loop 的话，则需要使用 run 函数下面两个有 `limitDate` 参数的 run 函数并结合一个 while 循环使用，如上面 Apple 给的示例代码一样，等下面我们会对此情景进行详细的讲解。

&emsp;`run` 函数的伪代码大概如下，`CFRunLoopStop` 函数（它是 run loop 的停止函数，下面会细讲，这里主要帮助我们理解 run 函数的内部逻辑）对调用 `run` 函数启动的 run loop 无效，使用 `CFRunLoopStop` 函数停止的可能只是某一次循环中的 `runMode:beforeDate:`，下次循环进来时 run loop 对象又一次调用了 `runMode:beforeDate:` 函数。 
```c++
NSRunLoop *theRL = [NSRunLoop currentRunLoop];
while(1) {
    Bool resul = [theRL runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]];
}
```
##### [NSDate distantFuture]
&emsp;`[NSDate distantFuture]` 是一个 NSDate 对象，表示遥远的将来的一个日期（以世纪为单位）。当需要 NSDate 对象以实质上忽略 date 参数时，可以传递此值。可以使用 distantFuture 返回的对象作为 date 参数来无限期地等待事件发生。
```c++
@property (class, readonly, copy) NSDate *distantFuture;
```
&emsp;当前打印 `[NSDate distantFuture]` 是: **4001-01-01 08:00:00，当前实际时间是 2020 12 11**。

&emsp;等下我们再进行手动退出 run loop 的功能点，暂时先验证下在已经启动 run loop 的线程中我们是否可以动态的给该线程添加任务。
#### 在已启动 run loop 的线程中添加任务
&emsp;我们需要对上面的测试代码进行修改。首先我们把上面的 `commonThread` 局部变量修改为 `ViewController` 的一个属性。
```c++
@property (nonatomic, strong) CommonThread *commonThread;
```
&emsp;然后把之前 `commonThread` 局部变量的创建赋值给 `self.commonThread`，然后添加如下一个自定义函数 `rocket:` 和 `ViewController` 的 `touchesBegan:withEvent:` 方法。
```c++
- (void)rocket:(NSObject *)param {
    sleep(1);
    NSLog(@"🚀🚀 %@ param: %p", [NSThread currentThread], param);
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    NSLog(@"📺📺 START...");
    
    // 末尾的 wait 参数表示 performSelector:onThread:withObject: 函数是否等 @selector(rocket:) 执行完成后才返回，还是直接返回，
    // 类似 dispatch_async 和 dispatch_sync，表示在 self.commonThread 线程中是异步执行 @selector(rocket:) 还是同步执行 @selector(rocket:)。
    [self performSelector:@selector(rocket:) onThread:self.commonThread withObject:nil waitUntilDone:YES];
    
    NSLog(@"📺📺 END...");
}
```
&emsp;上面代码编辑好后，触摸 `ViewController` 的空白区域，看到 `rocket` 函数正常执行。
```c++
📺📺 START...
🚀🚀 <CommonThread: 0x281f8ce80>{number = 5, name = (null)} param: 0x0
📺📺 END...
```
> &emsp;（这里发现一个点，连续点击屏幕，点击几次 `rocket` 函数就能执行几次，即使在 `performSelector:onThread:withObject:waitUntilDone:` 函数的最后参数传递 `YES` 时，`touchesBegan:withEvent:` 函数本次没有执行完成的时候，我们就点击屏幕，系统依然会记录我们点击过屏幕的次数，然后 `rocket` 函数就会执行对应的次数。把 thread 参数使用主线程 `[NSThread mainThread]`，依然会执行对应的点击次数，不过子线程和主线程还是有些许区别的，感兴趣的话可以自行测试一下。（其实是我真的不知道怎么描述这个区别）） 

&emsp;然后我们再进行一个测试，把 `self.commonThread` 线程任务中的 run loop 代码注释的话，则触摸屏幕是不会执行 `rocket` 函数的，如果把 `performSelector:onThread:withObject:waitUntilDone:` 函数最后一个参数传 `YES` 的话，则会直接 crash，之前 `commonThread` 线程是一个局部变量的时候我们能看到它会退出并且被销毁了，此时虽然我们修改为了 `ViewController` 的一个属性被强引用，但是当不主动启动 `self.commonThread` 线程的 run loop 的话，它依然是没有活性的。
#### 停止已启动 run loop 线程的 run loop
&emsp;下面学习如何停止 run loop，首先我们在 `ViewController` 上添加一个停止按钮并添加点击事件，添加如下代码:
```c++
// 停止按钮的点击事件
- (IBAction)stopAction:(UIButton *)sender {
    NSLog(@"🎏 stop loop START(ACTION)...");
    [self performSelector:@selector(stopRunLoop:) onThread:self.commonThread withObject:nil waitUntilDone:NO];
    NSLog(@"🎏 stop loop END(ACTION)...");
}

// 停止 run loop
- (void)stopRunLoop:(NSObject *)param {
    NSLog(@"🎏 stop loop START...");
    CFRunLoopStop(CFRunLoopGetCurrent());
    NSLog(@"🎏 stop loop END...");
}
```
&emsp;点击停止按钮后，可看到两个函数都正常的执行了。但是我们点击屏幕的空白区域，发现 `rocket` 函数依然能正常调用。
```c++
 🎏 stop loop START(ACTION)...
 🎏 stop loop END(ACTION)...
 🎏 stop loop START...
 🎏 stop loop END...
 📺📺 START...
 🚀🚀 <CommonThread: 0x2807c2a80>{number = 5, name = (null)} param: 0x0
 📺📺 END...
```
&emsp;那么我们把 `[commonRunLoop run];` 修改为 `[commonRunLoop runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]];`，然后运行程序后，我们直接点击停止按钮，看到控制台有如下打印:
```c++
 🎏 stop loop START(ACTION)...
 🎏 stop loop END(ACTION)...
 🎏 stop loop START...
 🎏 stop loop END...
 ♻️♻️ 0x2819d6700 <CFRunLoop 0x2801d7000 [0x20e729430]>{wakeup port = 0x9b03, stopped = false, ignoreWakeUps = true, 
current mode = (none),
...
 🍀🍀🍀 0x2819d6700 NSRunLoop -[NSRunLoop(Common) dealloc]
```
&emsp;此逻辑大概是 `commonRunLoop` 执行完 `CFRunLoopStop` 函数后，`[commonRunLoop runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]];` 函数返回了，然后下面的 `NSLog(@"♻️♻️ %p %@", commonRunLoop, commonRunLoop);` 得到了执行，然后 `self.commonThread` 创建时添加的 block 函数就完整执行完毕了，完整执行完此逻辑后，`commonRunLoop` 便退出并且销毁了。

&emsp;下面我们再看一种情况。再次运行程序，我们不点击停止按钮，直接点击屏幕空白区域，看到控制台有如下打印:
```c++
 📺📺 START...
 📺📺 END...
  🚀🚀 <CommonThread: 0x280fddb00>{number = 5, name = (null)} param: 0x0
 ♻️♻️ 0x283e86b80 <CFRunLoop 0x282687900 [0x20e729430]>{wakeup port = 0x9b03, stopped = false, ignoreWakeUps = true, 
current mode = (none),
...
 🍀🍀🍀 0x283e86b80 NSRunLoop -[NSRunLoop(Common) dealloc]
```
&emsp;本次我们没有执行 `CFRunLoopStop` 函数，仅在 `self.commonThread` 线程执行了一个事件，执行完 `rocket` 函数以后，`[commonRunLoop runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]];` 函数返回了，然后下面的 `NSLog(@"♻️♻️ %p %@", commonRunLoop, commonRunLoop);` 得到了执行，然后 `self.commonThread` 创建时添加的 block 函数就完整执行完毕了，完整执行完此逻辑后，`commonRunLoop` 便退出并且销毁了。

&emsp;那么我们根据 run 函数中的注释来把代码修改为 Apple 示例代码的样子。首先添加一个布尔类型的 `shouldKeepRunning` 属性，并初始为 `YES`，然后把 `[commonRunLoop runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]];` 修改为 `while (self.shouldKeepRunning && [commonRunLoop runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]]);`，然后进行各项测试，可发现打印结果和 `[commonRunLoop run];` 使用时完全一致。

&emsp;下面我们优化一下代码，添加 `__weak` 修饰的 `self` 防止循环引用:
```c++
NSLog(@"🔞 START: %@", [NSThread currentThread]);
{
    __weak typeof(self) _self = self;
    self.commonThread = [[CommonThread alloc] initWithBlock:^{
        NSLog(@"🏃‍♀️🏃‍♀️ %@", [NSThread currentThread]);
        
        NSRunLoop *commonRunLoop = [NSRunLoop currentRunLoop];
        
        // 往 run loop 里面添加 Source\Timer\Observer
        [commonRunLoop addPort:[[NSPort alloc] init] forMode:NSDefaultRunLoopMode];
        
        // NSTimer *time = [NSTimer scheduledTimerWithTimeInterval:1 repeats:YES block:^(NSTimer * _Nonnull timer) {
        //     NSLog(@"⏰⏰ %@", timer);
        // }];
        // [[NSRunLoop currentRunLoop] addTimer:time forMode:NSDefaultRunLoopMode];
        
        NSLog(@"♻️ %p %@", commonRunLoop, commonRunLoop);
        __strong typeof(_self) self = _self;
        while (self && self.shouldKeepRunning && [commonRunLoop runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]]);
        NSLog(@"♻️♻️ %p %@", commonRunLoop, commonRunLoop);
    }];
    
    [self.commonThread start];
}

NSLog(@"🔞 END: %@", [NSThread currentThread]);
```
&emsp;然后是 `stopRunLoop` 函数，当点击停止按钮时修改 `self.shouldKeepRunning` 为 `NO`，保证 `CFRunLoopStop` 函数执行后 `commonRunLoop` 停止，然后不再进入 `while` 循环。
```c++
- (void)stopRunLoop:(NSObject *)param {
    NSLog(@"🎏 stop loop START...");
    self.shouldKeepRunning = NO;
    CFRunLoopStop(CFRunLoopGetCurrent());
    NSLog(@"🎏 stop loop END...");
}
```
&emsp;然后我们再测试一下，运行程序，首先点击屏幕空白区域，`rocket` 函数正常执行，然后点击停止按钮，看到 `self.commonThread` 线程的 run loop 退出并且 run loop 对象销毁，然后由当前 `ViewControler` 返回上一个控制器（跳转逻辑可以自行添加），看到当前控制器和 `self.commonThread` 线程都正常销毁。如下是控制台打印:
```c++
 🔞 START: <NSThread: 0x283cfda80>{number = 1, name = main}
 🔞 END: <NSThread: 0x283cfda80>{number = 1, name = main}
 🏃‍♀️🏃‍♀️ <CommonThread: 0x283c84180>{number = 6, name = (null)} // ⬅️ self.commonThread 子线程开启
 ♻️ 0x280dfdce0 <CFRunLoop 0x2815f9500 [0x20e729430]>{wakeup port = 0x1507, stopped = false, ignoreWakeUps = true, 
current mode = (none), // ⬅️ 正常获取到 self.commonThread 线程的 run loop 并且给它添加一个事件，防止无事件时 run loop 退出
...

 📺📺 START...
 📺📺 END...
 🚀🚀 <CommonThread: 0x283c84180>{number = 6, name = (null)} param: 0x0 // ⬅️ 触摸屏幕向已保持活性的 self.commonThread 线程添加任务能正常执行 
 🎏 stop loop START(ACTION)... // ⬅️ 点击停止按钮，停止 self.commonThread 线程的 run loop
 🎏 stop loop END(ACTION)...
 🎏 stop loop START...
 🎏 stop loop END...
 ♻️♻️ 0x280dfdce0 <CFRunLoop 0x2815f9500 [0x20e729430]>{wakeup port = 0x1507, stopped = false, ignoreWakeUps = true, 
current mode = (none), // self.commonThread 线程的 run loop 已停止，self.commonThread 线程创建时添加的 block 函数继续往下执行
...

 🍀🍀🍀 0x280dfdce0 NSRunLoop -[NSRunLoop(Common) dealloc] // self.commonThread 线程的 run loop 已经退出，run loop 对象正常销毁
 🍀🍀🍀 <ViewController: 0x10151b630> ViewController -[ViewController dealloc] // pop 后当前控制器正常销毁
 🍀🍀🍀 <CommonThread: 0x283c84180>{number = 6, name = main} CommonThread -[CommonThread dealloc] // self.commonThread 线程对象也正常销毁
```
&emsp;以上是在我们在完全手动可控的情况下：开启线程的 run loop、动态的向已开启 run loop 的线程中添加任务、手动停止已开启 run loop 的线程，看到这里我们大概对 run loop 保持线程的活性有一个整体的认识了。根据 Apple 提供的 NSThread 和 NSRunLoop 类以面向对象的思想学习线程和 run loop 确实更好的帮助我们理解一些概念性的东西。

&emsp;下面我们根据一些重要的知识点对上面的全部代码进行整体优化。

&emsp;`performSelector:onThread:withObject:waitUntilDone:` 函数的最后一个参数 `wait` 传 `YES` 时必须保证 thread 线程参数存在并且该线程已开启 run loop，否则会直接 crash，这是因为线程不满足以上条件时无法执行 selector 参数传递的事件，`wait` 传递 `YES` 又非要等 `selector` 执行完成，这固然是完全是不可能的。所以，我们在所有的 `performSelector:onThread:withObject:waitUntilDone:` 函数执行前可以加一行 `if (!self.commonThread) return;`，这里当然在 `self.commonThread` 线程创建完成后，若 `ViewController` 不 `dealloc` 函数执行完成并彻底销毁并释放 `self.commonThread` 的引用之前，`self.commonThread` 是不会为 `nil` 的，这里我们在 `self.commonThread` 的 run loop 执行 `CFRunLoopStop` 后手动把 `self.commonThread` 置为 `nil`，毕竟失去活性的线程和已经为 `nil` 没什么两样。

&emsp;因为我们在创建 `self.commonThread` 时就已经开启了该线程的 run loop，所以可以保证在向 `self.commonThread` 线程添加事件时它已经保持了活性。

&emsp;还有一个极隐秘的点。当我们使用 block 时会在 block 外面使用 `__weak` 修饰符取得一个的 `self` 的弱引用变量，然后在 block 内部又会使用 `__strong` 修饰符取得一个的 self 弱引用变量的强引用，首先这里是在 block 内部，当 block 执行完毕后会进行自动释放强引用的 self，这里的目的只是为了保证在 block  执行期间 self 不会被释放，这就默认延长了 self 的生命周期到 block 执行结束，这在我们的日常开发中没有任何问题，但是，但是，但是，放在 run loop 这里是不行的，当我们直接 push 进入 `ViewController` 然后直接 pop 会上一个页面时，我们要借用 ViewController 的 dealloc 函数来 stop `self.commonThread` 线程的 run loop 的，如果我们还用 `__strong` 修饰符取得 self 强引用的话，那么由于 `self.commonThread` 线程创建时的 block 内部的 run loop 的 `runMode:beforeDate:` 启动函数是没有返回的，它会一直潜在的延长 self 的生命周期，会直接导致 `ViewController` 无法释放，`dealloc` 函数得不到调用（描述的不够清晰，看下面的实例代码应该会一眼看明白的）。

&emsp;这里是 `__weak` 和 `__strong` 配对使用的一些解释，如果对 block 不清晰的话可以参考前面的文章进行学习。
```c++
// 下面在并行队列里面要执行的 block 没有 retain self
__weak typeof(self) _self = self;
dispatch_async(globalQueue_DEFAULT, ^{
    // 保证在下面的执行过程中 self 不会被释放，执行结束后 self 会执行一次 release。
    
    // 在 ARC 下，这里看似前面的 __wek 和这里的 __strong 相互抵消了，
    // 这里 __strong 的 self，在出了下面的右边花括号时，会执行一次 release 操作。 
    // 且只有此 block 执行的时候 _self 有值那么此处的 __strong self 才会有值，
    // 否则下面的 if 判断就直接 return 了。
    
    __strong typeof(_self) self = _self;
    if (!self) return;
    
    // do something
    // ...
    
    dispatch_async(dispatch_get_main_queue(), ^{
        // 此时如果能进来，表示此时 self 是存在的
        self.view.backgroundColor = [UIColor redColor];
    });
});
```
&emsp;下面是对应上面的解释结果的所有代码。
```c+
#import "ViewController.h"
#import "CommonThread.h"

@interface ViewController ()

@property (nonatomic, strong) CommonThread *commonThread;
@property (nonatomic, assign) BOOL shouldKeepRunning;

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    self.shouldKeepRunning = YES;
    
    NSLog(@"🔞 START: %@", [NSThread currentThread]);
    
    // ⬇️ 下面的 block 内部要使用 self 的弱引用，否则会导致 vc 无法销毁
    __weak typeof(self) _self = self;
    
    self.commonThread = [[CommonThread alloc] initWithBlock:^{
        NSLog(@"🏃‍♀️🏃‍♀️ %@", [NSThread currentThread]);
        NSRunLoop *commonRunLoop = [NSRunLoop currentRunLoop];
        
        // 往 run loop 里面添加 Source/Timer/Observer
        [commonRunLoop addPort:[[NSPort alloc] init] forMode:NSDefaultRunLoopMode];
        NSLog(@"♻️ %p %@", commonRunLoop, commonRunLoop);
        
        // ⬇️ 上面的最后一段描述即针对这里，这里不能再使用 __strong 强引用外部的 _self，会直接导致 vc 无法销毁
        // __strong typeof(_self) self = _self;
        
        while (_self && _self.shouldKeepRunning) {
            [commonRunLoop runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]];
        }
        NSLog(@"♻️♻️ %p %@", commonRunLoop, commonRunLoop);
    }];
    [self.commonThread start];
    NSLog(@"🔞 END: %@", [NSThread currentThread]);
}

- (void)rocket:(NSObject *)param {
    // sleep(3);
    NSLog(@"🚀🚀 %@ param: %p", [NSThread currentThread], param);
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    NSLog(@"📺📺 START...");
    if (!self.commonThread) return; // 首先判断 self.commonThread 不为 nil，下面的 wait 参数是 YES
    [self performSelector:@selector(rocket:) onThread:self.commonThread withObject:nil waitUntilDone:YES];
    NSLog(@"📺📺 END...");
}

- (IBAction)stopAction:(UIButton *)sender {
    NSLog(@"🎏 stop loop START(ACTION)...");
    if (!self.commonThread) return; // 首先判断 self.commonThread 不为 nil，下面的 wait 参数是 YES
    [self performSelector:@selector(stopRunLoop:) onThread:self.commonThread withObject:nil waitUntilDone:YES];
    NSLog(@"🎏 stop loop END(ACTION)...");
}

- (void)stopRunLoop:(NSObject *)param {
    NSLog(@"🎏 stop loop START...");
    self.shouldKeepRunning = NO;
    CFRunLoopStop(CFRunLoopGetCurrent()); // 停止当前线程的 run loop
    self.commonThread = nil; // run loop 停止后在这里把 self.commonThread 置为 nil
    NSLog(@"🎏 stop loop END...");
}

- (void)dealloc {
    [self stopAction:nil]; // 这里随着 vc 的销毁停止 self.commonThread 的 run loop
    NSLog(@"🍀🍀🍀 %@ ViewController %s", self, __func__);
}

@end
```
&emsp;看到这里我们应该对 run loop 和线程的关系有个大概的认知了，当然 run loop 的作用绝不仅仅是线程保活，还有许多其他方面的应用，下篇开始我们对 run loop 进入全面学习，本来准备本篇先对 NSRunLoop.h 文件先进行学习的，但是还是决定把 NSRunLoop 和 CFRunLoopRef 对照学习比较清晰，那么就下篇见吧！ 

&emsp;看到这里我们应该对 run loop 和线程的关系有个大概的认知了，当然 run loop 的作用绝不仅仅是线程保活，还有许多其他各方面的应用，那么下面我们以 Apple 提供的 NSRunLoop 类来学习 run loop。

## NSRunLoop
&emsp;The programmatic interface to objects that manage input sources. 管理输入源的对象的编程接口。
```c++
@interface NSRunLoop : NSObject {
@private
    id          _rl;
    id          _dperf;
    id          _perft;
    id          _info;
    id        _ports;
    void    *_reserved[6];
}
```
&emsp;NSRunLoop 对象处理来自 window system 的鼠标和键盘事件、NSPort 对象和 NSConnection 对象等 sources 的输入。NSRunLoop 对象还处理 NSTimer 事件。

&emsp;你的应用程序既不创建也不显式管理 NSRunLoop 对象。每个 NSThread 对象（包括应用程序的主线程）都有一个根据需要自动为其创建的 NSRunLoop 对象。如果你需要访问当前线程的 run loop，应使用类方法 currentRunLoop 进行访问。请注意，从 NSRunLoop 的角度来看，NSTimer  对象不是 "input"—而是一种特殊类型，这意味着它们在触发时不会导致 run loop 返回。

&emsp;NSRunLoop 类通常不被认为是线程安全的，其方法只能在当前线程的上下文中调用。永远不要尝试调用在其他线程中运行的 NSRunLoop 对象的方法，因为这样做可能会导致意外结果。
### currentRunLoop
&emsp;`currentRunLoop` 返回当前线程的 run loop，返回值是当前线程的 NSRunLoop 对象。如果该线程还没有 run loop，则会为其创建并返回一个 run loop。
```c++
@property (class, readonly, strong) NSRunLoop *currentRunLoop;
```
### currentMode
&emsp;`currentMode` 是接收者（NSRunLoop 实例对象）的当前 input mode。
```c++
@property (nullable, readonly, copy) NSRunLoopMode currentMode;
```
&emsp;接收者的当前 input mode，该方法仅在接收者运行时返回当前输入模式，否则，返回 nil。current mode 由运行 run loop 的方法设置，例如 `acceptInputForMode:beforeDate:` 和 `runMode:beforeDate:`。
### limitDateForMode:
&emsp;`limitDateForMode:` 在指定模式下通过 run loop 执行一次遍历，并返回计划下一个计时器触发的日期。
```c++
- (nullable NSDate *)limitDateForMode:(NSRunLoopMode)mode;
```
&emsp;`mode`：以此 run loop mode 进行搜索。你可以指定自定义模式或使用 Run Loop Modes 中列出的模式之一。





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
