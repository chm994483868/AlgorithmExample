# iOS_SideTables 和 SideTable

## SideTables
`SideTables` 可以理解为一个 `key` 是对象指针(`void *`)，`value` 是`SideTable` 的静态全局的 `hash` 数组，里面存储了 `SideTable` 类型的数据，其长度在 `TARGET_OS_IPHONE && !TARGET_OS_SIMULATOR` 的情况下是 8，其他所有情况下是 64。

`SideTables` 可以通过全局的静态函数获取:
```c++
static StripedMap<SideTable>& SideTables() {
    return SideTablesMap.get();
}
```
看到 `SideTables()` 下面定义了多个与 `lock` 相关的函数，内部实现其实都是调用的 `class StripedMap` 的函数，而 `class StripedMap` 又恰是其模版抽象类型 `T` 所支持的函数接口，对应 `SideTables` 的 `T` 类型是 `SideTable`，下面分析 `SideTable` 时，再一并分析看 `SideTable` 是怎么实现的。
```c++
void SideTableLockAll() {
    SideTables().lockAll();
}

void SideTableUnlockAll() {
    SideTables().unlockAll();
}

void SideTableForceResetAll() {
    SideTables().forceResetAll();
}

void SideTableDefineLockOrder() {
    SideTables().defineLockOrder();
}

void SideTableLocksPrecedeLock(const void *newlock) {
    SideTables().precedeLock(newlock);
}

void SideTableLocksSucceedLock(const void *oldlock) {
    SideTables().succeedLock(oldlock);
}

void SideTableLocksPrecedeLocks(StripedMap<spinlock_t>& newlocks) {
    int i = 0;
    const void *newlock;
    while ((newlock = newlocks.getLock(i++))) {
        SideTables().precedeLock(newlock);
    }
}

void SideTableLocksSucceedLocks(StripedMap<spinlock_t>& oldlocks) {
    int i = 0;
    const void *oldlock;
    while ((oldlock = oldlocks.getLock(i++))) {
        SideTables().succeedLock(oldlock);
    }
}

```
根据函数返回值类型可以看到 `SideTabls` 类型为模版类型 `StripedMap`。
接着看下 `SideTablesMap.get()`，`SideTableMap` 是一个类型为 `objc::ExplicitInit<StripedMap<SideTable>>` 的静态全局变量，`SideTablesMap` 定义：
```c++
static objc::ExplicitInit<StripedMap<SideTable>> SideTablesMap;
```
那接下来我们详细分析一下 `ExplicitInit` 类型。
## `ExplicitInit`
定义位于`Project Headers/DenseMapExtras.h` P37：

`ExplicitInit` 和 `LazyInit` 好像是对应的一对，`明确初始化` 和 `懒惰初始化`:
```c++
// 命名空间 objc
namespace objc {

// We cannot use a C++ static initializer to initialize certain globals because libc calls us before our C++ initializers run. 
// 我们不能使用 C++ 静态初始化程序来初始化某些全局变量，因为 libc 在 C++ 初始化程序运行之前会调用我们。

// We also don't want a global pointer to some globals because of the extra indirection.
// 由于额外的间接性，我们也不需要全局指针指向某些全局变量。

// ExplicitInit / LazyInit wrap doing it the hard way.
// ExplicitInit / LazyInit Wrap 很难做到

// ExplicitInit 抽象数据类型是 Type 的模版类
template <typename Type>
class ExplicitInit {
    // 内存对齐原则同 Type
    // 长度为 sizeof(Type) 的 uint8_t 数组
    // typedef unsigned char uint8_t; 无符号 char 类型
    alignas(Type) uint8_t _storage[sizeof(Type)];

public:
    // c++11 新增加了变长模板，Ts 是 T 的复数形式
    // 如果我们要避免这种转换呢？ 
    // 我们需要一种方法能按照参数原来的类型转发到另一个函数中，这才完美，我们称之为完美转发。
    // std::forward就可以保存参数的左值或右值特性。
    template <typename... Ts>
    void init(Ts &&... Args) {
        new (_storage) Type(std::forward<Ts>(Args)...);
    }

    Type &get() {
        return *reinterpret_cast<Type *>(_storage);
    }
};

template <typename Type>
class LazyInit {
    alignas(Type) uint8_t _storage[sizeof(Type)];
    bool _didInit;

public:
    template <typename... Ts>
    Type *get(bool allowCreate, Ts &&... Args) {
        if (!_didInit) {
            if (!allowCreate) {
                return nullptr;
            }
            new (_storage) Type(std::forward<Ts>(Args)...);
            _didInit = true;
        }
        return reinterpret_cast<Type *>(_storage);
    }
};

// Convenience class for Dense Maps & Sets
template <typename Key, typename Value>
class ExplicitInitDenseMap : public ExplicitInit<DenseMap<Key, Value>> { };

template <typename Key, typename Value>
class LazyInitDenseMap : public LazyInit<DenseMap<Key, Value>> { };

template <typename Value>
class ExplicitInitDenseSet : public ExplicitInit<DenseSet<Value>> { };

template <typename Value>
class LazyInitDenseSet : public LazyInit<DenseSet<Value>> { };

} // namespace objc
```
`ExplicitInit` 和 `LazyInit`  真的被这两绕晕了 😖😖，不理解也没事，完全不影响接下来的 `SideTable`。

