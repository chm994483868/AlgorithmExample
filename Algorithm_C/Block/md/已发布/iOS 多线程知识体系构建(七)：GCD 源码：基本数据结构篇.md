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
> // 这里用两个函数指针，（_os_object_t 应该是 _os_object_s 的指针）
> // 这里的函数指针大概是处理对象的引用和对象本身的操作吗？
> #define _OS_OBJECT_CLASS_HEADER() \
>         void (*_os_obj_xref_dispose)(_os_object_t); \
>         void (*_os_obj_dispose)(_os_object_t)
>         
> #endif
> ```
> &emsp;把 const _os_object_vtable_s *os_obj_isa 展开，在 arm64/x86_64 下，os_obj_isa 是一个指向长度是 5 元素是 void * 的指针。

&emsp;在 `dispatch_object_s` 结构体第一个成员变量是 `const struct dispatch_object_vtable_s *do_vtable`，这里的 `dispatch_object_vtable_s` 结构体也涉及到另外一个宏定义 `DISPATCH_CLASS_DECL_BARE` 下面来看一下。

### DISPATCH_CLASS_DECL_BARE
&emsp;
```c++
#define DISPATCH_CLASS_DECL_BARE(name, cluster) \
        OS_OBJECT_CLASS_DECL(dispatch_##name, \
        DISPATCH_##cluster##_VTABLE_HEADER(dispatch_##name))
```
```c++
// 1⃣️
DISPATCH_CLASS_DECL_BARE(object, OBJECT); 
```
&emsp;把上面宏定义展开如下:
```c++
// 2⃣️
OS_OBJECT_CLASS_DECL(dispatch_object, \
DISPATCH_OBJECT_VTABLE_HEADER(dispatch_object))
```
### DISPATCH_OBJECT_VTABLE_HEADER
&emsp;`DISPATCH_OBJECT_VTABLE_HEADER` 会根据 `USE_OBJC` 环境做不同的定义，当前是 `USE_OBJC` 为真的环境。
```c++
#if USE_OBJC

#define DISPATCH_OBJECT_VTABLE_HEADER(x) \
    unsigned long const do_type; \
    void (*const do_dispose)(struct x##_s *, bool *allow_free); \
    size_t (*const do_debug)(struct x##_s *, char *, size_t); \
    void (*const do_invoke)(struct x##_s *, dispatch_invoke_context_t, \
            dispatch_invoke_flags_t)
#else

#define DISPATCH_OBJECT_VTABLE_HEADER(x) \
    unsigned long const do_type; \
    const char *const do_kind; \
    void (*const do_dispose)(struct x##_s *, bool *allow_free); \
    size_t (*const do_debug)(struct x##_s *, char *, size_t); \
    void (*const do_invoke)(struct x##_s *, dispatch_invoke_context_t, \
            dispatch_invoke_flags_t)
#endif
```
### OS_OBJECT_CLASS_DECL
&emsp;定义一个新的适当的 “类”。
```c++
// define a new proper class
#define OS_OBJECT_CLASS_DECL(name, ...) \
        struct name##_s; \
        struct name##_extra_vtable_s { \
            __VA_ARGS__; \
        }; \
        struct name##_vtable_s { \
            _OS_OBJECT_CLASS_HEADER(); \
            struct name##_extra_vtable_s _os_obj_vtable; \
        }; \
        OS_OBJECT_EXTRA_VTABLE_DECL(name, name) \
        
        // OS_OBJECT_CLASS_SYMBOL 宏仅一行
        // #if OS_OBJECT_HAVE_OBJC_SUPPORT
        // #define OS_OBJECT_CLASS_SYMBOL(name) OS_##name##_class
        // #endif

        // #if USE_OBJC
        // #else
        // #define OS_OBJECT_CLASS_SYMBOL(name) _##name##_vtable
        // #endif
        
        extern const struct name##_vtable_s OS_OBJECT_CLASS_SYMBOL(name) \
                __asm__(OS_OBJC_CLASS_RAW_SYMBOL_NAME(OS_OBJECT_CLASS(name)))
```
&emsp;那么继续把上面 2⃣️ 处的宏定义继续展开如下：
```c++
// 3⃣️
struct dispatch_object_s; \
struct dispatch_object_extra_vtable_s {
    unsigned long const do_type;
    
    // 下面是三个函数指针
    void (*const do_dispose)(struct dispatch_object_s *, bool *allow_free);
    size_t (*const do_debug)(struct dispatch_object_s *, char *, size_t);
    void (*const do_invoke)(struct dispatch_object_s *, dispatch_invoke_context_t, dispatch_invoke_flags_t)
};

struct dispatch_object_vtable_s {
    void *_os_obj_objc_class_t[5];
    struct dispatch_object_extra_vtable_s _os_obj_vtable;
};

// 下面三行宏定义内容可忽略
OS_OBJECT_EXTRA_VTABLE_DECL(dispatch_object, dispatch_object) \
extern const struct dispatch_object_vtable_s OS_OBJECT_CLASS_SYMBOL(dispatch_object) \
        __asm__(OS_OBJC_CLASS_RAW_SYMBOL_NAME(OS_OBJECT_CLASS(dispatch_object)))
```
&emsp;看到这里，我们大概就能明白 `"const struct dispatch_object_vtable_s *do_vtable; /* must be pointer-sized */ // do_vtable 包含了对象类型和 dispatch_object_s 的操作函数"`  整行的含义了。

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
    struct dispatch_queue_s *_dq; // 队列（我们创建的队列都是这个类型，不管是串行队列还是并行队列）
    struct dispatch_queue_attr_s *_dqa; // 队列的属性，包含了队列里面的一些操作函数，可以表明这个队列是串行队列还是并发队列
    struct dispatch_group_s *_dg; // GCD 的 group
    struct dispatch_source_s *_ds; // GCD 的 source，可以监测内核事件，文字读写事件和 socket 通信事件
    struct dispatch_channel_s *_dch;
    struct dispatch_mach_s *_dm;
    struct dispatch_mach_msg_s *_dmsg;
    struct dispatch_semaphore_s *_dsema; // 信号量，如果了解过 pthread 都知道，信号量可以用来调度线程
    struct dispatch_data_s *_ddata;
    struct dispatch_io_s *_dchannel;
    
    struct dispatch_continuation_s *_dc; // 任务，（dispatch_aync 的 block 会封装成这个数据结构）
    struct dispatch_sync_context_s *_dsc;
    struct dispatch_operation_s *_doperation;
    struct dispatch_disk_s *_ddisk;
    struct dispatch_workloop_s *_dwl;
    struct dispatch_lane_s *_dl;
    struct dispatch_queue_static_s *_dsq;
    struct dispatch_queue_global_s *_dgq;
    struct dispatch_queue_pthread_root_s *_dpq;
    dispatch_queue_class_t _dqu;
    dispatch_lane_class_t _dlu;
    uintptr_t _do_value;
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
    struct dispatch_object_s _as_do[0]; // 长度为 0 的数组，可忽略，同时也在暗示着结构体内存空间中的数据类型
    struct _os_object_s _as_os_obj[0];

    const struct dispatch_queue_vtable_s *do_vtable; /* must be pointer-sized */
    int volatile do_ref_cnt;
    int volatile do_xref_cnt;

    struct dispatch_queue_s *volatile do_next;
    struct dispatch_queue_s *do_targetq;
    void *do_ctxt;
    void *do_finalizer
    
    // ⬆️ 
    // DISPATCH_OBJECT_HEADER(queue); 这里是分界，可以把以上内容理解为继承自 dispatch_object_s。 
    
    void *__dq_opaque1;
    union { 
        uint64_t volatile dq_state;
        struct {
            // typedef uint32_t dispatch_lock;
            // dispatch_lock 是 uint32_t 类型
            dispatch_lock dq_state_lock;
            uint32_t dq_state_bits;
        };
    };
    
    /* LP64 global queue cacheline boundary */ 
    unsigned long dq_serialnum; // 序列号，如主队列是 1
    const char *dq_label; // 队列标签或者队列名
    union { 
        uint32_t volatile dq_atomic_flags;
        struct {
            const uint16_t dq_width;
            const uint16_t __dq_opaque2;
        };
    };
    // typedef uint32_t dispatch_priority_t;
    // 在 priority.h 文件中，看到 dispatch_priority_t 是 uint32_t 类型  
    dispatch_priority_t dq_priority; // 队列优先级
    union { // 联合体
        struct dispatch_queue_specific_head_s *dq_specific_head;
        struct dispatch_source_refs_s *ds_refs;
        struct dispatch_timer_source_refs_s *ds_timer_refs;
        struct dispatch_mach_recv_refs_s *dm_recv_refs;
        struct dispatch_channel_callbacks_s const *dch_callbacks;
    };
    int volatile dq_sref_cnt; // 
    
    /* 32bit hole on LP64 */
} DISPATCH_ATOMIC64_ALIGN;
```
&emsp;细心观察会发现前面几个成员变量几乎和 `dispatch_object_s` 结构体的成员变量相同，它们都是来自 `_DISPATCH_OBJECT_HEADER` 宏展开，一个是 `_DISPATCH_OBJECT_HEADER(object)` 一个是 `_DISPATCH_OBJECT_HEADER(queue)`，可能看它的命名大概也看出了一些端倪“调度对象头部”，其实这里大概是在模拟继承，如 `dispatch_queue_s` 继承自 `dispatch_object_s`，那么头部的一些成员变量自然也要继承自 `dispatch_object_s` 了。

&emsp;下面我们顺着 `dispatch_object_t` 联合体内部不同成员变量的顺序，来看下它们的具体定义。
### dispatch_queue_attr_s
&emsp;`dispatch_queue_attr_s` 结构体用来表示队列的属性，包含了队列里面的一些操作函数，可以表明这个队列是串行队列还是并发队列等信息。

&emsp;`dispatch_queue_attr_s` 同样也是定义在 queue_internal.h 文件中。
```c++
struct dispatch_queue_attr_s {
    OS_OBJECT_STRUCT_HEADER(dispatch_queue_attr);
};
```
&emsp;把内部的 `OS_OBJECT_STRUCT_HEADER` 展开的话是:
```c++
struct dispatch_queue_attr_s {
    _OS_OBJECT_HEADER(\
    const struct dispatch_queue_attr_vtable_s *do_vtable, \
    do_ref_cnt, \
    do_xref_cnt);
    
    const struct dispatch_queue_attr_vtable_s *do_vtable;
    int volatile do_ref_cnt;
    int volatile do_xref_cnt;
    
};
```
&emsp;再把 `_OS_OBJECT_HEADER` 展开的话是:
```c++
struct dispatch_queue_attr_s {
    const struct dispatch_queue_attr_vtable_s *do_vtable;
    int volatile do_ref_cnt;
    int volatile do_xref_cnt;
};
```
&emsp;看到了熟悉的三个成员变量（`dispatch_object_s` 的前三个）。看到这里可能会迷惑，不是说好的 `dispatch_queue_attr_s` 是描述队列属性的数据结构吗，怎么内部就只有 “继承” 自 `dispatch_object_s` 的三个成员变量。实际描述队列的属性的结构体是 `dispatch_queue_attr_info_s/t`，
#### dispatch_queue_attr_info_t
&emsp;看到 `dispatch_queue_attr_info_s` 内部使用了位域来表示不同的值，来节省内存占用。
```c++
typedef struct dispatch_queue_attr_info_s {

    // typedef uint32_t dispatch_qos_t;
    
    dispatch_qos_t dqai_qos : 8; //（表示线程优先级）
    int      dqai_relpri : 8; //（表示优先级的偏移）
    uint16_t dqai_overcommit:2; // 
    uint16_t dqai_autorelease_frequency:2; // （自动释放频率）
    uint16_t dqai_concurrent:1; // 表示队列是并发队列还是串行队列
    uint16_t dqai_inactive:1; // 表示当前队列是否是活动状态
} dispatch_queue_attr_info_t;
```
&emsp;其实这里队列属性相关的内容包含更复杂的结构，在代码部分，看到用 `#pragma mark dispatch_queue_attr_t` 定义了一个区域的代码都与队列属性有关，下面我们把该区域的代码都看一遍。
#### DISPATCH_CLASS_DECL
&emsp;
```c++
DISPATCH_CLASS_DECL(queue_attr, OBJECT);
```
```c++
#define DISPATCH_CLASS_DECL(name, cluster) \
        _OS_OBJECT_DECL_PROTOCOL(dispatch_##name, dispatch_object) \
        _OS_OBJECT_CLASS_IMPLEMENTS_PROTOCOL(dispatch_##name, dispatch_##name) \
        DISPATCH_CLASS_DECL_BARE(name, cluster)
```
&emsp;上面宏展开:
```c++
// 1⃣️：
_OS_OBJECT_DECL_PROTOCOL(dispatch_queue_attr, dispatch_object) \
_OS_OBJECT_CLASS_IMPLEMENTS_PROTOCOL(dispatch_queue_attr, dispatch_queue_attr) \
DISPATCH_CLASS_DECL_BARE(queue_attr, OBJECT)
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
+ [c/c++:计算可变参数宏 __VA_ARGS__ 的参数个数](https://blog.csdn.net/10km/article/details/80760533)
