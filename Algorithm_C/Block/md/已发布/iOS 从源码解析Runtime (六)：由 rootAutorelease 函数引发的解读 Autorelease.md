# iOS 从源码解析Runtime (六)：由 rootAutorelease 函数引发的解读 Autorelease

> 上一篇文章分析了 `objc_object` 的 `retain` 和 `releasae` 等函数相关的内容，当看到 `rootAutorelease` 函数里面的 `AutoreleasePoolPage` 的时候，觉的是时候深入学习自动释放池了，那么就由本篇开始吧。

## `AutoreleasePoolPageData`
&emsp;老样子我们还是首先分析其所使用的数据结构，发现这是一个很好的切入角度。每次要详细进入一个知识点时可采取如下步骤：
1. 第一步首先找相关内容的文章，对大概的知识脉络有一个认知，尽管一些地方看不懂也没事，尽管看的模模糊糊也没事。
2. 第二步直接进入源码，源码部分一般都是简单的 `C++` 代码，然后 `Apple` 的封装也做的特别好，每个函数每个功能都特别清晰明了，看源码是最轻松也是最明了的，这时第一步看的相关原理就会在脑子里慢慢浮现慢慢被串联起来了。
3. 第三步源码看完了熟络了，然后再回到第一步，然后结合着源码可看一些更高深的文章，最终可做到融会贯通。⛽️⛽️

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
    pthread_t const thread; // struct _opaque_pthread_t 指针 8 字节
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
// 可以看到在 __LP64__ 下，同时在内存对齐原则下 thread_data_t size 也是 16
C_ASSERT(sizeof(thread_data_t) == 16);
```

## `AutoreleasePoolPage`
&emsp;下面我们开始解读 `AutoreleasePoolPage` 源码。

### `BREAKPOINT_FUNCTION`
&emsp;`BREAKPOINT_FUNCTION` 宏定义在 `Project Headers/objc-os.h` 文件下，针对不同的运行环境它的定义是不同的。
在 `TARGET_OS_MAC` 环境下定义如下: 
```c++
/* Use this for functions that are intended to be breakpoint hooks. 
   If you do not, the compiler may optimize them away.
   BREAKPOINT_FUNCTION( void stop_on_error(void) );
   // 如果我们要对某个函数做 breakpoint hook，则需要用该宏定义声明一下。
   // 如果我们不这样做的话，编译器可能会优化它们。
   // 例如：BREAKPOINT_FUNCTION( void stop_on_error(void) );
*/
#   define BREAKPOINT_FUNCTION(prototype)                             \
    OBJC_EXTERN __attribute__((noinline, used, visibility("hidden"))) \
    prototype { asm(""); }