## `SideTable`
首先看一下位于 `Source/NSObject.mm` P108 的 `struct SideTable` 的定义:
```c++
struct SideTable {
    spinlock_t slock; // 自旋锁，防止多线程访问冲突
    RefcountMap refcnts; // 对象的引用计数 map
    weak_table_t weak_table; // 对象的弱引用 map

    SideTable() {
        // 把从 &weak_table 位置开始的长度为 sizeof(weak_table) 的内存置为 0
        memset(&weak_table, 0, sizeof(weak_table));
    }

    ~SideTable() {
        // 析构函数
        // 看到 SidetTable 是不能析构的，如果进行析构则会直接 creash
        _objc_fatal("Do not delete SideTable.");
    }
    
    // 看到了三个函数接口
    // 这里三个函数接口正对应了 StripedMap 中模版抽象类型 T 的接口要求
    // 三个函数的实现也是直接调用 spinlock_t slock 成员变量来执行
    void lock() { slock.lock(); }
    void unlock() { slock.unlock(); }
    void forceReset() { slock.forceReset(); }

    // Address-ordered lock discipline for a pair of side tables.
    
    // 按锁的顺序对两个 SideTable 参数里的 slock 上锁
    // HaveOld 和 HaveNew 分别表示 lock1 和 lock2 是否存在
    // 对应于 __weak 变量是否指向有旧值和目前要指向的新值
    // lock1 代表旧值对象所处的 SideTable 
    // lock2 代表新值对象所处的 SideTable
    template<HaveOld, HaveNew>
    static void lockTwo(SideTable *lock1, SideTable *lock2);
    
    // 同上，对 slock 解锁
    template<HaveOld, HaveNew>
    static void unlockTwo(SideTable *lock1, SideTable *lock2);
};
```
`struct SideTable` 定义很清晰，首先是 3 个成员变量:

1. `spinlock_t slock;`: 自旋锁，用于 `SideTable` 的加锁和解锁。
  此锁正是重点来解决弱引用机制的线程安全问题的，看前面的两大块 `weak_table_t` 和 `weak_entry_t` 的时候，看到所有操作中都不是线程安全的，它们的操作完全没有提及锁的事情，其实是把保证它们线程安全的任务交给了 `SideTable`。下面可以看到 `SideTable` 提供的方法都与锁有关。

2. `RefcountMap refcnts;`: 以 `DisguisedPtr<objc_object>` 为 `key` 的 `hash` 表，用来存储 `OC` 对象的引用计数（仅在未开启 `isa` 优化或者 `isa` 优化情况下 `isa_t` 的引用计数溢出时才会用到，这里就牵涉到 `isa_t`里的 `uintptr_t has_sidetable_rc` 和 `uintptr_t extra_rc` 两个字段，以前看的 `isa` 的结构这里终于用到了，还有这时候终于知道 `rc` 其实是 `refcount`(引用计数) 的缩写。😄）
3. `weak_table_t weak_table;` 存储对象弱引用的指针的 `hash` 表，是 `OC` `weak` 功能实现的核心数据结构。

