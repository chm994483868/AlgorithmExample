#  iOS 从源码解析Runtime (九)：聚焦 objc_class(cache_t 及方法缓存实现相关内容篇(2))

> 上篇分析了 `bucket_t` 和 `cache_t` 的几乎全部内容，最后由于篇幅限制剩两个函数留在本篇来分析，然后准备接着分析 `objc-cache.mm` 文件中与 `objc-cache.h` 文件对应的几个核心函数，正是由它们构成了完整的方法缓存实现，那么一起 ⛽️⛽️ 吧！


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



## 参考链接
**参考链接:🔗**
+ [方法查找流程 objc_msg_arm64.s](https://www.jianshu.com/p/a8668b81c5d6)
+ [OC 底层探索 09、objc_msgSend 流程 1-缓存查找](https://www.cnblogs.com/zhangzhang-y/p/13704597.html)
+ [汇编指令解读](https://blog.csdn.net/peeno/article/details/53068412)
