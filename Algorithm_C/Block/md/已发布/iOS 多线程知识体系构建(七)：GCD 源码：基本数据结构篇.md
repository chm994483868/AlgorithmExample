# iOS 多线程知识体系构建(七)：GCD 源码：基本数据结构篇

> &emsp;由本篇正式进入 GCD 源码。

> &emsp;首先下载源码，看到当前最新版本是：[libdispatch-1173.40.5](https://opensource.apple.com/tarballs/libdispatch/)。看到项目中 Dispatch Public Headers 文件夹正是我们前几天看的一众 .h 文件，然后下面的 Dispatch Source 文件夹内正是各个 .h 所对应的实现文件（.c 文件，GCD 完全由 C 语言实现），倍感亲切，那么就此开始吧！⛽️⛽️

&emsp;那么我们还由基础的数据结构定义开始，例如 `dispatch_object_t/s`、`dispatch_queue_t/s`、`dispatch_group_t/s`等等，是我们之前见的很多次的指针类型和结构体类型，这里首先要对它们做出区分，其中 `**_t` 一般都是用 `typedef` 所定义的指向 `**_s` 结构体的指针，例如: `typedef struct dispatch_group_s *dispatch_group_t`，其中 `dispatch_group_t` 是指向 `dispatch_group_s` 结构体的指针。（其中结尾处的 `t` 和 `s` 分别来自 `typedef` 和 `struct` 的首字母）

&emsp;当然如果对前面的文章还有印象的话一定记得，其实它们的声明都来自 `DISPATCH_DECL` 宏:
```c++
#define DISPATCH_DECL(name) typedef struct name##_s *name##_t
```
&emsp;这是 `DISPATCH_DECL` 在 C（Plain C）环境下的宏定义，其中还有 C++/Objective-c/Swift 环境下的，但这里我们仅看 C 环境下的。前面几篇文章在 .h 中我们只看到的结构体的名字而完全没有看到它们的具体定义，那么就去 libdispatch 源码中找它们的具体定义吧！
## dispatch_object_s 
&emsp;`dispatch_object_s` 是 GCD 的基础结构体，其中涉及连续的多个宏定义（看宏定义真的好烦），下面一起来看一下。
```c++
struct dispatch_object_s {
    _DISPATCH_OBJECT_HEADER(object);
};
```
### _DISPATCH_OBJECT_HEADER
```c++
#define _DISPATCH_OBJECT_HEADER(x) \
struct _os_object_s _as_os_obj[0]; \ ⬅️ 这里是一个长度为 0 的数组，不占用任何内存，暂时可以忽略

OS_OBJECT_STRUCT_HEADER(dispatch_##x); \ ⬅️ 这里需要 OS_OBJECT_STRUCT_HEADER 宏展开

struct dispatch_##x##_s *volatile do_next; \
struct dispatch_queue_s *do_targetq; \
void *do_ctxt; \
void *do_finalizer
```
### OS_OBJECT_STRUCT_HEADER
```c++
#if TARGET_OS_MAC && !TARGET_OS_SIMULATOR && defined(__i386__)
#define OS_OBJECT_HAVE_OBJC1 1
#else
#define OS_OBJECT_HAVE_OBJC1 0 // ⬅️ 当前 x86_64 平台下
#endif

#if OS_OBJECT_HAVE_OBJC1
#define OS_OBJECT_STRUCT_HEADER(x) \
    _OS_OBJECT_HEADER(\
    const void *_objc_isa, \
    do_ref_cnt, \
    do_xref_cnt); \
    const struct x##_vtable_s *do_vtable
#else

// ⬇️ 当前平台下取这里 iOS 和 x86_64 下
#define OS_OBJECT_STRUCT_HEADER(x) \
    _OS_OBJECT_HEADER(\
    const struct x##_vtable_s *do_vtable, \
    do_ref_cnt, \
    do_xref_cnt)
#endif
```
### _OS_OBJECT_HEADER
```c++
#define _OS_OBJECT_HEADER(isa, ref_cnt, xref_cnt) \
isa; /* must be pointer-sized */ \ // isa 必须是指针大小
int volatile ref_cnt; \ // 引用计数
int volatile xref_cnt // 外部引用计数，两者都为 0 时，对象才能释放
```
&emsp;把上面的宏定义内容全部展开后，`dispatch_object_s` 结构体定义如下:
```c++
struct dispatch_object_s {
    struct _os_object_s _as_os_obj[0]; // 长度为 0 的数组，这里可忽略
    
    const struct dispatch_object_vtable_s *do_vtable; /* must be pointer-sized */
    int volatile do_ref_cnt;
    int volatile do_xref_cnt;
    
    struct dispatch_object_s *volatile do_next;
    struct dispatch_queue_s *do_targetq;
    void *do_ctxt;
    void *do_finalizer
};
```





## 参考链接
**参考链接:🔗**
+ [libdispatch苹果源码](https://opensource.apple.com/tarballs/libdispatch/)
+ [GCD源码分析1 —— 开篇](http://lingyuncxb.com/2018/01/31/GCD源码分析1%20——%20开篇/)
+ [扒了扒libdispatch源码](http://joeleee.github.io/2017/02/21/005.扒了扒libdispatch源码/)
+ [GCD源码分析](https://developer.aliyun.com/article/61328)
+ [关于GCD开发的一些事儿](https://www.jianshu.com/p/f9e01c69a46f)
+ [GCD 深入理解：第一部分](https://github.com/nixzhu/dev-blog/blob/master/2014-04-19-grand-central-dispatch-in-depth-part-1.md)
+ [dispatch_once 详解](https://www.jianshu.com/p/4fd27f1db63d)
+ [透明联合类型](http://nanjingabcdefg.is-programmer.com/posts/23951.html)
+ [变态的libDispatch结构分析-dispatch_object_s](https://blog.csdn.net/passerbysrs/article/details/18228333?utm_source=blogxgwz2)


+ [深入浅出 GCD 之基础篇](https://xiaozhuanlan.com/topic/9168375240)
+ [从源码分析Swift多线程—DispatchGroup](http://leevcan.com/2020/05/30/从源码分析Swift多线程—DispatchGroup/)


&emsp;<dispatch/block.h> 文件到这里就全部看完了。下面接着看另一个文件 <dispatch/io.h>，
## <dispatch/io.h>
&emsp;Dispatch I/O 对文件描述符（file descriptors）提供流和随机访问异步读取和写入操作。可以从文件描述符（file descriptor）中将一个或多个 dispatch I/O channels 创建为 `DISPATCH_IO_STREAM` 类型或 `DISPATCH_IO_RANDOM` 类型。创建通道后，应用程序可以安排异步读取和写入操作。

&emsp;应用程序可以在 dispatch I/O channel 上设置策略，以指示长时间运行的操作所需的 I/O 处理程序频率。

&emsp;Dispatch I/O 还为 I/O 缓冲区提供了内存管理模型，可避免在通道之间进行管道传输时不必要的数据复制。Dispatch I/O 监视应用程序的整体内存压力和 I/O 访问模式，以优化资源利用率。
