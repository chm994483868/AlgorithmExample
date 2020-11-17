# iOS 多线程知识体系构建(三)：GCD API 解析篇

> &emsp;Grand Central Dispatch (GCD) 是 Apple 开发的一个多核编程的较新的解决方法。

> &emsp;Execute code concurrently on multicore hardware by submitting work to dispatch queues managed by the system.
> &emsp;通过提交工作到 dispatch 系统管理的队列（dispatch queues），在多核硬件上同时执行代码。主要用于优化应用程序以支持多核处理器以及其他对称多处理系统。可以理解为 Dispatch 队列封装了底层多核系统调度的操作，我们只需要关心对 Dispatch 队列的操作，不需要关心任务到底分配给哪个核心，甚至不需要关心任务在哪个线程执行（主线程和其它子线程需要关注）。

## Grand Central Dispatch（GCD）
### GCD 概述
&emsp;Dispatch，也称为 Grand Central Dispatch（GCD），包含语言功能、运行时库和系统增强功能，这些功能为支持 macOS、iOS、watchOS 和 tvOS 中的多核硬件上的并发代码执行提供了系统的、全面的改进。

&emsp;对 BSD子系统、Core Foundation 和 cococoa api 都进行了扩展，以使用这些增强功能来帮助系统和应用程序更快、更高效地运行，并提高响应能力。考虑一下单个应用程序有效地使用多个核心有多困难，更不用说在具有不同计算核心数量的不同计算机上或在多个应用程序竞争这些核心的环境中这样做有多困难。GCD 在系统级运行，可以更好地满足所有运行的应用程序的需求，以平衡的方式将它们与可用的系统资源相匹配。
### Dispatch Objects and ARC
> &emsp;GCD 的源码是开源的，我们首先学习完 GCD 的 API 使用以后，再直接深入它的源码一探究竟，这里先有个大致的认知概念就行。

&emsp;何谓 Dispatch Objects 呢？Dispatch Objects（调度对象）有多种类型，包括 `dispatch_queue_t`、`dispatch_group_t` 和 `dispatch_source_t`。基本 Dispatch Object 接口允许管理内存、暂停和恢复执行、定义对象上下文、记录任务数据等。

&emsp;默认情况下，使用 Objective-C 编译器构建 dispatch objects 时，它们被声明为 Objective-C 类型。此行为允许你采用 ARC 并启用静态分析器的内存泄漏检查。它还允许你将对象添加到 Cocoa 集合（NSMutableArray、NSMutableDictionary...）中。

&emsp;使用 Objective-C 编译器构建应用程序时，所有 dispatch objects 都是 Objective-C 对象。因此，启用自动引用计数（ARC）时，调度对象将自动保留和释放，就像任何其它 Objective-C 对象一样。如果未启用 ARC（在 MRC 下），需要使用 `dispatch_retain` 和 `dispatch_release` 函数（或 Objective-C 语义）来保留和释放 dispatch objects。不能使用 Core Foundation 中的 `retain` 和 `release`。

&emsp;如果你需要在启用了 ARC 的应用程序中使用 retain 和 release 语义（为了保持与现有代码的兼容性），可以通过在编译器标志中添加 DOS_OBJECT_USE_OBJC=0 来禁用基于 Objective-C 的 dispatch objects。

&emsp;如果需要在具有更高部署目标的已启用 ARC 的应用程序中使用 retain 和 release 语义（以保持与现有代码的兼容性），可以通过在编译器标志中添加 DOS_OBJECT_USE_OBJC = 0 来禁用基于 Objective-C 的 dispatch objects。

### GCD 中的类型
&emsp;为了深入理解 GCD，首先从我们日常使用 GCD API 中常见的类型入手，搞懂这些类型的具体定义有助于我们理解 GCD 的使用方式以及内部的实现逻辑。首先按住 command 点击 `dispatch_queue_t` 看到 queue.h 文件中的 `dispatch_queue_t` 是一个宏定义：`DISPATCH_DECL(dispatch_queue);`，看到小括号内没有 `_t`，那这个 `_t` 的小尾巴是从哪里来的呢？下面我们沿着 `DISPATCH_DECL` 宏的具体内容来看一下，涉及到的一系列宏都定义在 `usr/include/os/object.h` 文件中。

