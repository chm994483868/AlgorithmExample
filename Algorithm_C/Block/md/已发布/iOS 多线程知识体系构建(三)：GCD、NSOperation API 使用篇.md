# iOS 多线程知识体系构建(三)：GCD、NSOperation API 使用篇

> &emsp;Grand Central Dispatch (GCD) 是 Apple 开发的一个多核编程的较新的解决方法。
> &emsp;Execute code concurrently on multicore hardware by submitting work to dispatch queues managed by the system.
> &emsp;通过提交工作到 dispatch 系统管理的队列（dispatch queues），在多核硬件上同时执行代码。主要用于优化应用程序以支持多核处理器以及其他对称多处理系统。可以理解为 Dispatch 队列封装了底层多核系统调度的操作，我们只需要关心对 Dispatch 队列的操作，不需要关心任务到底分配给哪个核心，甚至不需要关心任务在哪个线程执行。

## Grand Central Dispatch（GCD）
### GCD 概述
&mesp;Dispatch，也称为 Grand Central Dispatch（GCD），包含语言功能、运行时库和系统增强功能，这些功能为支持 macOS、iOS、watchOS 和 tvOS 中的多核硬件上的并发代码执行提供了系统的、全面的改进。

&emsp;对 BSD子系统、Core Foundation 和 cococoa api 都进行了扩展，以使用这些增强功能来帮助系统和应用程序更快、更高效地运行，并提高响应能力。考虑一下单个应用程序有效地使用多个核心有多困难，更不用说在具有不同计算核心数量的不同计算机上或在多个应用程序竞争这些核心的环境中这样做有多困难。GCD在系统级运行，可以更好地满足所有运行的应用程序的需求，以平衡的方式将它们与可用的系统资源相匹配。
### Dispatch Objects and ARC
> &emsp;GCD 的源码是开源的，我们首先学习完 GCD 的 API 使用以后，再直接深入它的源码一探究竟，这里先有个大致的认知概念就行。

&emsp;何谓 Dispatch Objects 呢？Dispatch Objects（调度对象）有多种类型，包括 `dispatch_queue_t`、`dispatch_group_t` 和 `dispatch_source_t`。基本 Dispatch Object 接口允许管理内存、暂停和恢复执行、定义对象上下文、记录任务数据等。

&emsp;默认情况下，使用 Objective-C 编译器构建 dispatch objects 时，它们被声明为 Objective-C 类型。此行为允许你采用 ARC 并启用静态分析器的内存泄漏检查。它还允许你将对象添加到 Cocoa 集合（NSArray、NSDictionary...）中。

&emsp;使用 Objective-C 编译器构建应用程序时，所有 dispatch objects 都是 Objective-C 对象。因此，启用自动引用计数（ARC）时，调度对象将自动保留和释放，就像任何其它 Objective-C 对象一样。如果未启用 ARC（在 MRC 下），需要使用 `dispatch_retain` 和 `dispatch_release` 函数（或 Objective-C 语义）来保留和释放 dispatch objects。不能使用 Core Foundation 中的 `retain` 和 `release`。

&emsp;如果你需要在启用了 ARC 的应用程序中使用 retain 和 release 语义（为了保持与现有代码的兼容性），可以通过在编译器标志中添加 DOS_OBJECT_USE_OBJC=0 来禁用基于 Objective-C 的 dispatch objects。


sync + 串行队列：不开启新线程，串行执行任务
async + 串行队列：开启新线程（仅开启 1 条），串行执行任务

sync + 并发队列：不开启新线程，串行执行任务（之所以是串行，是因为没有新线程开启，所以只能进行串行执行）
async + 并发队列：开始新线程（开启多条），并发执行任务

sync + 主队列：死锁
async + 主队列：不开启新线程（所有任务都是在主线程执行的，对比上面的 async + 串行队列，会开启一条子线程），串行执行任务


## 参考链接
**参考链接:🔗**
+ [GCD--百度百科词条](https://baike.baidu.com/item/GCD/2104053?fr=aladdin)
+ [Dispatch 官方文档](https://developer.apple.com/documentation/dispatch?language=objc)
+ [iOS多线程：『GCD』详尽总结](https://juejin.im/post/6844903566398717960)
+ [https://juejin.im/post/6844903566398717960](https://juejin.im/post/6844904096973979656)