```
#### `__attribute__((used))`
&emsp;`__attribute__((used))` 的作用：
1. 用于告诉编译器在目标文件中保留一个静态函数或者静态变量，即使它没有被引用。
2. 标记为 `attribute__((used))` 的函数被标记在目标文件中，以避免链接器删除未使用的节。
3. 静态变量也可以标记为 `used`，方法是使用 `attribute((used))`。
4. 使用 `used` 字段，即使没有任何引用，在 `Release` 下也不会被优化。

警告信息产生的原因可参考: [__attribute__((used))的使用问题](http://www.openedv.com/forum.php?mod=viewthread&tid=277480&extra=page=3)
```c++
// 表示该函数或变量可能不使用，这个属性可以避免编译器产生警告信息
#define __attribute_unused__ __attribute__((__unused__))
// 向编译器说明这段代码有用，即使在没有用到的情况下编译器也不会警告
#define __attribute_used__ __attribute__((__used__))
```
#### `__attribute__((visibility("hidden")))`
> 在 `Linux` 下动态库`(.so)`中，通过`GCC`的`C++ visibility`属性可以控制共享文件导出符号。在`GCC 4.0`及以上版本中，有个`visibility`属性，可见属性可以应用到函数、变量、模板以及C++类。
  限制符号可见性的原因：从动态库中尽可能少地输出符号是一个好的实践经验。输出一个受限制的符号会提高程序的模块性，并隐藏实现的细节。动态库装载和识别的符号越少，程序启动和运行的速度就越快。导出所有符号会减慢程序速度，并耗用大量内存。
  `default`：用它定义的符号将被导出，动态库中的函数默认是可见的。
  `hidden`：用它定义的符号将不被导出，并且不能从其它对象进行使用，动态库中的函数是被隐藏的。
  `default`意味着该方法对其它模块是可见的。而`hidden`表示该方法符号不会被放到动态符号表里，所以其它模块(可执行文件或者动态库)不可以通过符号表访问该方法。
  要定义`GNU`属性，需要包含`__attribute__`和用括号括住的内容。可以将符号的可见性指定为`visibility(“hidden”)`，这将不允许它们在库中被导出，但是可以在源文件之间共享。实际上，隐藏的符号将不会出现在动态符号表中，但是还被留在符号表中用于静态链接。
  导出列表由编译器在创建共享库的时候自动生成，也可以由开发人员手工编写。导出列表的原理是显式地告诉编译器可以通过外部文件从对象文件导出的符号是哪些。GNU用户将此类外部文件称作为”导出映射”。[Linux下__attribute__((visibility ("default")))的使用](https://blog.csdn.net/fengbingchun/article/details/78898623)

在 `TARGET_OS_WIN32` 环境下定义如下: 
```c++
/* Use this for functions that are intended to be breakpoint hooks.
   If you do not, the compiler may optimize them away.
   BREAKPOINT_FUNCTION( void MyBreakpointFunction(void) ); */
   // 和👆一致
   // 看宏定义的内容，意思是把函数标记为 noinline(不要内联) 就行了吗？
#   define BREAKPOINT_FUNCTION(prototype) \
    __declspec(noinline) prototype { __asm { } }
```

未找到下面两个函数的实现:
```c++
BREAKPOINT_FUNCTION(void objc_autoreleaseNoPool(id obj));
// token 莫不是开始的注释: a pool token 是指向该池的 POOL_BOUNDARY 的指针。
BREAKPOINT_FUNCTION(void objc_autoreleasePoolInvalid(const void *token));
```
&emsp;`AutoreleasePoolPage` 是一个私有继承自 `AutoreleasePoolPageData` 的类。并且 `thread_data_t` 是它 友元结构体。
```c++
class AutoreleasePoolPage : private AutoreleasePoolPageData
{
    friend struct thread_data_t;
    ...
};
```
### `SIZE`
&emsp;表示 `AutoreleasePoolPage` 的容量。已知在 `Private Headers/NSObject-internal.h` 中 `PROTECT_AUTORELEASEPOOL` 值为 `0`。
```c++
    static size_t const SIZE =
#if PROTECT_AUTORELEASEPOOL
        PAGE_MAX_SIZE;  // must be multiple of vm page size
#else
        PAGE_MIN_SIZE;  // size and alignment, power of 2
#endif

#define PAGE_MIN_SIZE           PAGE_SIZE
#define PAGE_SIZE               I386_PGBYTES
#define I386_PGBYTES            4096            /* bytes per 80386 page */
```
可看到 `SIZE` 的值是 `4096`。
```c++
private:
    // typedef __darwin_pthread_key_t pthread_key_t;
    // typedef unsigned long __darwin_pthread_key_t;
    // 所以 pthread_key_t 实际是一个 unsigned long 类型
    
    // #define AUTORELEASE_POOL_KEY    ((tls_key_t)__PTK_FRAMEWORK_OBJC_KEY3)
    // typedef pthread_key_t tls_key_t;
    // #define __PTK_FRAMEWORK_OBJC_KEY3    43
    // AUTORELEASE_POOL_KEY 
    // tls 全拼是 Thread Local Storage 表示在当前线程存储一些数据用，
    // 而这些数据的存储与读取是通过这些固定的 Key 来做的。
    
    static pthread_key_t const key = AUTORELEASE_POOL_KEY;
    
    // SCRIBBLE 
    static uint8_t const SCRIBBLE = 0xA3;  // 0xA3A3A3A3 after releasing
    
    // 可保存的 id 的数量 4096 / 8 = 512
    static size_t const COUNT = SIZE / sizeof(id);

    // EMPTY_POOL_PLACEHOLDER is stored in TLS when exactly one pool is pushed and it has never contained any objects. 
    // 当创建了一个自动释放池且未放入任何对象的时候 EMPTY_POOL_PLACEHOLDER 就会存储在 TLS 中。 
    
    // This saves memory when the top level (i.e. libdispatch) pushes and pops pools but never uses them.
    // 当 top level(例如 libdispatch) pushes 和 pools 却从不使用它们的时候可以节省内存。
    
    // objc_object **