&emsp;这里为了便于理解我们只看 Objective-C 语言下的 GCD。
+ `DISPATCH_DECL` 宏定义：
```c++
#define DISPATCH_DECL(name) OS_OBJECT_DECL_SUBCLASS(name, dispatch_object)

DISPATCH_DECL(dispatch_queue) ➡️ OS_OBJECT_DECL_SUBCLASS(dispatch_queue, dispatch_object)
```
+ `OS_OBJECT_DECL_SUBCLASS` 宏定义：
```c++
#define OS_OBJECT_DECL_SUBCLASS(name, super) \
        OS_OBJECT_DECL_IMPL(name, <OS_OBJECT_CLASS(super)>)

OS_OBJECT_DECL_SUBCLASS(dispatch_queue, dispatch_object) ➡️ OS_OBJECT_DECL_IMPL(dispatch_queue, <OS_OBJECT_CLASS(dispatch_object)>)
```
+ `OS_OBJECT_CLASS` 宏定义：( `##` 运算符可以用于宏函数的替换部分。这个运算符把两个语言符号组合成单个语言符号，为宏扩展提供了一种连接实际变元的手段。)
```c++
#define OS_OBJECT_CLASS(name) OS_##name

<OS_OBJECT_CLASS(dispatch_object)> ➡️ <OS_dispatch_object>
OS_OBJECT_DECL_IMPL(dispatch_queue, <OS_OBJECT_CLASS(dispatch_object)>) ➡️ OS_OBJECT_DECL_IMPL(dispatch_queue, <OS_dispatch_object>)
```
+ `OS_OBJECT_DECL_IMPL` 宏定义：
```c++
#define OS_OBJECT_DECL_IMPL(name, ...) \
        OS_OBJECT_DECL_PROTOCOL(name, __VA_ARGS__) \
        typedef NSObject<OS_OBJECT_CLASS(name)> \
                * OS_OBJC_INDEPENDENT_CLASS name##_t
        
OS_OBJECT_DECL_IMPL(dispatch_queue, <OS_dispatch_object>) ➡️ 

    OS_OBJECT_DECL_PROTOCOL(dispatch_queue, <OS_dispatch_object>) \
    typedef NSObject<OS_dispatch_queue> \
            * OS_OBJC_INDEPENDENT_CLASS dispatch_queue_t
```
+ `OS_OBJC_INDEPENDENT_CLASS` 宏定义：
```c++
#if __has_attribute(objc_independent_class)
#define OS_OBJC_INDEPENDENT_CLASS __attribute__((objc_independent_class))
#endif // __has_attribute(objc_independent_class)

#ifndef OS_OBJC_INDEPENDENT_CLASS
#define OS_OBJC_INDEPENDENT_CLASS
#endif

OS_OBJECT_DECL_PROTOCOL(dispatch_queue, <OS_dispatch_object>) \
typedef NSObject<OS_dispatch_queue> \
        * OS_OBJC_INDEPENDENT_CLASS dispatch_queue_t ➡️

OS_OBJECT_DECL_PROTOCOL(dispatch_queue, <OS_dispatch_object>) \
typedef NSObject<OS_dispatch_queue> \
        * dispatch_queue_t
```
+ `OS_OBJECT_DECL_PROTOCOL` 宏定义：
```c++
#define OS_OBJECT_DECL_PROTOCOL(name, ...) \
        @protocol OS_OBJECT_CLASS(name) __VA_ARGS__ \
        @end

OS_OBJECT_DECL_PROTOCOL(dispatch_queue, <OS_dispatch_object>) \
typedef NSObject<OS_dispatch_queue> \
        * dispatch_queue_t ➡️

@protocol OS_dispatch_queue <OS_dispatch_object> \
@end \
typedef NSObject<OS_dispatch_queue> \
        * dispatch_queue_t
```
&emsp;连续的宏定义整理到这里 `DISPATCH_DECL(dispatch_queue);` 即为:
```c++
@protocol OS_dispatch_queue <OS_dispatch_object>
@end

typedef NSObject<OS_dispatch_queue> * dispatch_queue_t;
```
&emsp;`OS_dispatch_queue` 是继承自 `OS_dispatch_object` 协议的协议，并且为遵循该协议的 `NSObject` 实例对象类型的指针定义了一个 `dispatch_queue_t` 的别名，看到这里我们恍然大悟，我们整天使用的 `dispatch_queue_t globalQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);` 获取一个全局并发对象，而这个 `globalQueue` 其实就是一个遵循 `OS_dispatch_queue` 协议的 `NSObject` 实例对象指针（`dispatch_queue_t` 具体是不是 NSObject 后面待确认）。