下面是构造函数和析构函数：
构造函数只做了一件事，把 `weak_table` 的数据空间置为 `0`：
```c++
// 把从 &weak_table 位置开始的长度为 sizeof(weak_table) 的内存置为 0
memset(&weak_table, 0, sizeof(weak_table));
```
析构函数也是只做了一件事，就是把你的程序直接给停止运行，明确指出 `SideTable` 是不能被析构的。`_objc_fatal` 会调用 `exit` 或者 `abort`。
再下面是锁的操作，同时接口也符合上面提到的 `StripedMap` 中关于模版抽象类型 `T` 类型的 `value` 的接口要求。

## `spinlock_t`
`spinlock_t` 的最终定义是实际上是一个 `uint32_t` 类型 `非公平的自旋锁`，（目前底层实现已由互斥锁(`os_unfair_lock`)所替换）。所谓非公平是指，就是说获得锁的顺序和申请锁的顺序无关，也就是说，第一个申请锁的线程有可能会是最后一个获得该锁，或者是刚获得锁线程会再次立刻获得该锁，造成忙等（`busy-wait`）。
同时，`os_unfair_lock` 在 `_os_unfair_lock_opaque` 也记录了获取它的线程信息，只有获得该锁的线程才能够解开这把锁。
```c++
OS_UNFAIR_LOCK_AVAILABILITY
typedef struct os_unfair_lock_s {
    uint32_t _os_unfair_lock_opaque;
} os_unfair_lock, *os_unfair_lock_t;
```
`os_unfair_lock` 的实现，苹果并未公开，大体上应该是操作 `_os_unfair_lock_opaque` 这个 `uint32_t` 的值，当大于 0 时，锁可用，当等于或小于 0 时，表示锁已经被其他线程获取且还没有解锁，当前线程再获取这把锁，就要被等待（或者直接阻塞，知道能获取到锁）。

## `RefcountMap`
`RefcountMap refcnts;` 用来存储对象的引用计数。首先看一下它的类型定义:
```c++
// RefcountMap disguises its pointers because we don't want the table to act as a root for `leaks`.
// 同样是使用 DisguisedPtr 把对象的地址隐藏起来，逃过 leaks 等工具的检测
typedef objc::DenseMap<DisguisedPtr<objc_object>,size_t,RefcountMapValuePurgeable> RefcountMap;
```
它实质上是一个 `key` 是 `DisguisedPtr<objc_object>`，`value` 是 `size_t` 的 `hash` 表，而 `value` 就是对象的引用计数。（《Objective-C 高级编程xxx》那本书里面的苹果实现引用计数的逻辑可以在这里得到解答！⛽️）`RefcountMapValuePurgeable` 表示是否在 `value = 0` 的时候自动释放掉响应的 `hash` 节点，默认是 `true`。下面是它的定义:
```c++
struct RefcountMapValuePurgeable {
    static inline bool isPurgeable(size_t x) {
        return x == 0;
    }
};
```
`RefcountMap` 的实际代码实现是一个模版类。
定义位于 `Project Headers/llvm-DenseMap.h` P750，代码如下：
```c++
template <typename KeyT, typename ValueT,
          typename ValueInfoT = DenseMapValueInfo<ValueT>,
          typename KeyInfoT = DenseMapInfo<KeyT>,
          typename BucketT = detail::DenseMapPair<KeyT, ValueT>>
class DenseMap : public DenseMapBase<DenseMap<KeyT, ValueT, ValueInfoT, KeyInfoT, BucketT>,
                                     KeyT, ValueT, ValueInfoT, KeyInfoT, BucketT> {
  friend class DenseMapBase<DenseMap, KeyT, ValueT, ValueInfoT, KeyInfoT, BucketT>;

  // Lift some types from the dependent base class into this class for
  // simplicity of referring to them.
  using BaseT = DenseMapBase<DenseMap, KeyT, ValueT, ValueInfoT, KeyInfoT, BucketT>;

  BucketT *Buckets;
  unsigned NumEntries;
  unsigned NumTombstones;
  unsigned NumBuckets;
  ...
};
```
关于 `DenseMap` 和相关的模版定义，实在是太长啦，等后面再看。😭

**参考链接:🔗**
[【C++】C++11可变参数模板（函数模板、类模板）](https://blog.csdn.net/qq_38410730/article/details/105247065?utm_medium=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-2.channel_param&depth_1-utm_source=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-2.channel_param)
[C++11新特性之 std::forward(完美转发)](https://blog.csdn.net/wangshubo1989/article/details/50485951?utm_medium=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-3.channel_param&depth_1-utm_source=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-3.channel_param)