#   define EMPTY_POOL_PLACEHOLDER ((id*)1)
    // pool 的边界是 nil
#   define POOL_BOUNDARY nil

// SIZE-sizeof(*this) bytes of contents follow
```
### `new/delete`
```c++
// 申请空间并进行内存对齐

static void * operator new(size_t size) {
    // extern malloc_zone_t *malloc_default_zone(void); /* The initial zone */ // 初始 zone
    // extern void *malloc_zone_memalign(malloc_zone_t *zone, size_t alignment, size_t size) __alloc_size(3) __OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_0);
    // alignment 对齐长度
    // 分配一个大小为 size 的新指针，其地址是对齐的精确倍数。对齐方式必须是 2 的幂并且至少与 sizeof(void *) 一样大。 zone 必须为非 NULL。
    return malloc_zone_memalign(malloc_default_zone(), SIZE, SIZE);
}
static void operator delete(void * p) {
    // 释放内存
    return free(p);
}
```
### `protect/unprotect`
&emsp;已知在 `Private Headers/NSObject-internal.h` 中 `PROTECT_AUTORELEASEPOOL` 值为 `0`，所以这两个函数在 `x86_64` 下什么也不做。
```c++
    inline void protect() {
#if PROTECT_AUTORELEASEPOOL
        mprotect(this, SIZE, PROT_READ);
        check();
#endif
    }

    inline void unprotect() {
#if PROTECT_AUTORELEASEPOOL
        check();
        mprotect(this, SIZE, PROT_READ | PROT_WRITE);
#endif
    }
```
&emsp;在 `Linux` 中 `mprotect()` 函数可以用来修改一段指定内存区域的保护属性。
函数原型如下：
```c++
#include <unistd.h>
#include <sys/mmap.h>
int mprotect(const void *start, size_t len, int prot);
```
`mprotect()`函数把自`start`开始的、长度为`len`的内存区的保护属性修改为`prot`指定的值。
`prot` 可以取以下几个值，并且可以用 “|” 将几个属性合起来使用：
1. `PROT_READ`：表示内存段内的内容可读。
2. `PROT_WRITE`：表示内存段内的内容可写。
3. `PROT_EXEC`：表示内存段中的内容可执行。
4. `PROT_NONE`：表示内存段中的内容根本没法访问。
&emsp;需要指出的是，指定的内存区间必须包含整个内存页`（4K, 不同体系结构和操作系统，一页的大小不尽相同。如何获得页大小呢？通过 PAGE_SIZE 宏或者 getpagesize() 系统调用即可）`。区间开始的地址`start`必须是一个内存页的起始地址，并且区间长度`len`必须是页大小的整数倍。如果执行成功，则返回`0`；如果执行失败，则返回`-1`。[mprotect 函数用法](https://www.cnblogs.com/ims-/p/13222243.html)

### `AutoreleasePoolPage(AutoreleasePoolPage *newParent)/~AutoreleasePoolPage() `
&emsp;`AutoreleasePoolPage` 的构造函数，看到这里用了一个 `AutoreleasePoolPage *newParent` 作为参数，我们已知自动释放池的完整结构 是一个由 `AutoreleasePoolPage` 构成的双向链表，它的成员变量 `AutoreleasePoolPage * const parent` 和 `AutoreleasePoolPage *child` 作为前后两个链接节点的链接指针，那么 `parent` 和 `child` 谁在前谁在后呢？

根据 `AutoreleasePoolPageData` 的构造函数可知，第一个节点的 `parent`  和 `child` 都是 `nil`，当第一个 `AutoreleasePoolPage` 满了，会再创建一个 `AutoreleasePoolPage`，此时会拿第一个节点作为 `newParent` 参数来构建这第二个节点。即第一个节点的 `child` 指向第二个节点，第二个节点的 `parent` 指向第一个节点。

```c++
AutoreleasePoolPage(AutoreleasePoolPage *newParent) :
    AutoreleasePoolPageData(begin(),
                            // 当前所处的线程
                            objc_thread_self(),
                            // parent
                            newParent,
                            // 大概是序号，第一个节点的 depth 是 0，第二个节点是 1，第三个节点是 2....
                            newParent ? 1+newParent->depth : 0,
                            // 这个值目前还不清楚...
                            newParent ? newParent->hiwat : 0)
{ 
    if (parent) {
        // 检查 parent 节点是否合规
        parent->check();
        // parent 节点的 child 必须为 nil，因为当前构建的节点要作为 parent 的 child
        ASSERT(!parent->child);
        
        // 当前什么也不做
        parent->unprotect();
        
        // 把当前节点作为 parent 的 child 节点
        parent->child = this;
        
        // 但前什么也不做
        parent->protect();
    }
    
    // 当前什么也不做
    protect();
}

