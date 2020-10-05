#  iOS 从源码解析Runtime (九)：聚焦 objc_class(cache_t 及方法缓存实现相关内容篇(2))

> 上篇分析了 `bucket_t` 和 `cache_t` 的几乎全部内容，最后由于篇幅限制剩两个函数留在本篇来分析，然后准备接着分析 `objc-cache.mm` 文件中与 `objc-cache.h` 文件对应的几个核心函数，正是由它们构成了完整的方法缓存实现，那么一起 ⛽️⛽️ 吧！

> 这篇文章发的太晚了，主要是这几天时间都花在看汇编上了，我的汇编水平大概只是一年前看过王爽老师的那本汇编的书，然后就没怎么接触过了，感觉接下来的源码学习涉及到汇编的地方太多了，所以还是特别有必要对汇编做一个整体的认知和学习的，而不是单单只知道寄存器和单个指令是什么意思。本篇后半部分对 `objc-msg-arm64.s` 文件的每一行都做到了分析，那么一起⛽️⛽️吧！


## `insert`
&emsp;把指定的 `sel` 和 `imp` 插入到 `cache_t` 中，如果开始是空状态，则首先会初始一个容量为 4 散列数组再进行插入。其它情况插入之前会计算已用的容量占比是否到了临界值，如果是则首先进行扩容，然后再进行插入操作，如果还没有达到则直接插入。插入操作如果发生了哈希冲突则依次进行 `+1/-1` 的哈希探测。
```c++
ALWAYS_INLINE
void cache_t::insert(Class cls, SEL sel, IMP imp, id receiver)
{
#if CONFIG_USE_CACHE_LOCK
    cacheUpdateLock.assertLocked();
#else
    // 加锁，如果加锁失败则执行断言
    runtimeLock.assertLocked(); // 同样，__objc2__ 下使用 runtimeLock 
#endif
    
    // 断言 sel 不能是 0 且 cls 已经完成初始化
    ASSERT(sel != 0 && cls->isInitialized());

    // Use the cache as-is if it is less than 3/4 full.
    // 如果缓存占用少于 3/4 则可以继续保持原样使用。
    
    // 记录新的已占用量（旧已占用量加 1）
    mask_t newOccupied = occupied() + 1;
    
    // 旧容量
    unsigned oldCapacity = capacity(), capacity = oldCapacity;
    
    if (slowpath(isConstantEmptyCache())) { // 很可能为假
        // 如果目前是空缓存的话，空缓存只是 static bucket_t **emptyBucketsList 用来占位的，
        // 实际并不存储 bucket_t，我们需要重新申请空间，替换空缓存。
        // Cache is read-only. Replace it.
        
        if (!capacity) capacity = INIT_CACHE_SIZE; // 如果 capacity 为 0，则赋值给初始值 4
        // 根据 capacity 申请新空间并初始化 buckets、mask(capacity - 1)、_occupied 
        // 这里还有一个点，由于旧 buckets 是准备的占位的静态数据是不需要释放的，
        // 所有最后一个参数传递的是 false
        reallocate(oldCapacity, capacity, /* freeOld */false);
    }
    else if (fastpath(newOccupied + CACHE_END_MARKER <= capacity / 4 * 3)) { 
        // 大部分情况都在这里
        // Cache is less than 3/4 full. Use it as-is.
        // 缓存占用少于 3/4 的空间。照原样使用。
        
        // 小括号里面加了一个 CACHE_END_MARKER
        // 是因为在 __arm__  ||  __x86_64__  ||  __i386__ 这些平台下，
        // 会在 buckets 的末尾放一个 bucket_t *end，所以这里又加了 1
        // 而 __arm64__ 平台下则不存在这个多 +1
    }
    else {
        // 第三种情况则是需要对散列表空间进行扩容
        // 扩大为原始 capacity 的 2 倍
        // 且这里的扩容时为了性能考虑是不会把旧的缓存复制到新空间的。
        
        capacity = capacity ? capacity * 2 : INIT_CACHE_SIZE;
        
        // 如果大于 MAX_CACHE_SIZE，则使用 MAX_CACHE_SIZE(1 << 16)
        if (capacity > MAX_CACHE_SIZE) {
            capacity = MAX_CACHE_SIZE;
        }
        
        // 申请空间并做一些初始化
        // 不同与 isConstantEmptyCache 的情况，这里扩容后需要释放旧的 buckets，
        // 所以这里第三个参数传的是 true，表示需要释放旧 buckets，而这里它也不是立即释放的，
        // 在旧 buckets 没有被使用并且收集的旧 buckets 容量已经到达阀值了，
        // 则会真正进行内存空间的释放
        reallocate(oldCapacity, capacity, true);
    }

    // 临时变量
    bucket_t *b = buckets();
    mask_t m = capacity - 1;
    
    // 使用 sel 和 _mask 进行哈希计算，取得 sel 的哈希值 
    mask_t begin = cache_hash(sel, m);
    mask_t i = begin;

    // Scan for the first unused slot and insert there.
    // 扫描第一个未使用的 "插槽"，然后将 bucket_t 插入其中。
    
    // There is guaranteed to be an empty slot because the
    // minimum size is 4 and we resized at 3/4 full.
    // 保证有一个空插槽，因为最小大小为4，
    // 且上面已经做过判断如果使用占比超过 3/4 则进行扩容，
    // 且这里的扩容为了性能考虑是不会把旧的缓存复制到新空间的，
    // 旧 buckets 会被抛弃，并在合适时候释放其内存空间
    
    // 这里如果发生哈希冲突的话 do while 会进行一个线性的哈希探测(开放寻址法)，
    // 为 sel 和 imp 找一个空位
    do {
        if (fastpath(b[i].sel() == 0)) {
            // 如果 self 为 0，则表示 sel 的哈希值对应的下标处刚好是一个空位置，
            // 直接把 sel 和 imp 放在此处即可
            
            // occupied 已占用数量 +1 
            incrementOccupied();
            
            // 以原子方式把 sel 和 imp 保存在 Bucket_t 的 _sel 和 _imp 中 
            b[i].set<Atomic, Encoded>(sel, imp, cls);
            
            return;
        }
        if (b[i].sel() == sel) {
            // The entry was added to the cache by some other
            // thread before we grabbed the cacheUpdateLock.
            // 在 cacheUpdateLock(runtimeLock) 加锁之前，该 sel/imp 已由其他一些线程添加到缓存中。
            
            return;
        }
        
      // 下一个哈希值探测，这里不同的平台不同处理方式依次 +1 或者 -1
    } while (fastpath((i = cache_next(i, m)) != begin));

    // 
    cache_t::bad_cache(receiver, (SEL)sel, cls);
}
```
### `INIT_CACHE_SIZE`
```c++
/* 
Initial cache bucket count. INIT_CACHE_SIZE must be a power of two.
初始化缓存桶的容量。INIT_CACHE_SIZE 必须为 2 的幂
*/
enum {
    INIT_CACHE_SIZE_LOG2 = 2,
    INIT_CACHE_SIZE      = (1 << INIT_CACHE_SIZE_LOG2), // 1 << 2 = 0b100 = 4
    MAX_CACHE_SIZE_LOG2  = 16,
    MAX_CACHE_SIZE       = (1 << MAX_CACHE_SIZE_LOG2), // 1 << 16 = 2^16 
};
```
### `cache_hash`
```c++
// Class points to cache. SEL is key. Cache buckets store SEL+IMP.
// 类指向缓存。 SEL 是 key。Cache 的 buckets 中保存 SEL+IMP(即 struct bucket_t)。

// Caches are never built in the dyld shared cache.
// Caches 永远不会构建在 dyld 共享缓存中。

static inline mask_t cache_hash(SEL sel, mask_t mask) 
{
    // 觉的 hash 值计算好随意，就是拿 sel 和 mask 与一下，保证不会越界
    return (mask_t)(uintptr_t)sel & mask;
}
```
### `cache_next`
&emsp;这是是 `sel` 发生哈希冲突时，哈希值的移动探测方式在不同的平台下有不同的处理。
```c++
#if __arm__  ||  __x86_64__  ||  __i386__
// objc_msgSend has few registers available.
// objc_msgSend 的可用寄存器很少。

// Cache scan increments and wraps at special end-marking bucket.
// 缓存扫描增量包裹在特殊的末端标记桶上。
//（此处应该说的是 CACHE_END_MARKER 是 1 时的 endMarker 的位置在 buckets 首位）

#define CACHE_END_MARKER 1

// i 每次向后移动 1，与 mask，保证不会越界
//（并且是到达 mask 后再和 mask 与操作会是 0 ，此时则从 buckets 的 0 下标处开始，
// 然后再依次向后移动探测直到到达 begin，如果还没有找到合适位置，那说明发生了内存错误问题）

static inline mask_t cache_next(mask_t i, mask_t mask) {
    return (i+1) & mask;
}

#elif __arm64__
// objc_msgSend has lots of registers available.
// objc_msgSend 有很多可用的寄存器。
// Cache scan decrements. No end marker needed.
// 缓存扫描减量。无需结束标记。
//（此处说的是 CACHE_END_MARKER 是 0 时，不存在 endMarker 赋值）

#define CACHE_END_MARKER 0

// i 依次递减
static inline mask_t cache_next(mask_t i, mask_t mask) {
    return i ? i-1 : mask;
}

#else

// 未知架构
#error unknown architecture

#endif
```
### `bad_cache`
```c++
void cache_t::bad_cache(id receiver, SEL sel, Class isa)
{
    // Log in separate steps in case the logging itself causes a crash.
    // 请分别登录，以防日志记录本身导致崩溃。
    
    _objc_inform_now_and_on_crash
        ("Method cache corrupted. This may be a message to an "
         "invalid object, or a memory error somewhere else.");
         
    // 取得 cache     
    cache_t *cache = &isa->cache;
    
    // 不同的平台处理 buckets 和 mask
#if CACHE_MASK_STORAGE == CACHE_MASK_STORAGE_OUTLINED
    bucket_t *buckets = cache->_buckets.load(memory_order::memory_order_relaxed);
    _objc_inform_now_and_on_crash
        ("%s %p, SEL %p, isa %p, cache %p, buckets %p, "
         "mask 0x%x, occupied 0x%x", 
         receiver ? "receiver" : "unused", receiver, 
         sel, isa, cache, buckets,
         cache->_mask.load(memory_order::memory_order_relaxed),
         cache->_occupied);
    _objc_inform_now_and_on_crash
        ("%s %zu bytes, buckets %zu bytes", 
         receiver ? "receiver" : "unused", malloc_size(receiver), 
         malloc_size(buckets));
#elif (CACHE_MASK_STORAGE == CACHE_MASK_STORAGE_HIGH_16 || \
       CACHE_MASK_STORAGE == CACHE_MASK_STORAGE_LOW_4)
    uintptr_t maskAndBuckets = cache->_maskAndBuckets.load(memory_order::memory_order_relaxed);
    _objc_inform_now_and_on_crash
        ("%s %p, SEL %p, isa %p, cache %p, buckets and mask 0x%lx, "
         "occupied 0x%x",
         receiver ? "receiver" : "unused", receiver,
         sel, isa, cache, maskAndBuckets,
         cache->_occupied);
    _objc_inform_now_and_on_crash
        ("%s %zu bytes, buckets %zu bytes",
         receiver ? "receiver" : "unused", malloc_size(receiver),
         malloc_size(cache->buckets()));
#else

// 未知的缓存掩码存储类型
#error Unknown cache mask storage type.

#endif

// SEL 只是表示方法名的字符串（强制转换为 const char * 类型）
// const char *sel_getName(SEL sel) 
// {
//     if (!sel) return "<null selector>";
//     return (const char *)(const void*)sel;
// }

    // sel
    _objc_inform_now_and_on_crash
        ("selector '%s'", sel_getName(sel));
        
    // 类名
    _objc_inform_now_and_on_crash
        ("isa '%s'", isa->nameForLogging());

    _objc_fatal
        ("Method cache corrupted. This may be a message to an "
         "invalid object, or a memory error somewhere else.");
}
```
&emsp;到这里 `bucket_t` 和 `cache_t` 定义的内容就全部看完了。接下来我们分析 `objc-cache.h` 中的内容。
## `objc-cache.h`
```c++
// objc-cache.h 文件的全部内容
#ifndef _OBJC_CACHE_H
#define _OBJC_CACHE_H

#include "objc-private.h"

__BEGIN_DECLS

extern void cache_init(void); // 初始化
extern IMP cache_getImp(Class cls, SEL sel); // 获得指定的 IMP
extern void cache_fill(Class cls, SEL sel, IMP imp, id receiver); // // 
extern void cache_erase_nolock(Class cls); // 重置缓存
extern void cache_delete(Class cls); // 删除 buckets
extern void cache_collect(bool collectALot); //旧 buckets 回收

__END_DECLS

#endif
```
### `cache_init`
```c++
// Define HAVE_TASK_RESTARTABLE_RANGES to enable usage of task_restartable_ranges_synchronize()
// 定义 HAVE_TASK_RESTARTABLE_RANGES 以启用使用 task_restartable_ranges_synchronize() 函数

// 任务可 重新开始/可重新启动的 范围/区间

// #if TARGET_OS_SIMULATOR || defined(__i386__) || defined(__arm__) || !TARGET_OS_MAC
// #   define HAVE_TASK_RESTARTABLE_RANGES 0
// #else
//     看到我们的 x86_64 和 arm64 平台下都是 1
// #   define HAVE_TASK_RESTARTABLE_RANGES 1
// #endif

void cache_init()
{
#if HAVE_TASK_RESTARTABLE_RANGES
    // unsigned int
    mach_msg_type_number_t count = 0;
    // int
    kern_return_t kr;

    // typedef struct {
    //   uint64_t          location; // 位置
    //   unsigned short    length; // 长度
    //   unsigned short    recovery_offs; // 偏移
    //   unsigned int      flags; // 标志位
    // } task_restartable_range_t;
    
    // extern "C" task_restartable_range_t objc_restartableRanges[];
    
    // 统计某种东西
    while (objc_restartableRanges[count].location) {
        count++;
    }

    // extern mach_port_t      mach_task_self_;
    // #define mach_task_self() mach_task_self_
    // #define current_task()  mach_task_self()
    
    // register
    kr = task_restartable_ranges_register(mach_task_self(),
                                          objc_restartableRanges, count);
                                          
    if (kr == KERN_SUCCESS) return; // 如果成功则 return 
    
    // 如果失败则 crash
    _objc_fatal("task_restartable_ranges_register failed (result 0x%x: %s)",
                kr, mach_error_string(kr));
                
#endif // HAVE_TASK_RESTARTABLE_RANGES
}
```
### `cache_getImp`
&emsp;`cache_getImp` 函数竟然是个汇编函数。（突然莫名兴奋，终于找到需要认真复习总结汇编的理由了，之前看王爽老师的汇编书现在差不多已经忘的干净，终于可以重拾汇编了。🎉🎉）
### `cache_fill`
```c++
void cache_fill(Class cls, SEL sel, IMP imp, id receiver)
{
    // 直接使用 runtimeLock 加锁，加锁失败则执行断言
    //（这里怎么不用那个 cacheUpdateLock 和 runtimeLock 使用哪个锁的判定了）
    runtimeLock.assertLocked();

#if !DEBUG_TASK_THREADS
    // Never cache before +initialize is done.
    // 在 +initialize 完成之前不进行缓存。
    
    if (cls->isInitialized()) {
        // 取得 Class 的 cache 
        cache_t *cache = getCache(cls);
        
#if CONFIG_USE_CACHE_LOCK // __OBJC2__ 下 cache 不使用 lock 
        mutex_locker_t lock(cacheUpdateLock);
#endif

        // 插入
        cache->insert(cls, sel, imp, receiver);
    }
    
#else
    // 进行验证
    _collecting_in_critical();
    
#endif
}
```
#### `DEBUG_TASK_THREADS`
```c++
/*
objc_task_threads 
Replacement for task_threads(). 
Define DEBUG_TASK_THREADS to debug crashes when task_threads() is failing.
定义 DEBUG_TASK_THREADS 以在 task_threads() 失败时调试 crash。

A failure in task_threads() usually means somebody has botched their Mach or MIG traffic. 
task_threads() 失败通常意味着有人破坏了他们的 Mach 或 MIG 通信量。
 
For example, somebody's error handling was wrong and they left a message queued on the MIG reply port for task_threads() to trip over.
例如，有人的错误处理是错误的，他们在 MIG 应答端口上留下了一条消息，让 task_threads() "跳闸/绊倒"。

The code below is a modified version of task_threads(). 
下面的代码是 task_threads() 的修改版本。

It logs the msgh_id of the reply message. The msgh_id can identify the sender of the message, which can help pinpoint the faulty code.
它记录回复消息的 msgh_id。msgh_id 可以识别消息的发送者，这可以帮助查明错误的代码。

DEBUG_TASK_THREADS also calls collecting_in_critical() during every message dispatch, which can increase reproducibility of bugs.
DEBUG_TASK_THREADS 还会在每次消息分发期间调用 collection_in_critical()，这可以提高错误的可重复性。

This code can be regenerated by running `mig /usr/include/mach/task.defs`.
可以通过运行 `mig /usr/include/mach/task.defs` 来重新生成该代码。
*/
```
### `cache_erase_nolock`
&emsp;`cache_erase_nolock` 函数的作用是为把 `cache` 置为 “空状态”，并回收旧`buckets`。 
```c++
// Reset this entire cache to the uncached lookup by reallocating it.
// 通过重新分配整个缓存，将其重置为 未缓存的查询(uncached lookup)。(重新分配整个缓存的时候是不会把旧缓存再放入新缓存内存空间里面的)

// This must not shrink the cache - that breaks the lock-free scheme.
// 这一定不能缩小缓存 - 这会破坏无锁方案。

void cache_erase_nolock(Class cls)
{
#if CONFIG_USE_CACHE_LOCK
    cacheUpdateLock.assertLocked();
#else
    runtimeLock.assertLocked(); // __OBJC2__ 下加锁，加锁失败则执行断言
#endif
    
    // 取得 cache
    cache_t *cache = getCache(cls);
    
    // cache 容量，return mask() ? mask()+1 : 0; 
    mask_t capacity = cache->capacity();
    
    if (capacity > 0  &&  cache->occupied() > 0) {
        // 容量大于 0 并且已占用也大于 0
        
        // 取得 buckets，bucket_t *
        auto oldBuckets = cache->buckets();
        
        // 取得一个空 buckets（标记用，实际保存 bucket_t 时会重新申请空间）
        // buckets 是一个全局的 cache_t::emptyBuckets() 或是在静态 emptyBucketsList 中准备一个 buckets
        // (emptyBucketsForCapacity 函数的 allocate 参数默认是 true，
        //  当指定 capacity 的字节数大于 EMPTY_BYTES 时会申请空间)
        
        auto buckets = emptyBucketsForCapacity(capacity);
        
        // 设置 _buckets 和 _mask 同时也会把 _occupied 置 0
        cache->setBucketsAndMask(buckets, capacity - 1); // also clears occupied

        // 把旧 buckets 回收起来等待释放
        cache_collect_free(oldBuckets, capacity);
    }
}
```
### `cache_delete`
```c++
void cache_delete(Class cls)
{
#if CONFIG_USE_CACHE_LOCK
    mutex_locker_t lock(cacheUpdateLock);
#else
    runtimeLock.assertLocked(); // 加锁，加锁失败会执行断言
#endif

    // 判断是否可以进行释放操作
    if (cls->cache.canBeFreed()) {
        // !isConstantEmptyCache(); 
        // !(occupied() == 0 && buckets() == emptyBucketsForCapacity(capacity(), false))
        
        // 是否记录了待释放的 buckets，此时要执行释放了，-1
        if (PrintCaches) recordDeadCache(cls->cache.capacity());
        
        // 释放 buckets 的内存
        free(cls->cache.buckets());
    }
}
```
### `cache_collect`
&emsp;`void cache_collect(bool collectALot)` 函数的功能是尝试去释放旧的 `buckets`。`collectALot` 参数表示是否尽力去尝试释放旧 `buckets` 的内存（即使目前处于待释放的 `buckets` 的内存占用少于阀值（`32*1024`），也尽力去尝试释放内存）。函数本体的话首先是加锁，然后如果待释放的 `buckets` 的内存占比小于阀值并且 `collectALot` 为 `false` 则直接 `return`，如果上述条件为 `false`，则继续进行是否能释放的判断，如果 `collectALot` 为 `false`，则判断是否有 `objc_msgSend`（或其他 `cache reader`）当前正在查找缓存，并且可能仍在使用一些待释放的 `buckets`，则此时直接返回。如果 `collectALot` 为 `true`，则一直循环等待 `_collecting_in_critical()` 直到没有  `objc_msgSend`（或其他 `cache reader`）正在查找缓存。然后接下来就是可以正常的进行释放了，并同时把 `garbage` 的标记值置为 0，表示为初始状态。更详细的内容可参看上篇。

