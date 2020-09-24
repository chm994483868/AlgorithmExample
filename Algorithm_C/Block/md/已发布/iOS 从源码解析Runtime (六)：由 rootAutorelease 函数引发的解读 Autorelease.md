# iOS 从源码解析Runtime (六)：由 rootAutorelease 函数引发的解读 Autorelease

> 上一篇文章分析了 `objc_object` 的 `retain` 和 `releasae` 等函数相关的内容，当看到 `rootAutorelease` 函数里面的 `AutoreleasePoolPage` 的时候，觉的是时候深入学习自动释放池了，那么就由本篇开始吧。

## `AutoreleasePoolPageData`
&emsp;老样子我们还是首先分析其所使用的数据结构，发现这是一个很好的切入角度。每次先看几篇大佬写的原理相关的文章，然后直接进入源码，原理知识点可能会看的模模糊糊，但是看源码却轻松很多，然后就着源码去理解原理的脉络。⛽️⛽️
&emsp;从 `rootAutorelease` 函数开始，按住 `command` 我们一层一层进入到 `Private Headers/NSObject-internal.h` 文件，它里面定义了三个结构体 `magic_t`、`AutoreleasePoolPageData`、`thread_data_t` 以及 `AutoreleasePoolPage` 的前向声明，正式它们构成了自动释放池的完整结构。

首先我们先看下 `NSObject-internal.h` 文件开头的注释和几个宏定义。
>    Autorelease pool implementation
  A thread's autorelease pool is a stack of pointers.
  Each pointer is either an object to release, or POOL_BOUNDARY which is
  an autorelease pool boundary.
  A pool token is a pointer to the POOL_BOUNDARY for that pool. 
  When the pool is popped, every object hotter than the sentinel is released.
  The stack is divided into a doubly-linked list of pages. Pages are added
  and deleted as necessary.
  Thread-local storage points to the hot page, where newly autoreleased objects are stored.

+ 一个线程的自动释放池就是一个存放指针的栈。
+ 每个指针要么是要释放的对象，要么是 `POOL_BOUNDARY` 自动释放池边界。
+ `pool token` 是指向该池的 `POOL_BOUNDARY` 的指针。
+ 当自动释放池执行 `popped`，`every object hotter than the sentinel is released.`。（这句没有看懂）
+ 这些栈分离在 `AutoreleasePoolPage` 构成的双向链表中。`AutoreleasePoolPage` 已添加或者根据需要删除。
+ `Thread-local storage points to the hot page, where newly autoreleased objects are stored.`

`#define AUTORELEASEPOOL_VERSION 1` 自动释放池的版本号，仅当 `ABI` 的兼容性被打破时才会改变。
`#define PROTECT_AUTORELEASEPOOL 0` 将此设置为 1 即可 `mprotect()` 自动释放池的内容。
`#define CHECK_AUTORELEASEPOOL (DEBUG)` 将此设置为 1 可以在所有时间验证整个自动释放池的 `header`。（例如，在任何地方使用 `check()` 代替 `fastcheck()`）

以及开头的一段警告：`WARNING  DANGER  HAZARD  BEWARE  EEK` 告诉我们此文件的任何内容都是 `Apple` 内部使用的，它们可能在任何的版本更新中以不可预测的方式修改文件里面的内容。