// 析构函数
~AutoreleasePoolPage() 
{
    // 检查
    check();
    
    // 当前什么也不做
    unprotect();
    
    // 必须为空
    ASSERT(empty());

    // Not recursive: we don't want to blow out the stack 
    // if a thread accumulates a stupendous amount of garbage
    
    // child 不存在
    ASSERT(!child);
}
```
&emsp;看到 `AutoreleasePoolPage` 必须满足 `empty()` 和 `child` 指向 `nil`，同时还有 `magic.check()` 必须为真，还有 `thread == objc_thread_self()`，这四个条件同时满足时才能正常析构。

### `busted/busted_die`
```c++
// 根据 log 参数不同会决定是 _objc_fatal 或 _objc_inform
template<typename Fn>
void busted(Fn log) const {
    // 一个完整默认值的 magic_t 变量 
    magic_t right;
    
    // log
    log("autorelease pool page %p corrupted\n"
         "  magic     0x%08x 0x%08x 0x%08x 0x%08x\n"
         "  should be 0x%08x 0x%08x 0x%08x 0x%08x\n"
         "  pthread   %p\n"
         "  should be %p\n", 
         this, 
         magic.m[0], magic.m[1], magic.m[2], magic.m[3], 
         right.m[0], right.m[1], right.m[2], right.m[3], 
         this->thread, objc_thread_self());
}

__attribute__((noinline, cold, noreturn))
void busted_die() const {
    // 执行 _objc_fatal 打印
    busted(_objc_fatal);
    __builtin_unreachable();
}
```
### `check/fastcheck`
&emsp;检查 `magic`是否等于默认值和当前所处的线程，然后 `log` 传递 `_objc_inform` 或 `_objc_fatal` 调用 `busted` 函数。 
```c++
    inline void
    check(bool die = true) const
    {
        if (!magic.check() || thread != objc_thread_self()) {
            if (die) {
                busted_die();
            } else {
                busted(_objc_inform);
            }
        }
    }

    inline void
    fastcheck() const
    {
    // #define CHECK_AUTORELEASEPOOL (DEBUG) // DEBUG 模式为 true RELEASE 模式为 false
#if CHECK_AUTORELEASEPOOL
        check();
#else
        if (! magic.fastcheck()) {
            busted_die();
        }
#endif
    }
```
### `begin/end/empty/full/lessThanHalfFull`

#### `begin`
&emsp;`begin` 函数超关键的，而且中间藏了一个很重要的点。首先要清楚一点 `begin` 是 `AutoreleasePoolPage` 中存放的 **自动释放对象** 的起点。回顾上面的的 `new` 函数的实现我们已知系统总共给 `AutoreleasePoolPage` 分配了 `4096` 个字节的空间，这么大的空间除了前面一部分空间用来保存 `AutoreleasePoolPage` 的成员变量外，剩余的空间都是用来存放放进自动释放池的对象的。

`AutoreleasePoolPage` 的成员变量都是继承自 `AutoreleasePoolPageDate`，它们总共需要 `56` 个字节的空间，然后剩余 `4040` 字节空间，一个对象指针占 `8` 个字节，那么一页 `AutoreleasePoolPage` 能存放 `505` 个需要自动释放的对象。（可在 `main.m` 中引入 `#include "NSObject-internal.h"` 打印 `sizeof(AutoreleasePoolPageData)` 的值确实是 `56`）

