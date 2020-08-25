# iOS_SideTables和SideTable

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
1. `spinlock_t slock;`: 自旋锁

**参考链接:🔗**
[【C++】C++11可变参数模板（函数模板、类模板）](https://blog.csdn.net/qq_38410730/article/details/105247065?utm_medium=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-2.channel_param&depth_1-utm_source=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-2.channel_param)
[C++11新特性之 std::forward(完美转发)](https://blog.csdn.net/wangshubo1989/article/details/50485951?utm_medium=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-3.channel_param&depth_1-utm_source=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-3.channel_param)