&emsp;那么这个 `OS_dispatch_object` 协议怎么来的呢？可看到是来自 `OS_OBJECT_DECL_CLASS(dispatch_object);` 下面对它进行解读。
```c++
/*
* By default, dispatch objects are declared as Objective-C types when building
* with an Objective-C compiler. This allows them to participate in ARC, in RR
* management by the Blocks runtime and in leaks checking by the static
* analyzer, and enables them to be added to Cocoa collections.
* See <os/object.h> for details.
*/

/*
 * 默认情况下，使用 Objective-C 编译器进行构建时，dispatch objects 被声明为 Objective-C 类型（NSObject）。
 * 这使他们可以参与 ARC，通过 Blocks 运行时参与 RR（retain/release）管理以及通过静态分析器参与泄漏检查，
 * 并将它们添加到 Cocoa 集合（NSMutableArray、NSMutableDictionary...）中。有关详细信息，请参见。
 */
OS_OBJECT_DECL_CLASS(dispatch_object);

// 如下代码验证:
dispatch_queue_t globalQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
NSMutableArray *array = [NSMutableArray array];
[array addObject:globalQueue];
NSLog(@"array:  %@", array);
NSLog(@"🍑🍑 %d", [globalQueue isKindOfClass:[NSObject class]]);
// 控制台如下打印：

2020-11-16 18:27:24.218954+0800 Simple_iOS[69017:9954014] array:  ( ⬅️ dispatch_queue_t 被添加到 NSMutableArray 中
    "<OS_dispatch_queue_global: com.apple.root.default-qos>"
)
2020-11-16 18:27:24.219246+0800 Simple_iOS[69017:9954014] 🍑🍑 1 ⬅️ dispatch_queue_t 是一个指向 NSObject 类型的指针
```
+ `OS_OBJECT_DECL_CLASS` 宏定义：
```c++
#define OS_OBJECT_DECL_CLASS(name) \
        OS_OBJECT_DECL(name)

OS_OBJECT_DECL_CLASS(dispatch_object) ➡️ OS_OBJECT_DECL(dispatch_object)
```
+ `OS_OBJECT_DECL` 宏定义：
```c++
#define OS_OBJECT_DECL(name, ...) \
        OS_OBJECT_DECL_IMPL(name, <NSObject>)

OS_OBJECT_DECL(dispatch_object) ➡️ OS_OBJECT_DECL_IMPL(dispatch_object, <NSObject>)
```
+ `OS_OBJECT_DECL_IMPL` 宏定义：
```c++
#define OS_OBJECT_DECL_IMPL(name, ...) \
        OS_OBJECT_DECL_PROTOCOL(name, __VA_ARGS__) \
        typedef NSObject<OS_OBJECT_CLASS(name)> \
                * OS_OBJC_INDEPENDENT_CLASS name##_t
                
OS_OBJECT_DECL_IMPL(dispatch_object, <NSObject>) ➡️

OS_OBJECT_DECL_PROTOCOL(dispatch_object, <NSObject>) \
typedef NSObject<OS_dispatch_object> \
        * OS_OBJC_INDEPENDENT_CLASS dispatch_object_t ➡️
        
@protocol OS_dispatch_object <NSObject> \
@end \
typedef NSObject<OS_dispatch_object> \
        * dispatch_object_t  
```
&emsp;连续的宏定义整理到这里 `OS_OBJECT_DECL_CLASS(dispatch_object);` 即为:
```c++
@protocol OS_dispatch_object <NSObject> 
@end

typedef NSObject<OS_dispatch_object> * dispatch_object_t;  
```
&emsp;`OS_dispatch_object` 是继承自 `NSObject` 协议的协议，并且为遵循该协议的 `NSObject` 实例对象类型的指针定义了一个 `dispatch_object_t` 的别名。（`dispatch_object_t` 具体是不是 NSObject 后面待确认）

