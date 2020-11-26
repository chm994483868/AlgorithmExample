# iOS 多线程知识体系构建(七)：GCD 源码：基本数据结构篇

> &emsp;由本篇正式进入 GCD 源码。

> &emsp;首先下载源码，看到当前最新版本是：[libdispatch-1173.40.5](https://opensource.apple.com/tarballs/libdispatch/)。下载完成打开项目看到其中 Dispatch Public Headers 文件夹正是我们前几天看的一众 .h 文件，然后下面的 Dispatch Source 文件夹内包含了各个 .h 所对应的实现文件（.c 文件，GCD 完全由 C 语言实现），倍感亲切，那么就此开始吧！⛽️⛽️

&emsp;那么我们还由基础的数据结构定义开始，例如 `dispatch_object_t/s`、`dispatch_queue_t/s`、`dispatch_group_t/s`等等，是我们之前见的很多次的指针类型和结构体类型，这里首先要对它们做出区分，其中 `**_t` 一般都是用 typedef 所定义的指向 `**_s` 结构体的指针，例如: `typedef struct dispatch_group_s *dispatch_group_t`，其中 `dispatch_group_t` 是指向 `dispatch_group_s` 结构体的指针。（其中结尾处的 `t` 和 `s` 分别来自 `typedef` 和 `struct` 的首字母）

&emsp;当然如果对前面的文章还有印象的话一定记得，其实它们的声明都来自 `DISPATCH_DECL` 宏:
```c++
#define DISPATCH_DECL(name) typedef struct name##_s *name##_t
```

&emsp;这是 `DISPATCH_DECL` 在 C（Plain C）环境下的宏定义，其中还有 C++/Objective-c/Swift 环境下的，但这里我们仅看 C 环境下的。在前面几篇文章的 .h 中我们只看到了各个结构体的名字而完全没有看到它们的具体定义是什么，那么现在就去 libdispatch 源码中找它们的具体定义吧！

&emsp;开始之前我们首先需要一些概念的上的认识。GCD 是由 C 语言实现的，C 语言作为面向过程的编程语言，它是没有类的概念的，那么我们想以面向对象的编程思想来实现 GCD 内部的各种“类”以及它们的继承关系该如何来做呢，看了前面的内容我们大概猜到了是运用结构体来模拟类（毕竟我们高级语言中的类和对象其本质也都是用结构体来实现的）。

&emsp;那么“继承关系”呢，这里是首先定义了基类的结构体，然后需要继承时，则是把基类结构体的成员变量直接放在子类结构体头部平铺展开，为了“易读和不显臃肿”，apple 定义了大量的宏，需要继承谁时直接在子类结构的头部放一个基类结构体的宏，在阅读时我们则需要把这些宏全部展开，前面 .h 中的内容仅是一些 `**_t` 的宏定义的展开就看的焦头烂额了，这下 `**_s` 的宏展开才是真正的告诉我们什么叫焦头烂额...
## _os_object_s
&emsp;`_os_object_s` 结构体内部的内容不多，它是作为 GCD 的基类存在的，它正是 `dispatch_object_s` 结构体的“父类”，下面看下它都包含哪些内容。
```c++
typedef struct _os_object_s {
    _OS_OBJECT_HEADER(
    const _os_object_vtable_s *os_obj_isa,
    os_obj_ref_cnt,
    os_obj_xref_cnt);
} _os_object_s;

// 把 _OS_OBJECT_HEADER 展开则是:
typedef struct _os_object_s {
    const _os_object_vtable_s *os_obj_isa; // 这个 _vtable_ 联想到了 C++ 中的虚函数表...
    int volatile os_obj_ref_cnt; // 引用计数
    int volatile os_obj_xref_cnt; // 外部引用计数
} _os_object_s;
```
&emsp;仅拥有三个成员变量的 `_os_object_s` 结构体。下面看一下它的第一个成员变量涉及的 `_os_object_vtable_s` 结构体的具体定义。

&emsp;`_os_object_s` 结构体的第一个成员变量 `const _os_object_vtable_s *os_obj_isa`。
```c++
typedef struct _os_object_vtable_s {
    _OS_OBJECT_CLASS_HEADER();
} _os_object_vtable_s;
```
&emsp;下面是 `_os_object_vtable_s` 结构体中的 `_OS_OBJECT_CLASS_HEADER()` 宏定义。
```c++
#if OS_OBJECT_HAVE_OBJC_SUPPORT

#if TARGET_OS_MAC && !TARGET_OS_SIMULATOR && defined(__i386__)
#define _OS_OBJECT_CLASS_HEADER() const void *_os_obj_objc_isa // 1⃣️
#else
#define _OS_OBJECT_CLASS_HEADER() void *_os_obj_objc_class_t[5] // 2⃣️
#endif

#else

// 这里是两个函数指针，（_os_object_t 应该是 _os_object_s 的指针）
// 这里的函数指针大概是处理结构体引用和结构体本身的操作吗？

#define _OS_OBJECT_CLASS_HEADER() \ // 3⃣️ 在 GCD 内部使用的应该是这里的 _OS_OBJECT_CLASS_HEADER 宏定义
        void (*_os_obj_xref_dispose)(_os_object_t); \
        void (*_os_obj_dispose)(_os_object_t)
#endif
```
&emsp;把上面的 `_os_object_vtable_s` 结构体完全展开的话是:
```c++
typedef struct _os_object_vtable_s {
    void (*_os_obj_xref_dispose)(_os_object_t);
    void (*_os_obj_dispose)(_os_object_t);
} _os_object_vtable_s;
```

&emsp;把 `const _os_object_vtable_s *os_obj_isa` 展开，在 arm64/x86_64 下，os_obj_isa 是一个指向长度是 5 元素是 void * 的指针。
## dispatch_object_s
&emsp;`dispatch_object_s` 是 GCD 的基础结构体，它是继承自 `_os_object_s` 结构体的，且其中涉及到连续的多个宏定义（看连续的宏定义真的好烦呀），下面一起来看一下。

&emsp;这里有一个细节要说一下，在 `dispatch_object_s` 内部仅有一行语句：一个宏定义 `_DISPATCH_OBJECT_HEADER`，这宏定义的命名为啥有些奇怪，你这好好的宏定义为啥命名还要加一个 `_HEADER` 后缀呢，这个 `_HEADER` 是有其用意的，它正是给继承自 `dispatch_object_s` 的子类准备的，当把它放在子类的头部时，即表明了子类所继承的父类是谁，然后把该宏完全展开时发现它们其实是一组父类的成员变量，平铺到子类中，而这正构成了 GCD 中的 “继承关系”。

&emsp;看到这里我们似乎有一些明白了，上面的 `_os_object_s` 结构体定义内部仅有一个 `_OS_OBJECT_HEADER` 宏定义，然后 `dispatch_object_s` 结构体内部也是仅有一个 `_DISPATCH_OBJECT_HEADER` 宏定义，宏定义的名字都是用了结构体名做前缀，然后加一个 `_HEADER` 后缀，而宏定义的内容则都是为了把当前的结构体包含的成员变量都包裹在一起。

&emsp;上面之所以说是把成员变量的内容包裹在一起，因为还有结构体能执行的函数调用的内容，例如：`_OS_OBJECT_CLASS_HEADER` 宏，它与 `_OS_OBJECT_HEADER` 相比名字里面加了 `CLASS` 且 `_HEADER` 后缀是保留的，`_OS_OBJECT_CLASS_HEADER` 宏定义的内容是把 `_os_object_s` 结构体指针做参数的一组函数指针。

&ensp;上面 `_os_object_vtable_s` 结构体定义内仅有的 `_OS_OBJECT_CLASS_HEADER` 宏定义包裹的是 `_os_object_s` 结构体指针做参数的一组函数指针，然后 `_os_object_s` 结构体的第一个成员变量 `os_obj_isa` 是一个指向 `_os_object_vtable_s` 的指针。

&emsp;这样看下来，结构体的成员变量有了，然后结构体做参数所能执行的一些函数调用也有了，这不就是完整的“类”定义吗。

&emsp;如果在全局搜 `DISPATCH_OBJECT_HEADER` 会发现有多个结构体的定义第一行都是 `DISPATCH_OBJECT_HEADER`，正表明了它们都是继承自 `dispatch_object_s`，如下面的队列组结构体、信号量结构体、io 结构体等。看到这里我们就真的明白为啥结构体定义的内部总是仅有一个 `_HEADER` 做后缀的宏定义了，都是为了接下来的“继承”做准备的。
```c++
// 队列组
struct dispatch_group_s {
    DISPATCH_OBJECT_HEADER(group);
    ...
};

// 信号量
struct dispatch_semaphore_s {
    DISPATCH_OBJECT_HEADER(semaphore);
    ...
};

struct dispatch_disk_s {
    DISPATCH_OBJECT_HEADER(disk);
    ...
};

struct dispatch_operation_s {
    DISPATCH_OBJECT_HEADER(operation);
    ...
};

struct dispatch_io_s {
    DISPATCH_OBJECT_HEADER(io);
    ...
};
```
&emsp;下面我们接着一步一步把 `dispatch_object_s` 的内容展开看看。
```c++
struct dispatch_object_s {
    _DISPATCH_OBJECT_HEADER(object);
};
```
### _DISPATCH_OBJECT_HEADER
&emsp;宏名中的 `_DISPATCH_OBJECT` 表明现在是 GCD 中的对象了。
```c++
#define _DISPATCH_OBJECT_HEADER(x) \
struct _os_object_s _as_os_obj[0]; \ ⬅️ 这里是一个长度为 0 的数组，不占用任何内存，同时它也预示了 dispatch_object_s 的 “父类” 是 _os_object_s 

OS_OBJECT_STRUCT_HEADER(dispatch_##x); \ ⬅️ OS_OBJECT_STRUCT_HEADER 宏展开就是把“父类”-_os_object_s 的成员变量平铺展开放在“子类”-dispatch_object_s 的头部

struct dispatch_##x##_s *volatile do_next; \ ⬅️ 下面的这一部分则是“子类”自己的成员变量
struct dispatch_queue_s *do_targetq; \
void *do_ctxt; \
void *do_finalizer
```
### OS_OBJECT_STRUCT_HEADER
&emsp;上面 `_os_object_s` 结构体的内容平铺展开放在 `dispatch_object_s` 结构体中。
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
```c++
#define _OS_OBJECT_HEADER(isa, ref_cnt, xref_cnt) \
isa; /* must be pointer-sized */ \ // isa 必须是指针大小
int volatile ref_cnt; \ // 引用计数
int volatile xref_cnt // 外部引用计数，两者都为 0 时，对象才能释放
```
&emsp;到这里后 `dispatch_object_s` 涉及到的宏定义就全部看完了，现在把上面的 `dispatch_object_s` 结构体内部的宏定义全部展开后如下:
```c++
struct dispatch_object_s {
    struct _os_object_s _as_os_obj[0]; // 长度为 0 的数组
    
    // _os_object_s 是仅包含下面三个成员变量的结构体，同时它也是 GCD 中所有“类”的基类，大概可以理解为 OC 中的 NSObject
    // const _os_object_vtable_s *os_obj_isa; 
    // int volatile os_obj_ref_cnt; 
    // int volatile os_obj_xref_cnt;
    
    const struct dispatch_object_vtable_s *do_vtable; /* must be pointer-sized */ // do_vtable 包含了对象类型和 dispatch_object_s 的操作函数
    int volatile do_ref_cnt; // 引用计数（do 应该是 Dispatch Object 的首字母，上面 _os_object_s 内使用的是 os_obj_ref_cnt）
    int volatile do_xref_cnt; // 外部引用计数，两者都为 0 时才会释放对象内存
    
    struct dispatch_object_s *volatile do_next; // do_next 表示链表的 next，（下一个 dispatch_object_s）
    struct dispatch_queue_s *do_targetq; // 目标队列，（表示当前任务要在这个队列运行，待验证）
    void *do_ctxt; // 上下文，即运行任务（其实是一个函数）的参数
    void *do_finalizer; // 最终销毁时调用的函数
};
```
&emsp;emmm... 还有一个点，上面虽然一直说子类平铺展开父类的成员变量，其实是成员变量的类型得到保留，而名字是发了变化的。

&emsp;看到 `dispatch_object_s` 内部比较诡异的第一行一个长度是 0 的 `_os_object_s` 结构体数组。同时它也暗示了 `dispatch_object_s` 的父类是谁，同时它还有一层含义，我们可能见过一些在结构体末尾放一个长度为 0 的数组，它们是为了表明内存空间接下来的类型是什么，那么这里的结构体头部的长度是 0 的数组是什么意思呢，它是用来表明当前内存空间的结构体类型吗？

&emsp;在 `dispatch_object_s` 结构体第一个成员变量是 `const struct dispatch_object_vtable_s *do_vtable`，这里的 `dispatch_object_vtable_s` 结构体和 `_os_object_s` 结构体中的 `_os_object_vtable_s` 结构体内容有何不同呢，下面一起来看一下。

&emsp;这里 `dispatch_object_vtable_s` 不是直接定义的，它涉及到另外一个宏定义 `OS_OBJECT_CLASS_DECL`，`dispatch_object_vtable_s` 结构体定义是放在 `OS_OBJECT_CLASS_DECL` 宏定义里面的（真的快看吐了...），宏定义名也表明了 `dispatch_object_s` 是继承自 `_os_object_s` 的。同时宏名里面也有 `_CLASS` 这也对应了上面 `_OS_OBJECT_CLASS_HEADER` 宏，有 `_CLASS` 的宏都是用来表明继承时的函数继承的，如这里的 `OS_OBJECT_CLASS_DECL` 宏主要是用来让 `dispatch_object_s` 结构体继承 `_os_object_s` 结构体的操作函数用的，下面来看一下吧。
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
&emsp;`DISPATCH_OBJECT_VTABLE_HEADER` 会根据 `USE_OBJC` 环境做不同的定义，两者唯一的区别是在非 `USE_OBJC` 环境下多了一个 `const char *const do_kind`。
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
    const char *const do_kind; \ // 多了一个 do_kind 成员变量
    void (*const do_dispose)(struct x##_s *, bool *allow_free); \
    size_t (*const do_debug)(struct x##_s *, char *, size_t); \
    void (*const do_invoke)(struct x##_s *, dispatch_invoke_context_t, \
            dispatch_invoke_flags_t)
#endif
```
&emsp;把上面 `OS_OBJECT_CLASS_DECL` 中的 `DISPATCH_OBJECT_VTABLE_HEADER` 宏定义展开如下。
```c++
// 3⃣️ 
unsigned long const do_type;
const char *const do_kind;
void (*const do_dispose)(struct dispatch_object_s *, bool *allow_free);
size_t (*const do_debug)(struct dispatch_object_s *, char *, size_t);
void (*const do_invoke)(struct dispatch_object_s *, dispatch_invoke_context_t, dispatch_invoke_flags_t);
```
### OS_OBJECT_CLASS_DECL
&emsp;`OS_OBJECT_CLASS_DECL` 宏定义的内容是完整定义了一个 “继承” 自 `_os_object_s` 的 “类”。
```c++

OS_OBJECT_CLASS_DECL(dispatch_object, \
DISPATCH_OBJECT_VTABLE_HEADER(dispatch_object))

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
&emsp;那么把上面 2⃣️ 处的宏定义展开如下：
```c++
// 4⃣️
struct dispatch_object_s;
struct dispatch_object_extra_vtable_s { // 这里表明子类的 vtable 内部的扩展，例如子类新增的内容（本来想说是新的操作函数呢，但是里面还有成员变量...）
    unsigned long const do_type;
    const char *const do_kind;
    void (*const do_dispose)(struct dispatch_object_s *, bool *allow_free);
    size_t (*const do_debug)(struct dispatch_object_s *, char *, size_t);
    void (*const do_invoke)(struct dispatch_object_s *, dispatch_invoke_context_t, dispatch_invoke_flags_t);
};

struct dispatch_object_vtable_s { // 这里就是我们抽丝剥茧一层一层要找的 dispatch_object_vtable_s 了。
    // _OS_OBJECT_CLASS_HEADER(); 此处两行是把父类 _os_object_s 的函数带过来 
    void (*_os_obj_xref_dispose)(_os_object_t);
    void (*_os_obj_dispose)(_os_object_t);
    
    // 下面是子类新增的内容
    struct dispatch_object_extra_vtable_s _os_obj_vtable;
};

// OS_OBJECT_EXTRA_VTABLE_DECL 是留给子类继承父类的方法用的
OS_OBJECT_EXTRA_VTABLE_DECL(dispatch_object, dispatch_object) \
extern const struct dispatch_object_vtable_s OS_OBJECT_CLASS_SYMBOL(dispatch_object) \
        __asm__(OS_OBJC_CLASS_RAW_SYMBOL_NAME(OS_OBJECT_CLASS(dispatch_object)))
```
```c++
#define OS_OBJECT_EXTRA_VTABLE_SYMBOL(name) _OS_##name##_vtable

#define OS_OBJECT_EXTRA_VTABLE_DECL(name, ctype) \
        extern const struct ctype##_vtable_s \
                OS_OBJECT_EXTRA_VTABLE_SYMBOL(name);
➡️
extern const struct dispatch_object_vtable_s _OS_dispatch_object_vtable;
                
#define OS_OBJECT_CLASS_SYMBOL(name) _##name##_vtable
➡️
extern const struct dispatch_object_vtable_s _dispatch_object_vtable __asm__(".objc_class_name_" OS_STRINGIFY(OS_dispatch_object))
```
&emsp;看到这里，我们就能明白前面 `dispatch_object_s` 结构体定义内部的这句："`const struct dispatch_object_vtable_s *do_vtable; /* must be pointer-sized */` // do_vtable 包含了对象类型和 dispatch_object_s 的操作函数"  的含义了。

&emsp;emmm...看到这里我们就把 `dispatch_object_s` 结构定义相关的内容全部看完了，真的是宏定义一层套一层，宏定义里面再套结构体的定义。（关于它定义里面的几个函数具体作用和实现待后续再展开讲解。）

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
                      };)
                     
union { 
    uint64_t volatile dq_state;
    struct {
        dispatch_lock dq_state_lock;
        uint32_t dq_state_bits;
    };
};

// 4. DISPATCH_UNION_ASSERT 仅是一个断言，且 typedef uint32_t dispatch_lock; 即双方都是 64 位，8 个字节，那么宏定义全部展开就只剩下:
union { 
    uint64_t volatile dq_state;
    struct {
        dispatch_lock dq_state_lock;
        uint32_t dq_state_bits;
    };
};
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
    uint16_t dqai_overcommit:2; // 是否可以 overcommit
    uint16_t dqai_autorelease_frequency:2; // （自动释放频率）
    uint16_t dqai_concurrent:1; // 表示队列是并发队列还是串行队列
    uint16_t dqai_inactive:1; // 表示当前队列是否是活动状态（是否激活）
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
### dispatch_continuation_t
&emsp;在 queue_internal.h 文件中看到 `#pragma mark dispatch_continuation_t` 行，往下的 200 多行的整个区域的代码都是和 `dispatch_continuation_t` 相关的代码。

&emsp;首先根据上面的规则我们已知 `dispatch_continuation_t` 是指向 `dispatch_continuation_s` 结构体的指针类型。

&emsp;当我们向队列提交任务时，无论 block 还是 function 形式，最终都会被封装为 `dispatch_continuation_s`，所以可以把它理解为描述任务内容的结构体。




















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