&emsp;到这里 `objc-cache.mm` 中除了跟线程相关的内容（由于线程相关的操作过于复杂这里就不展开讲了，以目前的水平真心看不懂，而且能找到的资料甚少，目前只需要知道线程会有自己的存储空间并根据几个指定的`key` 来保存一些信息就好了。其他相关的内容等深入学习线程相关内容的时候再深入探究）就全部看完了，接下来我们还有一个最重要的的汇编函数 `cache_getImp`，没错，它是用汇编来实现的，本人的汇编水平仅限于大概一年前看过王爽老师的一本汇编书籍外，别的对汇编好像一无所知，但是没关系其中涉及的指令并不复杂，如果我们上面已经深入学习了 `bucket_t` 和 `cache_t` 的结构的话，是一定能看的懂的，硬理解的话，无非就是我们日常的指针操作变成了寄存器操作而已，并不难理解，我们只需要专注于指令执行过程就好。
相信所有开发者都听说过  `Objective-C` 的消息发送流程的一些知识点，而方法缓存就是为消息发送流程来服务的，此时如果继续学习下去的话我们需要对消息发送流程有一个认知，要发送消息那总得先有消息吧，那这消息从哪来要到哪去呢，这就涉及我们的 `objc_msgSend` 函数的执行流程了，那么一起来学习吧。

