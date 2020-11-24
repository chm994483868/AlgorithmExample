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

&emsp;把上面的 `dispatch_object_s` 的宏定义内容全部展开后，如下:
```c++
struct dispatch_object_s {
    struct _os_object_s _as_os_obj[0]; // 长度为 0 的数组，这里可忽略
    
    // _os_object_s 是仅包含下面三个成员变量的结构体
    // const _os_object_vtable_s *os_obj_isa; 
    // int volatile os_obj_ref_cnt; 
    // int volatile os_obj_xref_cnt;
    
    const struct dispatch_object_vtable_s *do_vtable; /* must be pointer-sized */ // do_vtable 包含了对象类型和 dispatch_object_s 的操作函数
    int volatile do_ref_cnt; // 引用计数
    int volatile do_xref_cnt; // 外部引用计数，两者都为 0 时才会释放对象内存
    
    struct dispatch_object_s *volatile do_next; // do_next 表示链表的 next，（下一个 dispatch_object_s）
    struct dispatch_queue_s *do_targetq; // 目标队列，（表示当前任务要在这个队列运行，待验证）
    void *do_ctxt; // 上下文，即运行任务（其实是一个函数）的参数
    void *do_finalizer; // 最终销毁时调用的函数
};
```
&emsp;看到 `dispatch_object_s` 内部比较诡异的第一行一个长度是 0 的 `_os_object_s` 结构体数组，下面看一下 `_os_object_s` 结构体的定义。
### _os_object_s
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
&emsp;直白一点的话可以把前缀理解为 `os_obj`，即它是 `os_object` 的结构体。下面看一下它的第一个成员变量 `_os_object_vtable_s` 结构体的具体定义。
> &emsp;看下 _os_object_s 结构体的第一个成员变量 const _os_object_vtable_s *os_obj_isa。
> ```c++
> typedef struct _os_object_vtable_s {
>    _OS_OBJECT_CLASS_HEADER();
> } _os_object_vtable_s;
> ```
> &emsp;下面是 _os_object_vtable_s 结构体中的 _OS_OBJECT_CLASS_HEADER() 宏定义。
> ```c++
> #if OS_OBJECT_HAVE_OBJC_SUPPORT
> 
> #if TARGET_OS_MAC && !TARGET_OS_SIMULATOR && defined(__i386__)
> #define _OS_OBJECT_CLASS_HEADER() const void *_os_obj_objc_isa
> #else
> #define _OS_OBJECT_CLASS_HEADER() void *_os_obj_objc_class_t[5] // ⬅️ arm64/x86_64 应该是这个
> #endif
> 
> #else
> 
> #define _OS_OBJECT_CLASS_HEADER() \
>         void (*_os_obj_xref_dispose)(_os_object_t); \
>         void (*_os_obj_dispose)(_os_object_t)
>         
> #endif
> ```
> &emsp;把 const _os_object_vtable_s *os_obj_isa 展开，在 arm64/x86_64 下，os_obj_isa 是一个指向长度是 5 元素是 void * 的指针。

