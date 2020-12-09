# iOS 从源码解析Runloop (一)：runloop 基本概念理解篇

> &emsp;Runloops 是与 threads 关联的基本基础结构的一部分。runloop 是一个 event processing loop （**事件处理循环**），可用于计划工作并协调收到的事件的接收。runloop 的目的是让 thread 在有工作要做时保持忙碌，而在没有工作时让 thread 进入睡眠状态。（官方解释初次看时显的过于生涩，下面我们试图对 runloop 的概念进行深入理解。）

## runloop 概念
&emsp;什么是 runloop？顾名思义，runloop 就是在 “跑圈”，runloop 运行的核心代码就是一个有状态的 **do while循环**，do while 循环我们可能已经写过无数次，当然我们写的都是达到限制条件时要退出的 do while 循环，否则就成死循环了，而 runloop 只要不是超时或者故意退出状态下都可以看作是一个 “死循环”。

&emsp;runloop 提供了这么一种机制，当有任务处理时，线程的 runLoop 会保持忙碌，而在没有任何任务处理时，会让线程休眠，从而让出 CPU。当再次有任务需要处理时，runLoop 会被唤醒，来处理事件，直到任务处理完毕，再次进入休眠。

&emsp;前面我们学习线程时，多次提到主线程主队列都是在 app 启动时默认创建的，而恰恰主线程的 runloop 也是在 app 启动时默认跟着启动的，那么我们从 main.m 文件中找出一些端倪，使用 Xcode 创建一个 OC 语言的 Single View App 时会自动生成如下的 main.m 文件：
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
}
```
&emsp;`main` 函数最后一行 `return` 语句是返回 `UIApplicationMain` 函数的执行结果，我们把此行注释，然后添加一行 `return 0;`，运行程序后会看到执行 `NSLog` 语句后程序就结束了，而最后一行是 `return UIApplicationMain(argc, argv, nil, appDelegateClassName);` 的话运行程序后就进入了 app 的首页而并不会结束程序，那么我们大概想到了这个 `UIApplicationMain` 函数是不会返回的，它不会返回，所以 `main` 函数也就不会返回了。下面看一下 `UIApplicationMain` 函数的声明，看到是一个返回值是 int 类型的函数。
```c++
UIKIT_EXTERN int UIApplicationMain(int argc, char * _Nullable argv[_Nonnull], NSString * _Nullable principalClassName, NSString * _Nullable delegateClassName);
```
> &emsp;Return Value
> &emsp;Even though an integer return type is specified, this function never returns. When users exits an iOS app by pressing the Home button, the application moves to the background.
> &emsp;
> &emsp;返回值
> &emsp;即使指定了整数返回类型，此函数也从不返回。当用户通过按 Home 键退出 iOS 应用时，该应用将移至后台。

&emsp;在开发者文档中查看 `UIApplicationMain` 函数的注释，这里我们只看 Return Value 一项。看到 Apple 已经明确告诉我们 `UIApplicationMain` 函数是不会返回的。那么这里我们是不是可以理解为我们的 







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