## `objc_msgSend`
### `objc_msgSend` 是从哪里来
&emsp;首先我们使用控制台做一些 `cache_t` 结构的验证。
```c++
// LGPerson.h
@interface LGPerson : NSObject

// 实例方法
- (void)instanceMethod1;
- (void)instanceMethod2;
- (void)instanceMethod3;
- (void)instanceMethod4;
- (void)instanceMethod5;
- (void)instanceMethod6;
- (void)instanceMethod7;

@end

// 在 main.m 中编写如下调用

LGPerson *person = [LGPerson alloc];
LGPerson *p = [person init]; // ⬅️ 此行打断点

[p instanceMethod1];
[p instanceMethod2];
[p instanceMethod3];
[p instanceMethod4];
[p instanceMethod5];
[p instanceMethod6];
[p instanceMethod7];
```
&emsp;控制台打印如下：
```c++
// 打印类信息
(lldb) p [person class]
(Class) $0 = LGPerson

// 根据 objc_class 的结构可知， 0x1000021e8 即 cache 成员变量的起始地址

(lldb) x/4gx $0
0x1000021d8: 0x00000001000021b0 （isa） 0x00000001003ee140 (superclass)
0x1000021e8: 0x0000000100677860 0x0002801000000003 (cache_t)

(lldb) p (cache_t *)0x1000021e8 // 强制转换为 cache_t 指针
(cache_t *) $1 = 0x00000001000021e8

// 直接对 cache_t 指针进行解引用看它内部内容
(lldb) p *$1

// 目前我们是在 x86_64 平台下，所以 cache_t 的结构是 CACHE_MASK_STORAGE_OUTLINED 类型下，没有掩码的形式
(cache_t) $2 = {

  // bucket_t 指针，std::__1::atomic 是 c++ 的原子操作，这里我们只关注 <> 里面的模版抽象类型即可
  _buckets = {
    std::__1::atomic<bucket_t *> = 0x0000000100677860 {
      _sel = {
        std::__1::atomic<objc_selector *> = 0x00007fff70893e54
      }
      _imp = {
        std::__1::atomic<unsigned long> = 4041432
      }
    }
  }
  
  // mask 为 3，那么 capacity 就是 4，（之前看源码已知 chache_t 哈希数组的初始长度正是 4）
  _mask = {
    std::__1::atomic<unsigned int> = 3
  }
  
  _flags = 32784
  
  // 根据上面的代码看我们刚调用了一个 [LGPerson clloc] 函数
  // 此时占用是 2
  
  _occupied = 2
}

// 继续往下打印 _buckets 的内容看一下
(lldb) p (bucket_t *)$1->buckets()
(bucket_t *) $4 = 0x0000000100677860

// 上面的 _occupied 表示当前占用是 2
// 下面打印看到只有 $[0] 有值，后面都是 0，且当前 _buckets 是一个长度为 4 的 bucket_t 指针数组
// bucket_t 只有 _sel 和 _imp 两个成员变量

// 由于目前博主使用的是 xcode 12 貌似 runtime 里面的相关类都被苹果屏蔽了，无法再进行这些代码的测试
// 这些测试打印都是我之前在 xcode 11 上测试留下来的记录做的摘抄 😭，暂时只能这样将就看了。

// 不然的话应该能用 NSString *NSStringFromSelector(SEL aSelector) 函数取得 _sel 的名字看下是什么，
// 目前只能看一个十六进制的地址也看不出它到底是谁

(lldb) p $4[0]
(bucket_t) $5 = {
  _sel = {
    std::__1::atomic<objc_selector *> = 0x00007fff70893e54
  }
  _imp = {
    std::__1::atomic<unsigned long> = 4041432
  }
}

// 0
(lldb) p $4[1]
(bucket_t) $6 = {
  _sel = {
    std::__1::atomic<objc_selector *> = 0x0000000000000000
  }
  _imp = {
    std::__1::atomic<unsigned long> = 0
  }
}

// 0
(lldb) p $4[2]
(bucket_t) $7 = {
  _sel = {
    std::__1::atomic<objc_selector *> = 0x0000000000000000
  }
  _imp = {
    std::__1::atomic<unsigned long> = 0
  }
}
```
&emsp;通过 `Class cls = NSClassFromString(@"LGPerson");` 方式得到 `cls`，打印 `cls` 里面的缓存信息，发现 `mask = 0, _occupied = 0`，然后 `LGPerson *person = [LGPerson alloc];` 初始化之后，再打印，发现 `mask = 3, _occupied = 2`，也就是说在 `alloc` 过程中，进行了缓存操作。
```c++
// 只调用 NSClassFromString(@"LGPerson") 函数，获取 LGPerson
Class cls = NSClassFromString(@"LGPerson");
...
// 打印 cache_t
(cache_t) $3 = {
  _buckets = {
    std::__1::atomic<bucket_t *> = 0x00000001003e8490 {
      _sel = {
        std::__1::atomic<objc_selector *> = 0x0000000000000000
      }
      _imp = {
        std::__1::atomic<unsigned long> = 0
      }
    }
  }
  // mask 值是 0
  _mask = {
    std::__1::atomic<unsigned int> = 0
  }
  
  _flags = 16
  
  // 已占用也是 0 
  _occupied = 0 
}

// 断点执行到 [persont init] 处，再进行打印
// 所使用的命令列表同上
p [person class]
x/4gx $0
p (cache_t *)0x1000021f0
p *$1

...
_mask = {
  std::__1::atomic<unsigned int> = 3
}
_flags = 32784

// 已占用是 2
_occupied = 2 
...

// 执行到 init 以后再进行打印
...
// mask 的值是 3
_mask = {
  std::__1::atomic<unsigned int> = 3
}
_flags = 32784

// 已占用也是 3 
_occupied = 3
...
```
&emsp;然后是连续调用上面的 7 个实例函数，统计出的 `_capacity` `_mask` `_occupied` 三个成员变量的值:
| _capacity | 4 | 4 | 8 | 8 | 8 | 8 | 8 | 8 | 16 |
| _mask | 3 | 3 | 7 | 7 | 7 | 7 | 7 | 7 | 15 |
| _occupied | 2 | 3 | 1 | 2 | 3 | 4 | 5 | 6 | 1 |