```c++
id * begin() {
    // (uint8_t *)this 是 AutoreleasePoolPage 的起始地址，
    // 且这里用的是 (uint8_t *) 的强制类型转换，uint8_t 占 1 个字节，
    // 然后保证 (uint8_t *)this 加 56 时是按 56 个字节前进的
    
    // sizeof(*this) 是 AutoreleasePoolPage 所有成员变量的宽度是 56 个字节
    return (id *) ((uint8_t *)this+sizeof(*this));
}
```
#### `end`
```c++
id * end() {
    // (uint8_t *)this 起始地址，转为 uint8_t 指针
    // 然后前进 SIZE 个字节，刚好到 AutoreleasePoolPage 的末尾
    return (id *) ((uint8_t *)this+SIZE);
}
```
#### `empty`
&emsp;`next` 指针指向的是当前自动释放池内最后面一个自动释放对象的后面，如果此时它指向 `begin` 的位置，表示目前自动释放池内没有存放自动释放对象。
```c++
bool empty() {
    return next == begin();
}
```
#### `full`
&emsp;理解了 `empty` 再看 `full` 也很容易理解，`next` 指向了 `end` 的位置，表明自动释放池内已经存满了需要自动释放的对象。
```c++
bool full() { 
    return next == end();
}
```
#### `lessThanHalfFull`
&emsp;表示目前自动释放池存储的自动释放对象是否少于总容量的一半。`next` 与 `begin` 的距离是当前存放的自动释放对象的个数，`end` 与 `begin` 的距离是可以存放自动释放对象的总容量。
```c++
bool lessThanHalfFull() {
    return (next - begin() < (end() - begin()) / 2);
}
```
### `add`
&emsp;把 `autorelease` 对象放进自动释放池，并返回指向该对象的指针。
```c++
id *add(id obj)
{
    // 如果自动释放池已经满了，则执行断言
    ASSERT(!full());
    unprotect(); // 什么都不做
    
    // 记录当前 next 的指向
    id *ret = next;  // faster than `return next-1` because of aliasing
    
    // next 是一个 objc_object **，先使用解引用操作符 * 取出 objc_object * 然后 obj 会赋值给它，然后 next 会做一次自增操作，指向下一个位置
    *next++ = obj;
    
    protect(); // 什么也不做
    
    // ret 目前正是存放 obj 的位置
    return ret;
}
```
### `releaseAll/releaseUntil`
```c++
void releaseAll() 
{
    // 调用 releaseUntil 并传入 begin
    // 从 next 开始，一直往后移动，直到 begin，把 begin 到 next 的所有自动释放对象执行 objc_release 操作
    releaseUntil(begin());
}
```
&emsp;从 `next` 开始一直向后移动直到到达 `stop`，把经过路径上的所有自动释放对象都执行一次 `objc_release` 操作。
```c++
    void releaseUntil(id *stop) 
    {
        // Not recursive: we don't want to blow out the stack 
        // if a thread accumulates a stupendous amount of garbage
        
        // 循环结束的条件就是从 next 开始，一直后退，直到 next 到达 stop
        while (this->next != stop) {
            // Restart from hotPage() every time, in case -release 
            // autoreleased more objects
            
            // 取得当前的 AutoreleasePoolPage
            AutoreleasePoolPage *page = hotPage();

            // fixme I think this `while` can be `if`, but I can't prove it
            // fixme 我认为“ while”可以是“ if”，但我无法证明
            // 我觉得也是可以用 if 代替 while，
            // 一个 page 满了生成链接下一个 page，所以从第一个 page 开始到 hotPage 的前一个page，应该都是满的
            
            // 如果当前 page 已经空了，则往后退一步，把前一个 AutoreleasePoolPage 作为 hotPage
            while (page->empty()) {
                // 当前 page 已经空了，还没到 stop，
                // 往后走，开始进入前一个 page
                page = page->parent;
                // 把前一个 page 作为 hotPage
                setHotPage(page);
            }
            
            // 当前什么都不做
            page->unprotect();
            
            // next 后移一步，并用解引用符取出 objc_object * 赋值给 obj
            id obj = *--page->next;
            
            // 把 page->next 开始的 sizeof(*page->next) 个字节置为 SCRIBBLE
            memset((void*)page->next, SCRIBBLE, sizeof(*page->next));
            
            // 当前什么都不做
            page->protect();
            
            // 如果 obj 不为 nil，则执行 objc_release 操作
            if (obj != POOL_BOUNDARY) {
                objc_release(obj);
            }
        }

        // 把当前 this 作为 hotPage
        setHotPage(this);

#if DEBUG
        // we expect any children to be completely empty
        // 保证从当前 page 的 child 开始，向后都是空 page
        for (AutoreleasePoolPage *page = child; page; page = page->child) {
            ASSERT(page->empty());
        }
#endif
    }
```
&emsp;从最前面的 `page` 开始一直向后移动直到到达 `stop` 所在的 `page`，并把经过的 `page` 里保存的对象都执行一次 `objc_release` 操作，经过的 `page` 都被清空了，每个 `page` 的 `next` 都指向了该 `page` 的 `begin`。整体操作清晰明了，一直遍历向后。
### `kill`
```c++
void kill() 
{
    // Not recursive: we don't want to blow out the stack 
    // if a thread accumulates a stupendous amount of garbage
    
    AutoreleasePoolPage *page = this;
    
    // 从当前 page 开始一直沿着 child 链往前走，直到最后一个 page
    while (page->child) page = page->child;

    // 临时变量
    AutoreleasePoolPage *deathptr;
    // 是 do while 循环，所以会至少进行一次 delete，即当前 page 也会被执行 delete
    do {
        // 要执行 delete 的 page
        deathptr = page;
        
        // 记录前一个 page
        page = page->parent;
        
        // 如果当前 page 的 parent 存在的话，要把这个 parent 的 child 置为 nil
        if (page) {
            page->unprotect();
            // child 置为 nil
            page->child = nil;
            page->protect();
        }
        // delete page
        delete deathptr;
    } while (deathptr != this);
}
```
&emsp;从当前的 `page` 开始，一直根据 `child` 链向前走直到 `child` 为空，把经过的 `page` 全部执行 `delete` 操作（包括当前 `page`）。
### `tls_dealloc`

