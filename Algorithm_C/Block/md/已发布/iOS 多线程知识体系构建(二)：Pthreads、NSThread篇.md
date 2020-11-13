# iOS 多线程知识体系构建(二)：Pthreads、NSThread篇

> &emsp;本篇首先来学习 iOS 多线程技术中的 NSThread。⛽️⛽️

## Pthreads

> &emsp;可移植操作系统接口（英语：Portable Operating System Interface，缩写为POSIX）是 IEEE（电气和电子工程师协会）为要在各种 UNIX 操作系统上运行软件，而定义 API 的一系列互相关联的标准的总称，其正式称呼为 IEEE Std 1003，而国际标准名称为 ISO/IEC 9945。此标准源于一个大约开始于1985 年的项目。POSIX 这个名称是由理查德·斯托曼（RMS）应 IEEE 的要求而提议的一个易于记忆的名称。它基本上是 Portable Operating System Interface（可移植操作系统接口）的缩写，而 X 则表明其对 Unix API 的传承。--来自百度百科。

&emsp;Pthreads 一般指 POSIX 线程。 POSIX 线程（POSIX Threads，常被缩写为 Pthreads）是 POSIX 的线程标准，定义了创建和操纵线程的一套 API。
### Pthreads 简介
&emsp;实现 POSIX 线程标准的库常被称作 Pthreads，一般用于 Unix-like POSIX  系统，如 Linux、Solaris、macOS。但是 Microsoft Windows 上的实现也存在，例如直接使用 Windows API 实现的第三方库 pthreads-w32，而利用 Windows 的 SFU/SUA 子系统，则可以使用微软提供的一部分原生 POSIX API。Pthreads 是一套通用的多线程的 API，可以在 Unix / Linux / Windows 等系统跨平台使用，使用 C  语言编写，需要程序员自己管理线程的生命周期，这里我们对它常用的 API 学习一下，等到后面学习 GCD  源码的时候都会用到。

&emsp;Pthreads 定义了一套 C 语言的类型、函数与常量，它以 pthread.h 头文件和一个线程库实现。Pthreads API 中大致共有 100 个函数调用，全都以 "pthread_" 开头，并可以分为四类：（这里我们只关注第一类：线程管理，其它三类关于锁的部分可以参考前面锁的文章）

1. 线程管理，例如创建线程，等待（join）线程，查询线程状态等。
2. 互斥锁（Mutex）：创建、摧毁、锁定、解锁、设置属性等操作。
3. 条件变量（Condition Variable）：创建、摧毁、等待、通知、设置与查询属性等操作。
4. 使用了互斥锁的线程间的同步管理