&emsp;可看到 `_occupied` 每次达到 `_capacity` 的 `3/4` 以后都会进行扩容，扩容的话是每次扩大 2 倍。然后 `_occupied` 每次扩容以后又从 `1` 开始，也证明了上面的结论，`cache_t` 扩容以后后直接舍弃旧的 `buckets`。

那看了半天 `objc_msgSend` 怎么还没有呈现呢，那么现在就开始：
```c++
// LGPerson.h
@interface LGPerson : NSObject
- (void)method1;
- (NSString *)methodWithReturn;
- (NSString *)method:(NSInteger)param;
@end

// main.m
LGPerson *person = [[LGPerson alloc] init];
[person method1];
[person methodWithReturn];
[person method:11];
```
&emsp;然后我们在终端执行 `clang -rewrite-objc main.m` 指令，把 `main.m` 转化为 `main.cpp` 文件，查看 `main.cpp` 文件，摘出 `main` 函数的内容：
```c++
int main(int argc, const char * argv[]) {

/* @autoreleasepool */ { __AtAutoreleasePool __autoreleasepool; 
 
NSLog((NSString *)&__NSConstantStringImpl__var_folders_l0_ntvl5rs97t30j69kh6g3vb_c0000gn_T_main_416c0e_mi_0);
 
LGPerson *person = ((LGPerson *(*)(id, SEL))(void *)objc_msgSend)((id)((LGPerson *(*)(id, SEL))(void *)objc_msgSend)((id)objc_getClass("LGPerson"), sel_registerName("alloc")), sel_registerName("init"));
 
((void (*)(id, SEL))(void *)objc_msgSend)((id)person, sel_registerName("method1"));
 
((NSString *(*)(id, SEL))(void *)objc_msgSend)((id)person, sel_registerName("methodWithReturn"));

((NSString *(*)(id, SEL, NSInteger))(void *)objc_msgSend)((id)person, sel_registerName("method:"), (NSInteger)11);

} // 对应上面 autoreleasepool 结束的右边大括号

return 0;
}

// 分析其中的函数调用会发现每次函数调用时 objc_msgSend 都被转换成了不同的函数指针：
// [person method1];
// 返回值为空参数是 id 和 SEL：(void (*)(id, SEL))
// id 是调用函数的 person 对象
// SEL 是 sel_registerName("method1")

// [person method:11];
// 返回值 (NSString *(*)(id, SEL, NSInteger))
...
```
&emsp;看到这里发现我们日常编写的 `OC` 函数调用其实是被转化为 `objc_msgSend` 函数，此函数我们之前也多次见过，例如我们前几天刚看到的 `((id(*)(objc_object *, SEL))objc_msgSend)(this, @selector(retain));` 当重写了 `retain` 函数时会这样去调用。前面我们分析 `bucket_t` 时多次提到 `SEL` 是函数名字的字符串 `IMP` 是函数的地址，而函数执行的本质就是去找到函数的地址然后执行它，而这正是 `objc_msgSend` 所做的事情，再具体一点就是在 `id` 上找到 `SEL` 函数的地址并执行它。那么 `objc_msgSend` 是怎么实现的呢？乍看它以为是一个 `C/C++` 函数，但它其实是汇编实现的。
使用汇编的原因，除了 “快速，方法的查找操作是很频繁的，汇编是相对底层的语言更容易被机器识别，节省中间的一些编译过程”  还有一些重要的原因，可参考这篇 [翻译-为什么objc_msgSend必须用汇编实现](http://tutuge.me/2016/06/19/translation-why-objcmsgsend-must-be-written-in-assembly/)

### `objc_msgSend` 汇编实现

&emsp;在 `objc4-781/Source` 文件夹下面，我们能看到几个后缀是 `.s` 的文件，没错它们正是汇编文件，且在每个文件的名字后面都包含一个 `-arm/-arm64/-i386/-x86-64` 以及 `-simulator-i386/-simulator-x86-64` 的后缀，它们所表明的正是此汇编文件所对应的平台。那么下面我们就解读一下 `objc-msg-arm64.s` 文件。 

### `objc-msg-arm64.s`

#### ``
```c++
/*
 * objc-msg-arm64.s - ARM64 code to support objc messaging
 * objc-msg-arm64.s - 支持 objc 消息传递的 ARM64 代码
 */
```
```c++
#ifdef __arm64__ // 限定属于 __arm64__ 平台

#include <arm/arch.h>
#include "isa.h"
#include "arm64-asm.h"
#include "objc-config.h"

// 汇编程序中以 . 开头的名称并不是指令的助记符，不会被翻译成机器指令，而是给汇编器一些特殊提示，
// 称为汇编指示（Assembler Directive）或伪操作（Pseudo-operation），由于它不是真正的指令所以加个 "伪" 字。

// .section 指示把代码划分成若干个段（Section），程序被操作系统加载执行时，每个段被加载到不同的地址，
// 操作系统对不同的页面设置不同的读、写、执行权限。

// .section .data 
// .data 段保存程序的数据，是可读可写的，相当于 C 程序的全局变量。

// .section .text 
// .text 段保存代码，是只读和可执行的，后面那些指令都属于 .text 段。

// .section 分段，可以通过 .section 伪操作来自定义一个段
// .section expr; // expr 可以是 .text/.data/.bss
// .text 将定义符开始的代码编译到代码段
// .data 将定义符开始的数据编译到数据段
// .bss 将变量存放到 .bss 段，bss 段通常是指用来存放程序中未初始化的全局变量的一块内存区域，
// 数据段通常是指用来存放程序中已初始化的全局变量的一块内存区域
// 注意：源程序中 .bss 段应该在 .text 之前

.data // 表示将定义符开始的数据编译到数据段

// _objc_restartableRanges is used by method dispatch caching code to figure out whether
// any threads are actively in the cache for dispatching. 
// The labels surround the asm code that do cache lookups.
// The tables are zero-terminated.

// 方法调度缓存代码使用 _objc_restartableRanges 
// 来确定是否有任何线程在 缓存 中处于活动状态以进行调度。
// labels 围绕着执行缓存查找的 asm 代码。这些表以零结尾。


.macro RestartableEntry
#if __LP64__
    .quad    LLookupStart$0 // .quad 定义一个 8 个字节（两 word）的类型（以 L 开头的标签叫本地标签，这些标签只能用于函数内部） 
#else
    .long    LLookupStart$0 // .long 定义一个 4 个字节的长整型
    .long    0 // 这个 0 不知道是干啥用的，难道这个是补位的吗，硬补 4 个字节 ？
#endif

    .short    LLookupEnd$0 - LLookupStart$0 // .short 定义一个 2 个字节的短整型
    .short    LLookupRecover$0 - LLookupStart$0
    .long    0 // 这个 0 不知道是干啥用的，难道这个是补位的吗，硬补 4 个字节 ？
.endmacro // RestartableEntry 宏定义结束，主要用于下面的声明（对应下面的 fill，一个 RestartableEntry 刚好 16 字节）

    .align 4 // 表示以 2^4 16 字节对齐
    .private_extern _objc_restartableRanges // 私有外联吗 ？
_objc_restartableRanges:
    
    // 定义 6 个私有的 RestartableEntry，看名字可以对应到我们日常使用到的函数
    // 这里可以理解为 C 语言中的函数声明，它们的实现都在下面，等下我们一行一行来解读
    
    RestartableEntry _cache_getImp 
    RestartableEntry _objc_msgSend
    RestartableEntry _objc_msgSendSuper
    RestartableEntry _objc_msgSendSuper2
    RestartableEntry _objc_msgLookup
    RestartableEntry _objc_msgLookupSuper2
    
    // .fill repeat, size, value 含义是反复拷贝 size 个字节，重复 repeat 次，
    // 其中 size 和 value 是可选的，默认值分别是 1 和 0 
    // 全部填充 0
    
    .fill    16, 1, 0

// 下面是 C 的宏定义，C 与 汇编混编
/* objc_super parameter to sendSuper */

#define RECEIVER         0
#define CLASS            __SIZEOF_POINTER__

/* Selected field offsets in class structure */
#define SUPERCLASS       __SIZEOF_POINTER__
#define CACHE            (2 * __SIZEOF_POINTER__)

/* Selected field offsets in method structure */
#define METHOD_NAME      0
#define METHOD_TYPES     __SIZEOF_POINTER__
#define METHOD_IMP       (2 * __SIZEOF_POINTER__)

#define BUCKET_SIZE      (2 * __SIZEOF_POINTER__)
```

#### `ENTRY/STATIC_ENTRY/STATIC_ENTRY`
```c++
/*
 * ENTRY functionName
 * STATIC_ENTRY functionName
 * END_ENTRY functionName
 */

// 定义一个汇编宏 ENTRY，表示在 text 段定义一个 32 字节对齐的 global 函数，"$0" 同时生产一个函数入口标签。

.macro ENTRY /* name */
    .text // .text 定义一个代码段，处理器开始执行代码的时候，代表后面是代码，这是 GCC 必须的。
    .align 5 // 2^5，32 个字节对齐
    .globl    $0 // .global 关键字用来让一个符号对链接器可见，可以供其他链接对象模块使用，
                 // 告诉汇编器后续跟的是一个全局可见的名字（可能是变量，也可以是函数名）
                 
                 // 00001:
                 // 00002: .text
                 // 00003: .global _start
                 // 00004:
                 // 00005: _start:
                 
                 // .global _start 和 _start: 配合，给代码开始地址定义一个全局标记 _start。
                 // _start 是一个函数的起始地址，也是编译、链接后程序的起始地址。由于程序是通过加载器来加载的，
                 // 必须要找到 _start 名字的的函数，因此 _start 必须定义成全局的，以便存在于编译后的全局符号表中，
                 // 供其他程序（如加载器）寻找到。 
                 
                 // .global _start 让 _start 符号成为可见的标示符，
                 // 这样链接器就知道跳转到程序中的什么地方并开始执行。
                 // Linux 寻找这个 _start 标签作为程序的默认进入点
                 
                 // .extern xxx 说明 xxx 为外部函数，调用的时候可以遍访所有文件找到该函数并且使用它
                 
// 在汇编和 C 混合编程中，在 GNU ARM 编译环境下，汇编程序中要使用 .global 伪操作声明汇编程序为全局的函数，
// 意即可被外部函数调用，同时 C 程序中要使用 extern 声明要被汇编调用的函数。

$0:
.endmacro

.macro STATIC_ENTRY /*name*/
    .text
    .align 5
    .private_extern $0
$0:
.endmacro

.macro END_ENTRY /* name */
LExit$0:
.endmacro
```

```c++
/*
 * objc-msg-arm64.s - ARM64 code to support objc messaging
 * objc-msg-arm64.s - 支持 objc 消息传递的 ARM64 代码
 */

```
#### `CacheLookup`
```c++
.macro CacheLookup
    //
    // Restart protocol:
    // 重启协议:
    //
    // As soon as we're past the LLookupStart$1 label we may have loaded an invalid cache pointer or mask.
    // 一旦超过 LLookupStart$1 标签，我们可能已经加载了无效的 缓存指针 或 掩码。
    // 
    // When task_restartable_ranges_synchronize() is called, (or when a signal hits us) before we're past LLookupEnd$1,
    // then our PC will be reset to LLookupRecover$1 which forcefully jumps to the cache-miss codepath which have the following.
    // 当我们在超过 LLookupEnd$1 之前（或当 信号 命中我们）调用 task_restartable_ranges_synchronize()，我们的 PC 将重置为 LLookupRecover$1，这将强制跳转到缓存未命中的代码路径，其中包含以下内容。
    // requirements:
    // 要求:
    //
    // GETIMP:
    // 获得 IMP:
    // The cache-miss is just returning NULL (setting x0 to 0) // 缓存未命中只是返回 NULL
    //
    // NORMAL and LOOKUP:
    // - x0 contains the receiver // x0 表示函数接收者
    // - x1 contains the selector // x1 表示 SEL
    // - x16 contains the isa // x16 是class 的 isa
    // - other registers are set as per calling conventions // 其它寄存器根据调用约定来设置
    //
LLookupStart$1:

    // p1 = SEL, p16 = isa p1 表示 SEL，p16 表示 isa
    // #define CACHE (2 * __SIZEOF_POINTER__) // 即 16
    // [x16, #CACHE] 则表示 x16(isa) + 16 的内存地址，即 cache 的地址。
    
    // (对应于 objc_class 的第一个成员变量是 isa_t isa，
    //  第二个成员变量是 Class superclass，
    //  第三个成员变量是 cache_t cache，根据他们的类型可以知道 isa 和 cache 刚好相差 16 个字节)
    
    // ldr r1, [r2, #4] 将内存地址为 r2+4 的数据读取到 r1 中
    // 将 cache 的内容读取到 p11 中 (这里 cache_t 第一个成员变量是 8 个字节，读取如果按字的话，一次只能 2 个字节)
    // 即把 buckets 指针或者 _maskAndBuckets 读取到 p11 中（它们都是 8 个字节），会根据不同的掩码类型来进行读取
    ldr    p11, [x16, #CACHE]                // p11 = mask|buckets

// 根据掩码类型来做不同的处理
#if CACHE_MASK_STORAGE == CACHE_MASK_STORAGE_HIGH_16
    // p11 & #0x0000ffffffffffff，表示直接舍弃 p11 高 16 位的内容，只要后 48 位的 buckets 
    // 把 p11 & 0x0000ffffffffffff 的结果保存在 p10 中，即 p10 就是 buckets
    and    p10, p11, #0x0000ffffffffffff    // p10 = buckets
    
    // LSR 逻辑右移(Logic Shift Right)
    // p11, LSR #48 表示 _maskAndBuckets 右移 48 位取得 _mask
    // and 按位与，与 C 的 "&" 功能相同
    // p1 是 SEL，然后和上面👆取得的 _mask 做与操作即取得 SEL 的哈希值并保存在 p12 中
    // 在这里的时候 p12 和 x12 都是 SEL 的哈希值
    and    p12, p1, p11, LSR #48        // x12 = _cmd & mask (在函数内部 _cmd 即表示函数的 SEL)
#elif CACHE_MASK_STORAGE == CACHE_MASK_STORAGE_LOW_4
    // 掩码在低 4 位的情况
    and    p10, p11, #~0xf            // p10 = buckets
    and    p11, p11, #0xf            // p11 = maskShift
    mov    p12, #0xffff
    lsr    p11, p12, p11                // p11 = mask = 0xffff >> p11
    // 同样将 SEL 的哈希值保存在 p12 中
    and    p12, p1, p11                // x12 = _cmd & mask
#else

// ARM64 不支持的缓存掩码存储。
#error Unsupported cache mask storage for ARM64.

#endif
    // 在 Project Headers/arm64-asm.h 中可以看到 PTRSHIFT 的宏定义
    // #if __arm64__
    // #if __LP64__ // 64 位系统架构
    // #define PTRSHIFT 3  // 1<<PTRSHIFT == PTRSIZE // 0b1000 表示一个指针 8 个字节
    // // "p" registers are pointer-sized
    // // true arm64
    // #else
    // // arm64_32 // 32 位系统架构
    // #define PTRSHIFT 2  // 1<<PTRSHIFT == PTRSIZE // 0b100 表示一个指针 4 个字节
    // // "p" registers are pointer-sized
    // // arm64_32
    // #endif

    // LSL 逻辑左移(Logic Shift Left)
    // p10 是 buckets
    // p12 是 (_cmd & mask) // 哈希值
    // 即 p12 先做逻辑左移运算(这里的逻辑左移是表示在对哈希值做乘法扩大为 8 倍)，然后和 p10 相加，并把最后结果存放在 p12 中
    // p12 = buckets + ((_cmd & mask) << (1+PTRSHIFT))
    
    // (把 SEL 的哈希值左移 4 位意思是哈希值乘以 8，这个 8 指的的是一个指针占了 8 个字节的意思
    //  即算出 SEL 对应的 bucket_t 指针的位置与 buckets 的起始地址的距离，
    //  这里的距离单位是按字节计算的，所以要乘以 8)
    // 即此时 p12 指向的是 SEL 哈希值对应的在 buckets 散列数组下标下的 bucket_t 指针的起始地址
    add    p12, p10, p12, LSL #(1+PTRSHIFT)
                     // p12 = buckets + ((_cmd & mask) << (1+PTRSHIFT))

    // ldr 从存储器中加载 (Load) 字到一个寄存器 (Register) 中
    // str 把一个寄存器按字存储 (Store) 到存储器中
    // 示例: 
    // ldr r1, =0x123456789 大范围的地址读取指令: r1 = 0x123456789
    
    // ldr r1, [r2, #4] 内存访问指令（当 ldr 后面没有 = 号时为内存读取指令）
    // 将内存地址为 r2+4 的数据读取到 r1 中，相当于 C 语言中的 * 操作
    
    // 这种 [xxx] 与 #x 分离的情况比较特殊，要注意（它这个内容读取完毕以后再增加 r2 的距离，改变 r2 的指向）
    // ldr r1, [r2], #4 将内存地址为 r2 的数据读取到 r1 中，再将地址加 4，r2 = r2 + 4
    
    // str r1, [r2, #4] 存储指令: 将 r1 的值存入地址为 r2 + 4 的内存中
    
    // 这种 [xxx] 与 #x 分离的情况比较特殊，要注意（它这个内容存储完毕以后再增加 r2 地址值，改变 r2 的指向）
    // str r1, [r2], #4 将 r1 的值存入地址为 r2 的内存中，再将地址加 4，r2 = r2 + 4
    
    // ldp/stp 是 ldr/str 的衍生，可以同时读/写两个寄存器，ldr/str 只能读写一个
    // 示例: ldp x1, x0, [sp, #0x10] 将 sp 偏移 16 个字节的值取出来，放入 x1 和 x0
    
    // 这里 x12 就是 p12 吗 ？，表示以 SEL 哈希值为数组下标，在 buckets 散列数组中对应的 bucket_t 指针
    // 目前已知的变量 p10 是 buckets p12 是 SEL 在 buckets 数组中对应的 bucket_t 指针，那么这个 x12 到底是什么？
    // 而且在不同的平台下，bucket_t 的 _sel 和 _imp 的顺序是相反的，在 __arm64__ 下是 _imp 在前 _sel 在后，其他平台下则是相反的
    
    ldp    p17, p9, [x12]        // {imp, sel} = *bucket 那么 p17 和 p9  x12 此时都是同一个值了
    
    // cmp 比较指令
    // p1 = SEL (p1 的值自开始就没有被改变过)
    // 判断以 SEL 哈希值找到的 bucket_t 的 _sel 是否就是 SEL，这里可能会因为哈希冲突而导致与 SEL 不一样，
    // 此时需要根据不同的平台执行向前或者向后的线性探测找到对应的 bucket_t.
    
    // 这里还有一个点，p1 是 8 个字节，p9 应该至少是从 bucket_t 中取出来的 _sel 
1:  cmp    p9, p1            // if (bucket->sel != _cmd) // 这里的 1: 好像表示是段还是区来着，此时表示不同，
    b.ne    2f            //     scan more
    CacheHit $0            // call or return imp
    
2:  // not hit: p12 = not-hit bucket 未命中
    // CheckMiss normal -> 判断 p9 是否为 0 // 空 bucket_t 的初始值会是 0，而那个 end 占位的 bucket_t 的 _sel 是 1
    // 即判断查找到的是不是空，如果为空，即表示当前方法缓存列表里面没有缓存 sel 对应的方法，此时需要去类的方法列表里面去查找方法
    // 如果不是空，则表示此时发生了哈希冲突，bucket_t 存在别处，继续向前或者向后查找
    CheckMiss $0            // miss if bucket->sel == 0
    // 判断是否已经是第一个了，如果是首个就去类的方法列表查找
    cmp    p12, p10        // wrap if bucket == buckets
    b.eq    3f // 跳转到下面的 3f
    
    // 还可以继续冲突的向前查找
    // #define BUCKET_SIZE (2 * __SIZEOF_POINTER__) 16 个字节，正是 bucket_t 的宽度
    // 往前查找
    ldp    p17, p9, [x12, #-BUCKET_SIZE]!    // {imp, sel} = *--bucket
    // 循环
    b    1b            // loop

3:    // wrap: p12 = first bucket, w11 = mask
#if CACHE_MASK_STORAGE == CACHE_MASK_STORAGE_HIGH_16
    // p11 是 buckets 或者 _maskAndBuckets
    // p11 逻辑右移 44（这里包含了两步，首先 p11 右移 48 位得到 mask，然后再左移 4 位，
    // 表示扩大 8 倍（可代表指针的字节宽度），即整体 p11 右移了 44 位，这个值可以表示 buckets 指针需要需要移动的总距离）
    
    add    p12, p12, p11, LSR #(48 - (1+PTRSHIFT))
                    // 那么此时 p12 指向的是谁呢
                    // p12 = buckets + (mask << 1+PTRSHIFT)
                    
#elif CACHE_MASK_STORAGE == CACHE_MASK_STORAGE_LOW_4
    // 当低 4 位是掩码时，基本完全一样的操作
    add    p12, p12, p11, LSL #(1+PTRSHIFT)
                    // p12 = buckets + (mask << 1+PTRSHIFT)
#else

// ARM64不支持的缓存掩码存储。
#error Unsupported cache mask storage for ARM64.

#endif

    // Clone scanning loop to miss instead of hang when cache is corrupt.
    // 当缓存损坏时，克隆扫描循环将丢失而不是挂起。
    // The slow path may detect any corruption and halt later.
    // slow path 可能会检测到任何损坏并在稍后停止。
    
    // x12 的内容读取到 p17、p9 中
    ldp    p17, p9, [x12]        // {imp, sel} = *bucket
    // 比较
1:    cmp    p9, p1            // if (bucket->sel != _cmd)
    b.ne    2f            //     scan more
    CacheHit $0            // call or return imp
    
2:    // not hit: p12 = not-hit bucket
    CheckMiss $0            // miss if bucket->sel == 0
    cmp    p12, p10        // wrap if bucket == buckets
    b.eq    3f
    ldp    p17, p9, [x12, #-BUCKET_SIZE]!    // {imp, sel} = *--bucket
    b    1b            // loop

LLookupEnd$1:
LLookupRecover$1:
3:    // double wrap
    JumpMiss $0

.endmacro
```

## 参考链接
**参考链接:🔗**
+ [方法查找流程 objc_msg_arm64.s](https://www.jianshu.com/p/a8668b81c5d6)
+ [OC 底层探索 09、objc_msgSend 流程 1-缓存查找](https://www.cnblogs.com/zhangzhang-y/p/13704597.html)
+ [汇编指令解读](https://blog.csdn.net/peeno/article/details/53068412)
+ [objc-msg-arm64源码深入分析](https://www.jianshu.com/p/835ae53372ba)
+ [汇编语言学习笔记](https://chipengliu.github.io/2019/04/05/asm-note/)
+ [iOS汇编教程：理解ARM](https://www.jianshu.com/p/544464a5e630)
+ [汇编跳转指令B、BL、BX、BLX 和 BXJ的区别](https://blog.csdn.net/bytxl/article/details/49883103)
+ [iOS开发同学的arm64汇编入门](https://blog.cnbluebox.com/blog/2017/07/24/arm64-start/)
+ [C语言栈区的讲解(基于ARM)以及ARM sp,fp寄存器的作用](https://blog.csdn.net/Lcloud671/article/details/78232401)
+ [.align 5的是多少字节对齐](https://blog.csdn.net/yunying_si/article/details/9736173?ops_request_misc=%257B%2522request%255Fid%2522%253A%2522160185721219724839257560%2522%252C%2522scm%2522%253A%252220140713.130102334..%2522%257D&request_id=160185721219724839257560&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~first_rank_v2~rank_v28-3-9736173.pc_first_rank_v2_rank_v28&utm_term=.align+&spm=1018.2118.3001.4187)
+ [解读objc_msgSend](https://www.jianshu.com/p/75a4737741fd)
+ [ARM汇编指令](https://blog.csdn.net/qq_27522735/article/details/75043870)

**ARM 的栈是自减栈，栈是向下生长的，也就是栈底处于高地址处，栈顶处于低地址处，所以栈区一般都是放在内存的顶端。**

