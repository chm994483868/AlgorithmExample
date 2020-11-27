# iOS 多线程知识体系构建(八)：GCD 源码：基本数据结构篇（2）

> &emsp;首先来看队列属性使用的结构体 dispatch_queue_attr_s。

## dispatch_queue_attr_s
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
&emsp;看到了熟悉的三个成员变量（类似 `_os_object_s` 结构体的前三个成员变量）。看到这里可能会迷惑，不是说好的 `dispatch_queue_attr_s` 是描述队列属性的数据结构吗，怎么内部就只有 “继承” 自 `_os_object_s` 的三个成员变量。实际描述队列的属性的结构体其实是 `dispatch_queue_attr_info_t`（是 `dispatch_queue_attr_info_s` 结构体的别名）。
### dispatch_queue_attr_info_t
&emsp;看到 `dispatch_queue_attr_info_s` 内部使用了位域来表示不同的值，来节省内存占用。
```c++
typedef struct dispatch_queue_attr_info_s {
    // typedef uint32_t dispatch_qos_t; dispatch_qos_t 是 uint32_t 类型
    dispatch_qos_t dqai_qos : 8; //（表示线程优先级）
    int      dqai_relpri : 8; //（表示优先级的偏移）
    uint16_t dqai_overcommit:2; // 是否可以 overcommit（过的量是 CPU 的物理核心数）
    uint16_t dqai_autorelease_frequency:2; // （自动释放频率）
    uint16_t dqai_concurrent:1; // 表示队列是并发队列还是串行队列
    uint16_t dqai_inactive:1; // 表示当前队列是否是活动状态（是否激活）
} dispatch_queue_attr_info_t;
```
&emsp;其实这里队列属性相关的内容包含更复杂的内容，在 queue_internal.h 文件内部，看到用 `#pragma mark dispatch_queue_attr_t` 定义了一个区域的代码，它们都与队列属性有关，下面我们把该区域的代码都看一遍。
```c++
DISPATCH_CLASS_DECL(queue_attr, OBJECT);
```
### DISPATCH_CLASS_DECL
&emsp;`DISPATCH_CLASS_DECL(queue_attr, OBJECT)` 内部是定义 `dispatch_queue_attr_vtable_s` 的内容，定义 `dispatch_queue_attr_s` 的一些操作函数。
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
&emsp;在 C 环境下 `#define _OS_OBJECT_DECL_PROTOCOL(name, super)` 什么事情都不做。同样在 C 环境下 `#define _OS_OBJECT_CLASS_IMPLEMENTS_PROTOCOL(name, super)` 也是什么事情都不做。

```c++
#define DISPATCH_CLASS_DECL_BARE(name, cluster) \
        OS_OBJECT_CLASS_DECL(dispatch_##name, \
        DISPATCH_##cluster##_VTABLE_HEADER(dispatch_##name))
```