&emsp;下面我们看一下指向 `dispatch_object_s` 结构体的指针类型 `dispatch_object_t`，在此之前我们要扩展一个知识点：**透明联合类型**。
### DISPATCH_TRANSPARENT_UNION
&emsp;`DISPATCH_TRANSPARENT_UNION` 是用于添加 `transparent_union` 属性的宏定义。
```c++
#ifndef __cplusplus
#define DISPATCH_TRANSPARENT_UNION __attribute__((__transparent_union__))
#else
#define DISPATCH_TRANSPARENT_UNION
#endif
```
&emsp;透明联合类型削弱了 C 语言的类型检测机制，或者，换言之，它起到了类似强制类型转换的效果。考虑到在底层，类型实质上是不存在的，因此所谓的透明联合类型，也就是在一定程度上打破了类型对我们的束缚，使数据以一种更底层的角度呈现在我们面前。不过这样也弱化了 C 语言对类型的检测，由此也可能带来一些很严重的错误。详细可参考：[透明联合类型](http://nanjingabcdefg.is-programmer.com/posts/23951.html)。
### dispatch_object_t
&emsp;`dispatch_object_t` 结尾处的 `DISPATCH_TRANSPARENT_UNION` 表示它是一个透明联合体，即 `dispatch_object_t` 可以表示为指向联合体内部的任何一种类型的指针。
```c++
typedef union {
    struct _os_object_s *_os_obj;
    struct dispatch_object_s *_do; // GCD 的基类，上面我们已经对它进行详细分析
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
### dispatch_queue_s
&emsp;下面我们来看一下可能是 GCD 中最重要的一个数据结构了，队列的数据结构 `dispatch_queue_s`，前面我们看了无数次指向 `dispatch_queue_s` 结构体的指针 `dispatch_queue_t`，下面就看下队列内部的都包含哪些具体的内容吧。

&emsp;上面我们看 `dispatch_object_s` 时它的定义位于 `object_internal.h` 文件中，这次 `dispatch_queue_s` 定义在 `queue_internal.h` 文件中，大概发现了规律，看到还有 `data_internal.h`、`mach_internal.h`、`semaphore_internal.h`、`source_internal.h` 等等文件。

&emsp;`DISPATCH_ATOMIC64_ALIGN` 标记添加 8 字节对齐的属性。
```c++
#define DISPATCH_ATOMIC64_ALIGN  __attribute__((aligned(8)))
```
&emsp;`dispatch_queue_s` 结构体定义：
```c++
struct dispatch_queue_s {
    DISPATCH_QUEUE_CLASS_HEADER(queue, void *__dq_opaque1);
    /* 32bit hole on LP64 */
} DISPATCH_ATOMIC64_ALIGN;
```
### DISPATCH_QUEUE_CLASS_HEADER
&emsp;这宏定义看的真是吐血，一层套一层...
```c++
#define DISPATCH_QUEUE_CLASS_HEADER(x, __pointer_sized_field__) \

_DISPATCH_QUEUE_CLASS_HEADER(x, __pointer_sized_field__); \ // 等待展开的宏 1
/* LP64 global queue cacheline boundary */ \

unsigned long dq_serialnum; \ 
const char *dq_label; \

DISPATCH_UNION_LE(uint32_t volatile dq_atomic_flags, \ // 等待展开的宏 2
    const uint16_t dq_width, \
    const uint16_t __dq_opaque2 \
); \

dispatch_priority_t dq_priority; \
union { \
    struct dispatch_queue_specific_head_s *dq_specific_head; \
    struct dispatch_source_refs_s *ds_refs; \
    struct dispatch_timer_source_refs_s *ds_timer_refs; \
    struct dispatch_mach_recv_refs_s *dm_recv_refs; \
    struct dispatch_channel_callbacks_s const *dch_callbacks; \
}; \
int volatile dq_sref_cnt
```
### _DISPATCH_QUEUE_CLASS_HEADER
&emsp;前面我们已经看到在 arm64/x86_64 下，`OS_OBJECT_HAVE_OBJC1` 值为 0。所以 `_DISPATCH_QUEUE_CLASS_HEADER` 宏定义如下：
```c++
#define _DISPATCH_QUEUE_CLASS_HEADER(x, __pointer_sized_field__) \
DISPATCH_OBJECT_HEADER(x); \ // 这个宏定义同 "_DISPATCH_OBJECT_HEADER(object);"，这里是 "_DISPATCH_OBJECT_HEADER(queue);"，等下直接展开。
__pointer_sized_field__; \

DISPATCH_UNION_LE(uint64_t volatile dq_state, \
        dispatch_lock dq_state_lock, \
        uint32_t dq_state_bits \
)
```
### DISPATCH_OBJECT_HEADER
```c++
#define DISPATCH_OBJECT_HEADER(x) \
    struct dispatch_object_s _as_do[0]; \ // 长度为 0 的数组，暂时可忽略
    _DISPATCH_OBJECT_HEADER(x) // 这里对应上面 dispatch_object_s 结构体内部唯一的一行宏定义: "_DISPATCH_OBJECT_HEADER(object);" 这里则是："_DISPATCH_OBJECT_HEADER(queue);"
```
&emsp;看到这里有些领悟，`dispatch_queue_s` 结构体的前面几个成员变量的布局用到的宏定义和我们上面学习 `dispatch_object_s` 结构体时用到的是一样。即等下 `dispatch_queue_s` 结构体展开其前面几个成员变量和 `dispatch_object_s` 如出一辙的，这里类似是在模拟继承机制，例如可以这样理解 `dispatch_queue_s` 前面的几个成员变量继承自 `dispatch_object_s`。
### DISPATCH_UNION_LE
&emsp;`DISPATCH_UNION_LE` 宏定义包含的内容有两层的，首先是进行一个断言，然后是生成一个联合体，断言和下面的联合体内部转换几乎是相同的，都是使用相同的宏定义内容。
```c++
#define DISPATCH_UNION_LE(alias, ...) \
        DISPATCH_UNION_ASSERT(alias, DISPATCH_CONCAT(DISPATCH_STRUCT_LE, \
                DISPATCH_COUNT_ARGS(__VA_ARGS__))(__VA_ARGS__)) \
        union { alias; DISPATCH_CONCAT(DISPATCH_STRUCT_LE, \
                DISPATCH_COUNT_ARGS(__VA_ARGS__))(__VA_ARGS__); }

// DISPATCH_UNION_LE 内部嵌套的宏定义过多，这里我们以一个例子分析一下：
// DISPATCH_UNION_LE(uint64_t volatile dq_state, dispatch_lock dq_state_lock, uint32_t dq_state_bits)

// 1. DISPATCH_UNION_LE 里面的 DISPATCH_COUNT_ARGS(__VA_ARGS__) 是统计参数个数，
//    然后返回一个 _参数个数，假设参数个数是 2，可直接把 DISPATCH_COUNT_ARGS(__VA_ARGS__) 转换为 _2 如下：（暂时保留 __VA_ARGS__ 和 alias）

DISPATCH_UNION_ASSERT(alias, DISPATCH_CONCAT(DISPATCH_STRUCT_LE, _2)(__VA_ARGS__)) \
union { alias; DISPATCH_CONCAT(DISPATCH_STRUCT_LE, _2)(__VA_ARGS__); }

// 2. 然后是 DISPATCH_CONCAT(DISPATCH_STRUCT_LE, _2)，它是较简单的只是进行参数拼接，可继续转换如下：（暂时保留 __VA_ARGS__ 和 alias）

DISPATCH_UNION_ASSERT(alias, DISPATCH_STRUCT_LE_2(__VA_ARGS__)) \
union { alias; DISPATCH_STRUCT_LE_2(__VA_ARGS__); }
        
// 3. 然后是 DISPATCH_STRUCT_LE_2(__VA_ARGS__)，这里开始替换 __VA_ARGS__，可继续转换如下：

DISPATCH_UNION_ASSERT(uint64_t volatile dq_state,         
                      struct {
                          dispatch_lock dq_state_lock;
                          uint32_t dq_state_bits;
                      };
                     )
                     
union { 
        uint64_t volatile dq_state;
        struct {
            dispatch_lock dq_state_lock;
            uint32_t dq_state_bits;
        };
      }

// 4. DISPATCH_UNION_ASSERT 仅是一个断言，且 typedef uint32_t dispatch_lock; 即双方都是 64 位，8 个字节，那么宏定义全部展开就只剩下:
union { 
        uint64_t volatile dq_state;
        struct {
            dispatch_lock dq_state_lock;
            uint32_t dq_state_bits;
        };
      }
```
### DISPATCH_STRUCT_LE_2
```c++
#if BYTE_ORDER == LITTLE_ENDIAN
#define DISPATCH_STRUCT_LE_2(a, b)        struct { a; b; }
#define DISPATCH_STRUCT_LE_3(a, b, c)     struct { a; b; c; }
#define DISPATCH_STRUCT_LE_4(a, b, c, d)  struct { a; b; c; d; }
#else
#define DISPATCH_STRUCT_LE_2(a, b)        struct { b; a; }
#define DISPATCH_STRUCT_LE_3(a, b, c)     struct { c; b; a; }
#define DISPATCH_STRUCT_LE_4(a, b, c, d)  struct { d; c; b; a; }
#endif
```
### DISPATCH_UNION_ASSERT
&emsp;`DISPATCH_UNION_ASSERT` 是一个断言联合体，断言的内容是判断仅有一个成员变量 `alias` 的结构体内存字节长度是否等于 `st` 的内存字节长度。
```c++
#if __has_feature(c_startic_assert)
#define DISPATCH_UNION_ASSERT(alias, st) _Static_assert(sizeof(struct { alias; }) == sizeof(st), "bogus union");
#else
#define DISPATCH_UNION_ASSERT(alias, st)
#endif
```
### DISPATCH_CONCAT
&emsp;`DISPATCH_CONCAT` 宏较简单，只是把宏中的两个参数拼接在一起。
```c++
#define DISPATCH_CONCAT(x,y) DISPATCH_CONCAT1(x,y)
#define DISPATCH_CONCAT1(x,y) x ## y
```
### DISPATCH_COUNT_ARGS
&emsp;`DISPATCH_COUNT_ARGS` 统计宏定义中的参数个数，例如：`DISPATCH_COUNT_ARGS` 中有两个参数时返回 `_2`，有三个参数时返回 `_3`。
```c++
#define DISPATCH_COUNT_ARGS(...) DISPATCH_COUNT_ARGS1(, ## __VA_ARGS__, _8, _7, _6, _5, _4, _3, _2, _1, _0)
#define DISPATCH_COUNT_ARGS1(z, a, b, c, d, e, f, g, h, cnt, ...) cnt
```
&emsp;以上是 `dispatch_queue_s` 结构中涉及的宏定义，下面全部展开看下 `dispatch_queue_s` 包含的内容：
```c++
struct dispatch_queue_s {
    struct dispatch_object_s _as_do[0];
    struct _os_object_s _as_os_obj[0];

    const struct dispatch_queue_vtable_s *do_vtable; /* must be pointer-sized */
    int volatile do_ref_cnt;
    int volatile do_xref_cnt;

    struct dispatch_queue_s *volatile do_next;
    struct dispatch_queue_s *do_targetq;
    void *do_ctxt;
    void *do_finalizer

    void *__dq_opaque1;
    
    union { 
            uint64_t volatile dq_state;
            struct {
                dispatch_lock dq_state_lock;
                uint32_t dq_state_bits;
            };
          }
    /* LP64 global queue cacheline boundary */ 
    
    unsigned long dq_serialnum;
    const char *dq_label;
    
    union { 
            uint32_t volatile dq_atomic_flags;
            struct {
                const uint16_t dq_width;
                const uint16_t __dq_opaque2;
            };
          }
    
    dispatch_priority_t dq_priority;
    union {
        struct dispatch_queue_specific_head_s *dq_specific_head;
        struct dispatch_source_refs_s *ds_refs;
        struct dispatch_timer_source_refs_s *ds_timer_refs;
        struct dispatch_mach_recv_refs_s *dm_recv_refs;
        struct dispatch_channel_callbacks_s const *dch_callbacks;
    };
    int volatile dq_sref_cnt
    
    /* 32bit hole on LP64 */
} DISPATCH_ATOMIC64_ALIGN;
```
&emsp;前面部分几个成员变量完全照搬 `dispatch_object_s` 结构体的成员变量，












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
+ [c/c++:计算可变参数宏 __VA_ARGS__ 的参数个数](https://blog.csdn.net/10km/article/details/80760533)
