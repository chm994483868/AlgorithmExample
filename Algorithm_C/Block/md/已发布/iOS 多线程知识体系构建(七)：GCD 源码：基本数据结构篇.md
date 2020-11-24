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
&emsp;`dispatch_object_s` 是 GCD 的基础结构体。其中涉及连续的多个宏定义（看连续的宏定义真的好烦呀），下面一起来看一下。
```c++
struct dispatch_object_s {
    _DISPATCH_OBJECT_HEADER(object);
};
```
### _DISPATCH_OBJECT_HEADER
&emsp;`dispatch_object_s` 结构体内部唯一一个 `_DISPATCH_OBJECT_HEADER` 宏定义。
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
&emsp;`_DISPATCH_OBJECT_HEADER` 内部的一个 `OS_OBJECT_STRUCT_HEADER` 宏定义。
```c++
#if TARGET_OS_MAC && !TARGET_OS_SIMULATOR && defined(__i386__)
#define OS_OBJECT_HAVE_OBJC1 1
#else
#define OS_OBJECT_HAVE_OBJC1 0 // ⬅️ 当前 x86_64/arm64 平台下
#endif

#if OS_OBJECT_HAVE_OBJC1
#define OS_OBJECT_STRUCT_HEADER(x) \
    _OS_OBJECT_HEADER(\
    const void *_objc_isa, \
    do_ref_cnt, \
    do_xref_cnt); \
    const struct x##_vtable_s *do_vtable
#else

// ⬇️ 当前平台下取这里（arm64 和 x86_64 下）
#define OS_OBJECT_STRUCT_HEADER(x) \
    _OS_OBJECT_HEADER(\
    const struct x##_vtable_s *do_vtable, \
    do_ref_cnt, \
    do_xref_cnt)
#endif
```
### _OS_OBJECT_HEADER
&emsp;紧接着 `OS_OBJECT_STRUCT_HEADER` 内部的 `_OS_OBJECT_HEADER` 宏定义，可看到是 `OS_OBJECT` 的头部的三个成员变量。
```c++
#define _OS_OBJECT_HEADER(isa, ref_cnt, xref_cnt) \
isa; /* must be pointer-sized */ \ // isa 必须是指针大小
int volatile ref_cnt; \ // 引用计数
int volatile xref_cnt // 外部引用计数，两者都为 0 时，对象才能释放
```

&emsp;把上面的 `dispatch_object_s` 的宏定义内容全部展开替换后，如下:
```c++
struct dispatch_object_s {
    struct _os_object_s _as_os_obj[0]; // 长度为 0 的数组，这里可忽略
    
    const struct dispatch_object_vtable_s *do_vtable; /* must be pointer-sized */ // do_vtable 包含了对象类型和 dispatch_object_s 的操作函数
    int volatile do_ref_cnt; // 引用计数
    int volatile do_xref_cnt; // 外部引用计数，两者都为 0 时才会释放对象内存
    
    struct dispatch_object_s *volatile do_next; // do_next 表示链表的 next，（下一个 dispatch_object_s）
    struct dispatch_queue_s *do_targetq; // 目标队列，表示当前任务要在这个队列运行
    void *do_ctxt; // 上下文，即运行任务（其实是一个函数）的参数
    void *do_finalizer; // 最终销毁时调用的函数
};
```

&emsp;下面我们接着看一下 `dispatch_object_s` 内部的一些结构体类型的变量的具体定义是什么，首先看第一个 `_os_object_s` 结构体。
### _os_object_s
&emsp;上面的宏定义已经全部展开，看到比较诡异的第一行，一个长度是 0 的 `_os_object_s` 结构体数组，下面看一下 `_os_object_s` 结构体定义。
```c++
typedef struct _os_object_s {
    _OS_OBJECT_HEADER(
    const _os_object_vtable_s *os_obj_isa,
    os_obj_ref_cnt,
    os_obj_xref_cnt);
} _os_object_s;

// 把 _OS_OBJECT_HEADER 展开则是:
typedef struct _os_object_s {
    const _os_object_vtable_s *os_obj_isa; 
    int volatile os_obj_ref_cnt; 
    int volatile os_obj_xref_cnt;
} _os_object_s;
```
&emsp;直白一点的话可以把前缀理解为 `os_obj`。
### _os_object_vtable_s
&emsp;`_os_object_s` 结构体的第一个成员变量 `const _os_object_vtable_s *os_obj_isa`。
```c++
typedef struct _os_object_vtable_s {
    _OS_OBJECT_CLASS_HEADER();
} _os_object_vtable_s;
```
###  


### dispatch_object_t
```c++
typedef union {
    struct _os_object_s *_os_obj;
    struct dispatch_object_s *_do; // GCD 的基类
    struct dispatch_queue_s *_dq; // 任务队列（我们创建的队列都是这个类型，不管是串行队列还是并行队列）
    struct dispatch_queue_attr_s *_dqa; // 任务队列的属性，包含了任务队列里面的一些操作函数，可以表明这个任务队列是串行队列还是并发队列
    struct dispatch_group_s *_dg; // GCD 的 group
    struct dispatch_source_s *_ds; // GCD 的 source，可以监测内核事件，文字读写事件和 socket 通信事件
    struct dispatch_channel_s *_dch;
    struct dispatch_mach_s *_dm;
    struct dispatch_mach_msg_s *_dmsg;
    struct dispatch_semaphore_s *_dsema; // 信号量，如果了解过 pthread 都知道，信号量可以用来调度线程
    struct dispatch_data_s *_ddata;
    struct dispatch_io_s *_dchannel;
} dispatch_object_t DISPATCH_TRANSPARENT_UNION;
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
+ [GCD源码分析（一）](https://www.jianshu.com/p/bd629d25dc2e)
+ [GCD-源码分析](https://www.jianshu.com/p/866b6e903a2d)
+ [GCD底层源码分析](https://www.jianshu.com/p/4ef55563cd14)
+ [GCD源码吐血分析(1)——GCD Queue](https://blog.csdn.net/u013378438/article/details/81031938)