```c++
// 1⃣️
DISPATCH_CLASS_DECL_BARE(queue_attr, OBJECT)
```
&emsp;把上面宏定义展开如下:
```c++
// 2⃣️
OS_OBJECT_CLASS_DECL(dispatch_queue_attr, \
DISPATCH_OBJECT_VTABLE_HEADER(dispatch_queue_attr))
```
&emsp;把 `DISPATCH_OBJECT_VTABLE_HEADER(dispatch_queue_attr)` 宏定义展开如下:
```c++
// 3⃣️
unsigned long const do_type;
const char *const do_kind;
void (*const do_dispose)(struct dispatch_queue_attr_s *, bool *allow_free);
size_t (*const do_debug)(struct dispatch_queue_attr_s *, char *, size_t);
void (*const do_invoke)(struct dispatch_queue_attr_s *, dispatch_invoke_context_t, dispatch_invoke_flags_t)
```
&emsp;把 2⃣️ 处都宏定义展开如下:
```c++

OS_OBJECT_CLASS_DECL(dispatch_queue_attr, \
DISPATCH_OBJECT_VTABLE_HEADER(dispatch_queue_attr))

struct dispatch_queue_attr_s;
struct dispatch_queue_attr_extra_vtable_s {
    unsigned long const do_type;
    const char *const do_kind;
    void (*const do_dispose)(struct dispatch_queue_attr_s *, bool *allow_free);
    size_t (*const do_debug)(struct dispatch_queue_attr_s *, char *, size_t);
    void (*const do_invoke)(struct dispatch_queue_attr_s *, dispatch_invoke_context_t, dispatch_invoke_flags_t)
};

struct dispatch_queue_attr_vtable_s {
    void (*_os_obj_xref_dispose)(_os_object_t);
    void (*_os_obj_dispose)(_os_object_t);
            
    struct dispatch_queue_attr_extra_vtable_s _os_obj_vtable;
};
        
extern const struct dispatch_queue_attr_vtable_s _OS_dispatch_queue_attr_vtable;
extern const struct dispatch_queue_attr_vtable_s _dispatch_queue_attr_vtable __asm__(".objc_class_name_" OS_STRINGIFY(OS_dispatch_queue_attr))
```
### _dispatch_queue_attr_overcommit_t
&emsp;指定队列 overcommit 状态的枚举。 
```c++
typedef enum {
    _dispatch_queue_attr_overcommit_unspecified = 0, // 未指定
    _dispatch_queue_attr_overcommit_enabled, // 允许 overcommit
    _dispatch_queue_attr_overcommit_disabled, // 不允许 overcommit
} _dispatch_queue_attr_overcommit_t;
```
### DISPATCH_QUEUE_ATTR_COUNT
&emsp;是指队列属性的数量吗？值是不同属性的值的乘积。
```c++
#define DISPATCH_QUEUE_ATTR_OVERCOMMIT_COUNT 3

#define DISPATCH_QUEUE_ATTR_AUTORELEASE_FREQUENCY_COUNT 3

#define DISPATCH_QUEUE_ATTR_QOS_COUNT (DISPATCH_QOS_MAX + 1) // 6

#define DISPATCH_QUEUE_ATTR_PRIO_COUNT (1 - QOS_MIN_RELATIVE_PRIORITY) // 16

#define DISPATCH_QUEUE_ATTR_CONCURRENCY_COUNT 2

#define DISPATCH_QUEUE_ATTR_INACTIVE_COUNT 2

#define DISPATCH_QUEUE_ATTR_COUNT  ( \
        DISPATCH_QUEUE_ATTR_OVERCOMMIT_COUNT * \
        DISPATCH_QUEUE_ATTR_AUTORELEASE_FREQUENCY_COUNT * \
        DISPATCH_QUEUE_ATTR_QOS_COUNT * \
        DISPATCH_QUEUE_ATTR_PRIO_COUNT * \
        DISPATCH_QUEUE_ATTR_CONCURRENCY_COUNT * \
        DISPATCH_QUEUE_ATTR_INACTIVE_COUNT )
```
&emsp;计算可得 `DISPATCH_QUEUE_ATTR_COUNT = 3456(3 * 3 * 6 * 16 * 2 * 2)`。
### _dispatch_queue_attrs
&emsp;然后是一个全局变量 `_dispatch_queue_attrs`，一个长度是  3456 的 `dispatch_queue_attr_s` 数组。
```c++
extern const struct dispatch_queue_attr_s
_dispatch_queue_attrs[DISPATCH_QUEUE_ATTR_COUNT];
```
### _dispatch_queue_attr_to_info
&emsp;
```c++
dispatch_queue_attr_info_t
_dispatch_queue_attr_to_info(dispatch_queue_attr_t dqa)
{
    dispatch_queue_attr_info_t dqai = { };

    if (!dqa) return dqai;

#if DISPATCH_VARIANT_STATIC
    if (dqa == &_dispatch_queue_attr_concurrent) {
        dqai.dqai_concurrent = true;
        return dqai;
    }
#endif

    if (dqa < _dispatch_queue_attrs ||
            dqa >= &_dispatch_queue_attrs[DISPATCH_QUEUE_ATTR_COUNT]) {
        DISPATCH_CLIENT_CRASH(dqa->do_vtable, "Invalid queue attribute");
    }

    size_t idx = (size_t)(dqa - _dispatch_queue_attrs);

    dqai.dqai_inactive = (idx % DISPATCH_QUEUE_ATTR_INACTIVE_COUNT);
    idx /= DISPATCH_QUEUE_ATTR_INACTIVE_COUNT;

    dqai.dqai_concurrent = !(idx % DISPATCH_QUEUE_ATTR_CONCURRENCY_COUNT);
    idx /= DISPATCH_QUEUE_ATTR_CONCURRENCY_COUNT;

    dqai.dqai_relpri = -(int)(idx % DISPATCH_QUEUE_ATTR_PRIO_COUNT);
    idx /= DISPATCH_QUEUE_ATTR_PRIO_COUNT;

    dqai.dqai_qos = idx % DISPATCH_QUEUE_ATTR_QOS_COUNT;
    idx /= DISPATCH_QUEUE_ATTR_QOS_COUNT;

    dqai.dqai_autorelease_frequency =
            idx % DISPATCH_QUEUE_ATTR_AUTORELEASE_FREQUENCY_COUNT;
    idx /= DISPATCH_QUEUE_ATTR_AUTORELEASE_FREQUENCY_COUNT;

    dqai.dqai_overcommit = idx % DISPATCH_QUEUE_ATTR_OVERCOMMIT_COUNT;
    idx /= DISPATCH_QUEUE_ATTR_OVERCOMMIT_COUNT;

    return dqai;
}
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