&emsp;综上可知，宏定义 `OS_OBJECT_DECL_CLASS(name)` 会定义一个继承自 `NSObject` 协议的协议，协议的名称为固定的 `name` 添加 `OS_` 前缀，并且定义一个表示遵循该协议的 `NSObject` 实例对象类型的指针的别名，名称为 `name` 添加后缀 `_t`。

&emsp;由 `#define DISPATCH_DECL_SUBCLASS(name, base) OS_OBJECT_DECL_SUBCLASS(name, base)` 可知，还可以在定义一个协议时，指定其所继承的协议，但是在使用时，要保证指定的 `base` 协议是已经定义过的。
#### dispatch_queue_t
&emsp;Dispatch 是用于通过简单但功能强大的 API 来表达并发性的抽象模型。在核心上，dispatch 提供了可以向其提交 blocks 的串行 FIFO 队列。提交给这些 dispatch queues 的 blocks 在系统完全管理的线程池上调用。无法保证将在哪个线程上调用 block；但是，它保证一次只调用一个提交到 FIFO dispatch queue 的 block。当多个队列有要处理的块时，系统可以自由地分配额外的线程来并发地调用这些 blocks。当队列变为空时，这些线程将自动释放。

```c++
DISPATCH_DECL(dispatch_queue);
```
&emsp;转换宏定义后是：
```c++
@protocol OS_dispatch_queue <OS_dispatch_object>
@end

typedef NSObject<OS_dispatch_queue> * dispatch_queue_t;
```
&emsp;Dispatch queues 调用提交给它们的工作项。

&emsp;Dispatch queues 有多种形式，最常见的一种是调度串行队列（`dispatch_queue_serial_t`）。系统管理一个线程池，该线程池处理 dispatch queues 并调用提交给它们的工作项。从概念上讲，一个 dispatch queue 可以具有自己的执行线程，并且队列之间的交互是高度异步的。调度队列通过调用 dispatch_retain() 和 dispatch_release() 进行引用计数。提交给队列的待处理工作项也会保留对该队列的引用，直到它们完成为止。一旦释放了对队列的所有引用，系统将重新分配该队列（queue 被释放销毁）。
#### dispatch_queue_global_t
```c++
DISPATCH_DECL_SUBCLASS(dispatch_queue_global, dispatch_queue);
```
&emsp;转换宏定义后是：
```c++
@protocol OS_dispatch_queue_global <OS_dispatch_queue>
@end

typedef NSObject<OS_dispatch_queue_global> * dispatch_queue_global_t;
```
&emsp;`OS_dispatch_queue_global` 是继承自 `OS_dispatch_queue` 协议的协议，并且为遵循该协议的 `NSObject` 实例对象类型的指针定义了一个 `dispatch_queue_global_t` 的别名。（`dispatch_queue_global_t` 具体是不是 NSObject 后面待确认）

&emsp;调度全局并发队列（dispatch global concurrent queues）是围绕系统线程池的抽象，它调用提交到调度队列的工作项。

&emsp;调度全局并发队列（dispatch global concurrent queues）在系统管理的线程池之上提供优先级桶（这个大概是哈希桶，后续看源码时再分析），系统将根据需求和系统负载决定分配给这个池的线程数。特别是，系统会尝试为该资源保持良好的并发级别，并且当系统调用中有太多的现有工作线程阻塞时，将创建新线程。（NSThread 和 GCD 的一个重大区别，GCD 下线程都是系统自动创建分配的，而 NSThread 则是自己手动创建线程或者自己手动开启线程。）

&emsp;全局并发队列（global concurrent queues）是共享资源，因此，此资源的每个用户都有责任不向该池提交无限数量的工作，尤其是可能阻塞的工作，因为这可能导致系统产生大量线程（又名：线程爆炸 thread explosion）。