### `magic_t`
&emsp;`M0` 和 `M1` 的硬编码...，
```c++
struct magic_t {
    // 静态不可变 32 位 int 值
    static const uint32_t M0 = 0xA1A1A1A1;
    
    // 这个宏，emm....
#   define M1 "AUTORELEASE!"
    
    // m 数组占用 16 个字节，每个 uint32_t 占 4 个字节，减去第一个元素的 4 是 12 
    static const size_t M1_len = 12;
    // 长度为 4 的 uint32_t 数组
    uint32_t m[4];

    // magic_t 的构造函数
    magic_t() {
        // 都是 12
        ASSERT(M1_len == strlen(M1));
        // 12 = 3 * 4
        ASSERT(M1_len == 3 * sizeof(m[1]));

        // m 数组第一个元素是 M0
        m[0] = M0;
        // 把 M1 复制到从 m[1] 开始的往后 12 个字节内
        // 那么 m 数组，前面 4 个字节放数字 M0 然后后面 12 个字节放字符串 AUTORELEASE!
        strncpy((char *)&m[1], M1, M1_len);
    }
    
    // 析构函数
    ~magic_t() {
        // Clear magic before deallocation.
        // magic_t 在 deallocation 之前清理数据。
        
        // This prevents some false positives in memory debugging tools.
        // 这样可以防止内存调试工具出现误报。
        
        // fixme semantically this should be memset_s(), 
        // but the compiler doesn't optimize that at all (rdar://44856676).
        // fixme 从语义上讲，这应该是 memset_s（），但是编译器根本没有对其进行优化。
        
        // 把 m 转化为一个 uint64_t 的数组， uint64_t 类型占 8 个字节
        volatile uint64_t *p = (volatile uint64_t *)m;
        // 置 0
        p[0] = 0; p[1] = 0;
    }

    bool check() const {
        // 检测
        // 0 元素是 M0，后面 12 个字节是 M1，和构造函数中初始化的值一模一样的话即返回 true
        return (m[0] == M0 && 0 == strncmp((char *)&m[1], M1, M1_len));
    }

    bool fastcheck() const {
#if CHECK_AUTORELEASEPOOL
        // 程序在 DEBUG 模式下执行完整比较
        return check();
#else
        // 程序在 RELEASE 模式下是只比较 m[0] 的值是 0xA1A1A1A1
        return (m[0] == M0);
#endif
    }

// M1 解除宏定义
#   undef M1
};
```
### `struct AutoreleasePoolPageData`
```c++
// 前向声明，AutoreleasePoolPage 是私有继承自 AutoreleasePoolPageData 的类
class AutoreleasePoolPage;

struct AutoreleasePoolPageData
{
    // struct magic_t
    magic_t const magic;
    
    // __unsafe_unretained 修饰的 next，还是第一次见使用修饰符
    // next 指针作为游标指向栈顶最新 add 进来的 autorelease 对象的下一个位置
    __unsafe_unretained id *next;
    
    // typedef __darwin_pthread_t pthread_t;
    // typedef struct _opaque_pthread_t *__darwin_pthread_t;
    // 原始是 struct _opaque_pthread_t 指针
    // AutoreleasePool 是按线程一一对应的，thread 正是自动释放池所处的当前线程
    pthread_t const thread;
    
    // AutoreleasePool 没有单独的结构，而是由若干个 AutoreleasePoolPage 以双向链表的形式组合而成
    // parent 和 child 这两个 AutoreleasePoolPage指针这是构成链表用的值指针
    AutoreleasePoolPage * const parent;
    AutoreleasePoolPage *child;
    
    // 这两个属性还不知道是干嘛用的
    uint32_t const depth;
    uint32_t hiwat;

    // 构造函数（初始化列表默认对所有成员变量都执行了默认初始化）
    AutoreleasePoolPageData(__unsafe_unretained id* _next,
                            pthread_t _thread,
                            AutoreleasePoolPage* _parent,
                            uint32_t _depth,
                            uint32_t _hiwat) : magic(),
                                               next(_next),
                                               thread(_thread),
                                               parent(_parent),
                                               child(nil),
                                               depth(_depth),
                                               hiwat(_hiwat){
    }
};
```
### `struct thread_data_t`
```c++
struct thread_data_t
{
#ifdef __LP64__
    // struct _opaque_pthread_t 指针 8 字节
    pthread_t const thread;
    uint32_t const hiwat; // 4 字节
    uint32_t const depth; // 4 字节
#else
    pthread_t const thread;
    uint32_t const hiwat;
    uint32_t const depth;
    uint32_t padding;
#endif
};
// 一个断言，如果 thread_data_t 的 size 不是 16 的话就会执行该断言
C_ASSERT(sizeof(thread_data_t) == 16);
```

## `AutoreleasePoolPage`

### `BREAKPOINT_FUNCTION`
```c++
/* Use this for functions that are intended to be breakpoint hooks.
   If you do not, the compiler may optimize them away.
   BREAKPOINT_FUNCTION( void stop_on_error(void) ); */
#   define BREAKPOINT_FUNCTION(prototype)                             \
    OBJC_EXTERN __attribute__((noinline, used, visibility("hidden"))) \
    prototype { asm(""); }
```

```c++
BREAKPOINT_FUNCTION(void objc_autoreleaseNoPool(id obj));
BREAKPOINT_FUNCTION(void objc_autoreleasePoolInvalid(const void *token));
```


## `Autorelease` 对象什么时候释放？

## `Tagged Pointer` 什么时候释放？



## 参考链接
**参考链接:🔗**
+ [黑幕背后的Autorelease](http://blog.sunnyxx.com/2014/10/15/behind-autorelease/)
+ []()
+ [操作系统内存管理(思维导图详解)](https://blog.csdn.net/hguisu/article/details/5713164)