// 🔫🔫 等下再回来重看
// Thread Local Storage

```c++
static void tls_dealloc(void *p) 
{
    // # define EMPTY_POOL_PLACEHOLDER ((id*)1)
    if (p == (void*)EMPTY_POOL_PLACEHOLDER) {
        // No objects or pool pages to clean up here.
        return;
    }

    // reinstate TLS value while we work
    setHotPage((AutoreleasePoolPage *)p);

    if (AutoreleasePoolPage *page = coldPage()) {
        if (!page->empty()) objc_autoreleasePoolPop(page->begin());  // pop all of the pools
        if (slowpath(DebugMissingPools || DebugPoolAllocation)) {
            // pop() killed the pages already
        } else {
            // 从 page 开始一直沿着 child 向前把所有的 page 执行 delete
            page->kill();  // free all of the pages
        }
    }
    
    // clear TLS value so TLS destruction doesn't loop
    setHotPage(nil);
}
```
### `pageForPointer`
&emsp;`void *p` 转为 `AutoreleasePoolPage *`，主要用于 `pool token` 转为 `AutoreleasePoolPage *`。`pool token` 是指向自动释放池的 `POOL_BOUNDARY` 的指针。
```c++
static AutoreleasePoolPage *pageForPointer(const void *p) 
{
    // 指针转为 unsigned long
    return pageForPointer((uintptr_t)p);
}

static AutoreleasePoolPage *pageForPointer(uintptr_t p) 
{
    // result 临时变量
    AutoreleasePoolPage *result;
    // p 对 1024 取模
    uintptr_t offset = p % SIZE;

    // 对 4096 取模，所以 offset 的值应该是在 0~4095 之间
    // sizeof(AutoreleasePoolPage) 的值应该和 sizeof(AutoreleasePoolPageData) 一样的，都是 56
    // offset 必须大于等于 56，是为什么呢？
    ASSERT(offset >= sizeof(AutoreleasePoolPage));

    // 然后把 offset 减掉，是为什么呢？
    result = (AutoreleasePoolPage *)(p - offset);
    
    // 验证 result 是否 magic.check() 和 thread == objc_thread_self()，两个必须满足的的条件
    result->fastcheck();

    return result;
}
```
### `haveEmptyPoolPlaceholder/setEmptyPoolPlaceholder`