&emsp;提交到全局并发队列（global concurrent queues）的工作项相对于提交顺序没有排序保证，并且提交到这些队列的工作项可以并发调用（毕竟本质还是并发队列）。

&emsp;调度全局并发队列（dispatch global concurrent queues）是由 `dispatch_get_global_queue()` 函数返回的已知全局对象，这些对象无法修改。 `dispatch_suspend()`、`dispatch_resume()`、`dispatch_set_context()` 等等函数对此类型的队列调用无效。
#### dispatch_queue_serial_t
```c++
DISPATCH_DECL_SUBCLASS(dispatch_queue_serial, dispatch_queue);
```
&emsp;转换宏定义后是：
```c++
@protocol OS_dispatch_queue_serial <OS_dispatch_queue>
@end

typedef NSObject<OS_dispatch_queue_serial> * dispatch_queue_serial_t;
```
&emsp;`OS_dispatch_queue_serial` 是继承自 `OS_dispatch_queue` 协议的协议，并且为遵循该协议的 `NSObject` 实例对象类型的指针定义了一个 `dispatch_queue_serial_t` 的别名。（`dispatch_queue_serial_t` 具体是不是 NSObject 后面待确认）

&emsp;调度串行队列（dispatch serial queues）调用以 FIFO 顺序串行提交给它们的工作项。

&emsp;调度串行队列（dispatch serial queues）是轻量级对象，可以向其提交工作项以 FIFO 顺序调用。串行队列一次只能调用一个工作项，但是独立的串行队列可以各自相对于彼此并发地调用其工作项。

&emsp;串行队列可以相互定位（`dispatch_set_target_queue()`）（串行队列可以彼此作为目标）。队列层次结构底部的串行队列提供了一个排除上下文：在任何给定的时间，提交给这种层次结构中的任何队列的最多一个工作项将运行。这样的层次结构提供了一个自然的结构来组织应用程序子系统。

&emsp;通过将派生自 `DISPATCH_QUEUE_SERIAL` 的调度队列属性传递给 `dispatch_queue_create_with_target()` 来创建串行队列。（串行队列的创建过程后续会通过源码来进行解读）
#### dispatch_queue_main_t
```c++
DISPATCH_DECL_SUBCLASS(dispatch_queue_main, dispatch_queue_serial);
```
&emsp;转换宏定义后是：
```c++
@protocol OS_dispatch_queue_main <OS_dispatch_queue_serial>
@end

typedef NSObject<OS_dispatch_queue_main> * dispatch_queue_main_t;
```
&emsp;`OS_dispatch_queue_main` 是继承自 `OS_dispatch_queue_serial` 协议的协议，并且为遵循该协议的 `NSObject` 实例对象类型的指针定义了一个 `dispatch_queue_main_t` 的别名。（`dispatch_queue_main_t` 具体是不是 NSObject 后面待确认，看到这里发现主队列不愧是特殊的串行队列）

&emsp;`dispatch_queue_main_t` 是绑定到主线程的默认队列的类型。

&emsp;主队列是一个串行队列（`dispatch_queue_serial_t`），该队列绑定到应用程序的主线程。为了调用提交到主队列的工作项，应用程序必须调用 `dispatch_main()`，`NSApplicationMain()` 或在主线程上使用 `CFRunLoop`。

&emsp;主队列是一个众所周知的全局对象，它在进程初始化期间代表主线程自动创建，并由 `dispatch_get_main_queue()` 返回，无法修改该对象。`dispatch_suspend()`、`dispatch_resume()`、`dispatch_set_context()` 等等函数对此类型的队列调用无效（主队列只有一个，全局并发队列有多个）。
#### dispatch_queue_concurrent_t
```c++
DISPATCH_DECL_SUBCLASS(dispatch_queue_concurrent, dispatch_queue);
```
&emsp;转换宏定义后是：
```c++
@protocol OS_dispatch_queue_concurrent <OS_dispatch_queue>
@end

typedef NSObject<OS_dispatch_queue_concurrent> * dispatch_queue_concurrent_t;
```
&emsp;`OS_dispatch_queue_concurrent` 是继承自 `OS_dispatch_queue` 协议的协议，并且为遵循该协议的 `NSObject` 实例对象类型的指针定义了一个 `dispatch_queue_concurrent_t` 的别名。（`dispatch_queue_concurrent_t` 具体是不是 NSObject 后面待确认）