### Pthreads 使用
&emsp;在 iOS 中使用 Pthreads API 首先要引入头文件: `#import <pthread.h>`。我们先看一个最简单的开启线程的例子：
```c++
- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.

    // 定义一个 pthread_t 类型变量，为指向线程的指针，
    // 它实际类型是 struct _opaque_pthread_t 指针。
    pthread_t thread = NULL;
    
    // 用于在主线程内接收子线程任务执行完成后的返回值。（如果没有返回值则可忽略）
    void* thread_ret = NULL;
    
    // 准备一个变量用于传递参数
    NSObject *objc = [[NSObject alloc] init];
    NSLog(@"objc: %p", objc);
    
    // (__bridge void *)：在 C 和 OC 之间传递数据，需要使用 __bridge 进行桥接，桥接的目的就是为了告诉编译器如何管理内存。
    // 也可使用 CFBridgingRetain：pthread_create(&thread, NULL, run, (void *)CFBridgingRetain(objc))
    // 但是后要跟 CFBridgingRelease((__bridge void *)objc)，要不然会导致 objc 内存泄漏。
    
    // 直接传递 objc 会提示如下错误，并给了我们两种解决方法:
    // Implicit conversion of Objective-C pointer type 'NSObject *' to C pointer type 'void *' requires a bridged cast
    // Use __bridge to convert directly (no change in ownership)
    // Use CFBridgingRetain call to make an ARC object available as a +1 'void *'
    
    // MRC 中不需要使用桥接，能直接使用 objc
    
    int result = pthread_create(&thread, NULL, run, (__bridge void *)(objc));
    
    if (result == 0) {
        NSLog(@"创建线程成功 🎉🎉");
    } else {
        NSLog(@"创建线程失败 ❌，失败编号: %d", result);
    }
    
    // 线程分离
    // 设置子线程的状态设置为 detached，进行线程分离，该线程运行结束后系统会进行处理并释放线程的所有资源，
    // 或者在子线程中添加 pthread_detach(pthread_self())，其中 pthread_self() 是获得当前线程自身
    pthread_detach(thread);
    
    // 线程合并
    // 另外一种方式是进行线程合并 pthread_join，看到下面的 run 函数了吗，它是有一个通用的返回值的 void *，
    // 那么我们如何接收它的返回值呢？
    
    // pthread_join(thread, (void**)&thread_ret);
    // NSLog(@"thread_ret: %p", thread_ret);
    
    // 上面的线程分离和线程合并我们必须选择一种，否则会发生线程资源泄漏。具体的细节在下面进行分析。
    
    NSLog(@"🧑‍💻 %@", [NSThread currentThread]);
}

void* run(void *param) {
    // sleep(2);

    NSLog(@"🏃‍♀️ %@ param: %p", [NSThread currentThread], param);
    
    // return param;
    return NULL;
}

// 控制台打印:
objc: 0x6000039da810
创建线程成功 🎉🎉
🧑‍💻 <NSThread: 0x600002ec6980>{number = 1, name = main} // 主线程

// 本地打印看到 run 函数是最后执行的，如果把 objc 换成自己的类，并重写 dealloc 函数，会在这行多一条: 🍀🍀🍀 TEMP dealloc 打印
// 当 run 函数执行时，objc 已经释放了，再在其内部打印 objc 不是野指针访问吗...？？？??

// 注释 pthread_join 内容，打开 pthread_detach 内容，多次执行会发现 🏃‍♀️ 和 🧑‍💻 打印顺序是不定的，即子线程是异步执行的，不会阻塞主线程。

// 注释 pthread_detach，打开 pthread_join 内容。pthread_join(thread, (void**)&thread_ret) 来获取 run 函数返回值的话，🧑‍💻 会等到 🏃‍♀️ 执行完成以后才会执行。
// 即 pthread_join 会阻塞主线程的执行，直到 run 函数执行完毕并返回。

🏃‍♀️ <NSThread: 0x600002ea36c0>{number = 6, name = (null)} param: 0x6000039da810 // run 函数内部的开启了子线程，obj 也被传递过来了
```
&emsp;`pthread_detach` 线程分离时的不同的打印顺序。
```c++
2020-11-13 16:30:44.152497+0800 Simple_iOS[50694:9047896] objc: 0x600001558a40
2020-11-13 16:30:44.152932+0800 Simple_iOS[50694:9048038] 🏃‍♀️ <NSThread: 0x6000002d4fc0>{number = 6, name = (null)} param: 0x600001558a40
2020-11-13 16:30:44.153245+0800 Simple_iOS[50694:9047896] 创建线程成功 🎉🎉
2020-11-13 16:30:44.155304+0800 Simple_iOS[50694:9047896] 🧑‍💻 <NSThread: 0x60000025acc0>{number = 1, name = main}
```
```c++
2020-11-13 16:31:23.100374+0800 Simple_iOS[50700:9048791] objc: 0x600003ab4490
2020-11-13 16:31:23.100671+0800 Simple_iOS[50700:9048791] 创建线程成功 🎉🎉
2020-11-13 16:31:23.100921+0800 Simple_iOS[50700:9048791] 🧑‍💻 <NSThread: 0x600002da3040>{number = 1, name = main}
2020-11-13 16:31:23.100770+0800 Simple_iOS[50700:9048878] 🏃‍♀️ <NSThread: 0x600002df9580>{number = 6, name = (null)} param: 0x600003ab4490
```
> &emsp;这里看一个题外话，看源码时经常遇到命名后缀有 _t/ref 的类型，如 spinlock_t、weak_table_t、weak_entry_t 等，后缀 t 是用来表示 struct 的，这是由于 C 中没有类的定义，它是面向过程的语言，在 C 语言中表示类型时是用结构体 struct，所以在后面加一个 _t 只是为了标识类型。[iOS多线程中的实际方案之一pthread](https://www.jianshu.com/p/cfc6e7d2316a)

> &emsp;在混合开发时，如果在 C 和 OC 之间传递数据，需要使用 __bridge 进行桥接，桥接的目的就是为了告诉编译器如何管理内存，MRC 中不需要使用桥接；在 OC 中，如果是 ARC 开发，编译器会在编译时，根据代码结构，自动添加 retain/release/autorelease。但是，ARC 只负责管理 OC 部分的内存管理，而不负责 C 语言 代码的内存管理。因此，开发过程中，如果使用的 C 语言框架出现 retain/create/copy/new 等字样的函数，大多都需要 release，否则会出现内存泄漏，如上面的 CFBridgingRetain 和 CFBridgingRelease 配对使用。[iOS多线程中的实际方案之一pthread](https://www.jianshu.com/p/cfc6e7d2316a)

### pthread_t 定义
&emsp;`pthread_t` 是一个指向线程的指针，在 iOS 它是: `__darwin_pthread_t`。下面看一下源码定义:
```c++
typedef __darwin_pthread_t pthread_t;

typedef struct _opaque_pthread_t *__darwin_pthread_t;

struct _opaque_pthread_t {
    long __sig;
    struct __darwin_pthread_handler_rec  *__cleanup_stack;
    char __opaque[__PTHREAD_SIZE__];
};

struct __darwin_pthread_handler_rec {
    void (*__routine)(void *);    // Routine to call 线程的入口函数，即需要在新线程中执行的任务
    void *__arg;            // Argument to pass __routine 函数的参数
    struct __darwin_pthread_handler_rec *__next; 
};
```
&emsp;通过上面的代码一层一层递进：`pthread_t` 其实是 `_opaque_pthread_t` 结构体指针。
### pthread_create 线程创建
&emsp;`pthread_create` 是类 Unix 操作系统（Unix、Linux、Mac OS X等）的创建线程的函数。它的功能是创建线程（实际上就是确定调用该线程函数的入口点），在线程创建以后，就开始运行相关的线程函数。
`pthread_create` 的返回值: 若成功，返回 0；若出错，返回出错编号，并且 `pthread_t * __restrict` 中的内容未定义。下面看一下它的函数声明：

```c++
__API_AVAILABLE(macos(10.4), ios(2.0))
#if !_PTHREAD_SWIFT_IMPORTER_NULLABILITY_COMPAT
int pthread_create(pthread_t _Nullable * _Nonnull __restrict,
        const pthread_attr_t * _Nullable __restrict,
        void * _Nullable (* _Nonnull)(void * _Nullable),
        void * _Nullable __restrict);
#else
// 兼容 Swift
int pthread_create(pthread_t * __restrict,
        const pthread_attr_t * _Nullable __restrict,
        void *(* _Nonnull)(void *), void * _Nullable __restrict);
#endif // _PTHREAD_SWIFT_IMPORTER_NULLABILITY_COMPAT
```
&emsp;当线程创建成功时，由 `pthread_t * __restrict` 指向的内存单元被设置为新创建的线程的内容，`const pthread_attr_t * __restrict` 用于指定线程属性。新创建的线程从 `void * (* _Nonnull)(void *)` 函数地址开始运行，该函数指针指向的正是线程开启后的回调函数的起始地址，最后面的 `void * _Nullable __restrict` 则是作为它的参数，如果需要的参数不止一个，那么可以传递对象来包含不同的属性值进行传递。这种设计可以在线程创建之前就帮它准备好一些专有数据，最典型的用法就是使用 C++ 编程时的 this 指针。
&emsp;四个参数可精简总结如下:
1. 第一个参数为指向线程的指针。当一个新的线程创建成功之后，就会通过这个参数将线程的句柄返回给调用者，以便对这个线程进行管理。
2. 第二个参数用来设置线程属性，可传递 `NULL`。
3. 第三个参数是线程运行函数的起始地址。（即需要在新线程中执行的任务。该函数有一个返回值 `void *`，这个返回值可以通过 `pthread_join()` 接口获得）
4. 第四个参数是运行函数的参数，可传递 `NULL`。
### pthread_join 线程合并
&emsp;上面创建线程的内容我们大致已经清晰了，这里再回到我们的实例代码，首先打开关于 `pthread_join` 的注释，并把 `pthread_detach` 添加注释。运行程序可得到如下打印:
```c++
2020-11-13 16:33:09.516744+0800 Simple_iOS[50716:9050696] objc: 0x600000202bd0
2020-11-13 16:33:09.517021+0800 Simple_iOS[50716:9050696] 创建线程成功 🎉🎉
2020-11-13 16:33:11.518575+0800 Simple_iOS[50716:9050922] 🏃‍♀️ <NSThread: 0x600001580000>{number = 7, name = (null)} param: 0x600000202bd0
2020-11-13 16:33:11.519041+0800 Simple_iOS[50716:9050696] thread_ret: 0x600000202bd0
2020-11-13 16:33:11.519384+0800 Simple_iOS[50716:9050696] 🧑‍💻 <NSThread: 0x600001500fc0>{number = 1, name = main}
```
&emsp;保留了执行时间，看到 🎉🎉 和 🏃‍♀️ 执行时间相差 2 秒，这个 2 秒是 `run` 函数内部的 `sleep(2)` 执行时间，然后最后才打印 🧑‍💻。  可以明确的是 `thread` 阻塞了我们的主线程，等 `thread` 线程内部的 `run` 函数/任务执行完毕返回后，主线程才得以继续执行。

&emsp;这里为了做出对比可以试着运行 `pthread_detach` 的内容，看打印结果与 `pthread_join` 的区别。调用 `pthread_join(thread, (void**)&thread_ret)` 会把我们自己手动创建的 `thread` 线程与主线程进行合并。`pthread_join` 的第一个参数是新创建的 `thread` 线程句柄，第二个参数会去接收 `thread` 线程的返回值。`pthread_join` 会阻塞主进程的执行，直到合并的线程执行结束。由于 `thread` 线程在结束之后会将 `param` 返回，那么 `pthread_join` 获得的线程返回值自然也就是我们最初创建的 `objc`，输出结果中对象地址完全相同也证实了这一点(看到一些文章把线程合并描述为线程等待，例如我们这里是主线程等待 `thread` 线程，逻辑理解上和线程合并是完全一致的，要说明的一点是，一个线程不能被多个线程等待（合并），否则第一个接收到信号的线程成功返回，其余调用 `pthread_join` 的线程则返回错误代码 `ESRCH`)。看一下 `pthread_join` 函数的声明:
```c++
__API_AVAILABLE(macos(10.4), ios(2.0))
int pthread_join(pthread_t , void * _Nullable * _Nullable)
        __DARWIN_ALIAS_C(pthread_join);
```
&emsp;`pthread_join` 函数干了什么？什么是线程合并呢？

&emsp;我们首先要明确的一个问题就是什么是线程的合并。从前面的叙述中我们已经了解到了，`pthread_create` 负责创建了一个线程。那么线程也属于系统的资源，这跟内存没什么两样，而且线程本身也要占据一定的内存空间。众所周知的一个问题就是 C 或 C++ 编程中如果要通过 `malloc()` 或 `new` 分配了一块内存，就必须使用 `free()`或 `delete` 来回收这块内存，否则就会产生著名的内存泄漏问题。既然线程和内存没什么两样，那么有创建就必须得有回收，否则就会产生另外一个著名的资源泄漏问题，这同样也是一个严重的问题。那么线程的合并就是回收线程资源了。

&emsp;线程的合并是一种主动回收线程资源的方案。当一个进程或线程调用了针对其它线程的 `pthread_join` 函数，就是线程合并了。这个接口会阻塞调用进程或线程，直到被合并的线程结束为止。当被合并线程结束，`pthread_join` 接口就会回收这个线程的资源，并将这个线程的返回值返回给合并者。与线程合并相对应的另外一种线程资源回收机制是线程分离，调用接口是 `pthread_detach`，下面我们对线程分离进行分析。
### pthread_detach 线程分离
&emsp;首先看一下 `pthread_detach` 函数声明:
```c++
__API_AVAILABLE(macos(10.4), ios(2.0))
int pthread_detach(pthread_t);
```
&emsp;线程分离是将线程资源的回收工作交由系统自动来完成，也就是说当被分离的线程结束之后，系统会自动回收它的资源。因为线程分离是启动系统的自动回收机制，那么程序也就无法获得被分离线程的返回值，这就使得 `pthread_detach` 接口只要拥有一个参数就行了，那就是被分离线程句柄。

&emsp;**线程合并和线程分离都是用于回收线程资源的，可以根据不同的业务场景酌情使用。不管有什么理由，你都必须选择其中一种，否则就会引发资源泄漏的问题，这个问题与内存泄漏同样可怕。**

### pthread_attr_t 线程属性
&emsp;前面调用 `pthread_create` 函数创建线程时，第二个参数是设置线程的属性我们直接传了 `NULL`。当需要设置线程属性时我们可以传入一个 `pthread_attr_t` 指针，`pthread_attr_t` 实际是 `_opaque_pthread_attr_t` 结构体。
&emsp;我们可以使用 `pthread_attr_init` 接口初始化线程属性，使用 `pthread_attr_destroy` 接口来销毁线程属性。首先我们看一下 `pthread_attr_t` 定义。
```c++
typedef __darwin_pthread_attr_t pthread_attr_t;

typedef struct _opaque_pthread_attr_t __darwin_pthread_attr_t;

struct _opaque_pthread_attr_t {
    long __sig;
    char __opaque[__PTHREAD_ATTR_SIZE__];
};
```
&emsp;`pthread_attr_init` 函数声明:
```c++
__API_AVAILABLE(macos(10.4), ios(2.0))
int pthread_attr_init(pthread_attr_t *);
```
&emsp;`pthread_attr_destroy` 函数声明:
```c++
__API_AVAILABLE(macos(10.4), ios(2.0))
int pthread_attr_destroy(pthread_attr_t *);
```
#### 绑定属性
&emsp;


## 参考链接
**参考链接:🔗**
+ [pthread-百度百科词条](https://baike.baidu.com/item/POSIX线程?fromtitle=Pthread&fromid=4623312)
+ [pthread_create-百度百科词条](https://baike.baidu.com/item/pthread_create/5139072?fr=aladdin)
+ [在Linux中使用线程](https://blog.csdn.net/jiajun2001/article/details/12624923)
+ [iOS多线程：『pthread、NSThread』详尽总结](https://juejin.im/post/6844903556009443335)
+ [iOS 多线程系列 -- pthread](https://www.jianshu.com/p/291598217865)
+ [iOS底层原理总结 - pthreads](https://www.jianshu.com/p/4434f18c5a95)
+ [C语言多线程pthread库相关函数说明](https://www.cnblogs.com/mq0036/p/3710475.html)
+ [iOS多线程中的实际方案之一pthread](https://www.jianshu.com/p/cfc6e7d2316a)