// 🔫🔫 回过头再来看，还不知道这两个函数怎么用的

```c++
// 两个静态内联函数
static inline bool haveEmptyPoolPlaceholder()
{
    // key 是一个静态局部变量
    // static pthread_key_t const key = AUTORELEASE_POOL_KEY;
    // # define AUTORELEASE_POOL_KEY ((tls_key_t)__PTK_FRAMEWORK_OBJC_KEY3)
    // # define EMPTY_POOL_PLACEHOLDER ((id*)1)
    
    id *tls = (id *)tls_get_direct(key);
    return (tls == EMPTY_POOL_PLACEHOLDER);
}

static inline id* setEmptyPoolPlaceholder()
{
    // 设置
    ASSERT(tls_get_direct(key) == nil);
    tls_set_direct(key, (void *)EMPTY_POOL_PLACEHOLDER);
    return EMPTY_POOL_PLACEHOLDER;
}
```
### `hotPage/setHotPage`
```c++
static inline AutoreleasePoolPage *hotPage() 
{
    // 当前的 hotPage 是根据固定 key 保存在当前线程里面的吗 ？
    AutoreleasePoolPage *result = (AutoreleasePoolPage *)tls_get_direct(key);
    // 如果等于空标志的话，返回 nil
    if ((id *)result == EMPTY_POOL_PLACEHOLDER) return nil;
    // 执行一下 check 看是否符合约束规则
    if (result) result->fastcheck();
    return result;
}

static inline void setHotPage(AutoreleasePoolPage *page) 
{
    // page 入参检测，判断是否符合 AutoreleasePoolPage magic 的约束规则 
    if (page) page->fastcheck();
    // 设置
    tls_set_direct(key, (void *)page);
}
```
### `coldPage`
&emsp;"冷" `page`，首先找到 `hotPage` 然后沿着它的 `parent`，一直往后走，直到最后一个 `AutoreleasePoolPage` 并返回。
```c++
static inline AutoreleasePoolPage *coldPage() 
{
    // hotPage
    AutoreleasePoolPage *result = hotPage();
    if (result) {
        // 循环一直沿着 parent 指针往后找，直到第一个 AutoreleasePoolPage
        while (result->parent) {
        
            // 沿着 parent 更新 result
            result = result->parent;
            
            // 检测 result 符合 page 规则
            result->fastcheck();
        }
    }
    return result;
}
```
### `autoreleaseFast`
&emsp;把对象快速放进自动释放池。
```c++
static inline id *autoreleaseFast(id obj)
{
    // hotPage
    AutoreleasePoolPage *page = hotPage();
    if (page && !page->full()) {
        // 如果 page 存在并且 page 未满，则直接调用 add 函数把 obj 添加到 page
        return page->add(obj);
    } else if (page) {
        // 如果 page 满了，则调用 autoreleaseFullPage 构建新 AutoreleasePoolPage，并把 obj 添加进去
        return autoreleaseFullPage(obj, page);
    } else {
        // 如果 page 不存在，即当前线程还不存在自动释放池，则构建 AutoreleasePoolPage，并把 obj 添加进去
        return autoreleaseNoPage(obj);
    }
}
```
### `autoreleaseFullPage`
```c++
static __attribute__((noinline))
id *autoreleaseFullPage(id obj, AutoreleasePoolPage *page)
{
    // The hot page is full. 
    // Step to the next non-full page, adding a new page if necessary.
    // Then add the object to that page.
    // 如果 hotpage 满了，转到下一个未满的 page，必要时添加一个新的 page。  
    // 然后把 object 添加到新 page 里。
    
    // page 必须是 hotPage
    ASSERT(page == hotPage());
    // page 满了，或者...
    
    // OPTION( DebugPoolAllocation,
    //         OBJC_DEBUG_POOL_ALLOCATION,
    //         "halt when autorelease pools are popped out of order,
    //          and allow heap debuggers to track autorelease pools")
    // 自动释放池按顺序弹出时暂停，并允许堆调试器跟踪自动释放池
    
    ASSERT(page->full()  ||  DebugPoolAllocation);

    // do while 循环里面分为两种情况
    // 沿着 child 往前走，如果能找到一个非满的 page，则可以把 obj 放进去
    // 如果 child 不存在或者所有的 child 都满了，
    // 则构建一个新的 AutoreleasePoolPage 拼接在 AutoreleasePool 的双向链表中， 并把 obj 添加进新 page 里面
    do {
        if (page->child) page = page->child;
        else page = new AutoreleasePoolPage(page);
    } while (page->full());

    // 设置 page 为 hotPage
    setHotPage(page);
    
    // 把 obj 添加进 page 里面，返回值是在 page 里面的位置
    return page->add(obj);
}
```
### `autoreleaseNoPage`
```c++
static __attribute__((noinline))
id *autoreleaseNoPage(id obj)
{
    // "No page" could mean no pool has been pushed or an empty
    // placeholder pool has been pushed and has no contents yet
    // "No page" 可能意味着没有构建任何池，或者已经构建了一个空的占位符池，并且还没有内容。
    
    // hotPage 不存在，否则执行断言
    ASSERT(!hotPage());

    bool pushExtraBoundary = false;
    if (haveEmptyPoolPlaceholder()) {
        // We are pushing a second pool over the empty placeholder pool or pushing the first object into the empty placeholder pool.
        // Before doing that, push a pool boundary on behalf of the pool that is currently represented by the empty placeholder.
        pushExtraBoundary = true;
    }
    else if (obj != POOL_BOUNDARY  &&  DebugMissingPools) {
        // We are pushing an object with no pool in place, 
        // and no-pool debugging was requested by environment.
        _objc_inform("MISSING POOLS: (%p) Object %p of class %s "
                     "autoreleased with no pool in place - "
                     "just leaking - break on "
                     "objc_autoreleaseNoPool() to debug", 
                     objc_thread_self(), (void*)obj, object_getClassName(obj));
        objc_autoreleaseNoPool(obj);
        return nil;
    }
    else if (obj == POOL_BOUNDARY  &&  !DebugPoolAllocation) {
        // We are pushing a pool with no pool in place,
        // and alloc-per-pool debugging was not requested.
        // Install and return the empty pool placeholder.
        return setEmptyPoolPlaceholder();
    }

    // We are pushing an object or a non-placeholder'd pool.

    // Install the first page.
    AutoreleasePoolPage *page = new AutoreleasePoolPage(nil);
    setHotPage(page);
    
    // Push a boundary on behalf of the previously-placeholder'd pool.
    if (pushExtraBoundary) {
        page->add(POOL_BOUNDARY);
    }
    
    // Push the requested object or pool.
    return page->add(obj);
}
```


## `Autorelease` 对象什么时候释放？

## `Tagged Pointer` 什么时候释放？



## 参考链接
**参考链接:🔗**
+ [黑幕背后的Autorelease](http://blog.sunnyxx.com/2014/10/15/behind-autorelease/)
+ [GCC扩展 __attribute__ ((visibility("hidden")))](https://www.cnblogs.com/lixiaofei1987/p/3198665.html)
+ [Linux下__attribute__((visibility ("default")))的使用](https://blog.csdn.net/fengbingchun/article/details/78898623)
+ [操作系统内存管理(思维导图详解)](https://blog.csdn.net/hguisu/article/details/5713164)