&emsp;调度并发队列（dispatch concurrent queues）会同时调用提交给它们的工作项，并接受屏障工作项的概念（and admit a notion of barrier workitems，（barrier 屏障是指调用 `dispatch_barrier_async` 函数，向队列提交工作项。））。

&emsp;调度并发队列（dispatch concurrent queues）是可以向其提交常规和屏障工作项的轻量级对象。在排除其他任何类型的工作项目（按FIFO顺序）时，将调用屏障工作项目。（提交在 barrier 工作项之前的工作项并发执行完以后才会并发执行 barrier 工作项之后的工作项）。

&emsp;可以对同一并发队列以任何顺序并发调用常规工作项。但是，在调用之前提交的任何屏障工作项之前，不会调用常规工作项。

&emsp;换句话说，如果在 Dispatch 世界中串行队列等效于互斥锁，则并发队列等效于 reader-writer lock，其中常规项是读取器，而屏障是写入器。

&emsp;通过将派生自 `DISPATCH_QUEUE_CONCURRENT` 的调度队列属性传递给 `dispatch_queue_create_with_target()` 来创建并发队列。

&emsp;注意事项：当调用优先级较低的常规工作项（readers）时，此时调度并发队列不会实现优先级反转避免，并且会阻止调用优先级较高的屏障（writer）。
#### dispatch_block_t
&emsp;日常使用 GCD 向队列提交的工作项都是这种名字是 `dispatch_block_t`，参数和返回值都是 `void` 的 Block。
```c++
typedef void (^dispatch_block_t)(void);
```
&emsp;提交给调度队列（dispatch queues）的 blocks 的类型，不带任何参数且没有返回值。当不使用 Objective-C ARC 进行构建时，分配到堆上或复制到堆上的 block 对象必须通过 `-[release]` 消息或 `Block_release()` 函数释放。

&emsp;以字面量形式声明的 block 分配空间存储在栈上。因此如下是一个无效的构建:
```c++
dispatch_block_t block;
if (x) {
    block = ^{ printf("true\n"); };
} else {
    block = ^{ printf("false\n"); };
}
block(); // unsafe!!!
```
&emsp;幕后发生的事情：
```c++
if (x) {
    struct Block __tmp_1 = ...; // setup details
    block = &__tmp_1;
} else {
    struct Block __tmp_2 = ...; // setup details
    block = &__tmp_2;
}
```
&emsp;如示例所示，栈变量的地址正在转义其分配范围。那是一个经典的C bug。相反，必须使用 `Block_copy()` 函数或通过发送 `-[copy]` 消息将 block 复制到堆中。(看到这里对 block 内部结构比较熟悉的同学感觉应该会很亲切。)

#### dispatch_async
```c++
#ifdef __BLOCKS__
API_AVAILABLE(macos(10.6), ios(4.0))
DISPATCH_EXPORT DISPATCH_NONNULL_ALL DISPATCH_NOTHROW
void
dispatch_async(dispatch_queue_t queue, dispatch_block_t block);
#endif
```



## 参考链接
**参考链接:🔗**
+ [swift-corelibs-libdispatch-main](https://github.com/apple/swift-corelibs-libdispatch)
+ [Dispatch 官方文档](https://developer.apple.com/documentation/dispatch?language=objc)
+ [iOS libdispatch浅析](https://juejin.im/post/6844904143174238221)
+ [GCD--百度百科词条](https://baike.baidu.com/item/GCD/2104053?fr=aladdin)
+ [iOS多线程：『GCD』详尽总结](https://juejin.im/post/6844903566398717960)
+ [iOS底层学习 - 多线程之GCD初探](https://juejin.im/post/6844904096973979656)
+ [GCD 中的类型](https://blog.csdn.net/u011374318/article/details/87870585)
+ [iOS Objective-C GCD之queue（队列）篇](https://www.jianshu.com/p/d0017f74f9ca)
+ [变态的libDispatch结构分析-object结构](https://blog.csdn.net/passerbysrs/article/details/18223845)

