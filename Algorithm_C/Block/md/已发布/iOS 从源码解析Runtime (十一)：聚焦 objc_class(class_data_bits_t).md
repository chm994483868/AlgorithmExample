# iOS 从源码解析Runtime (十一)：聚焦 objc_class(class_data_bits_t)

> `objc_class` 的 `cache_t cache` 成员变量终于完完整整分析完了，接下来我们继续分析 `class_data_bits_t data`。

## `class_data_bits_t`
&emsp;`class_data_bits_t data` 作为 `objc_class` 的第三个成员变量也是最后一个成员变量，它的作用正如它的名字 `data`，而它也正是存储管理了类相关的所有数据，例如我们上篇一直讲的当缓存未命中时就会去类的方法列表中继续查找，而这个方法列表正保存在 `data` 中，且方法列表不仅包含我们直接在类定义中编写的实例函数还包括我们在分类中为类添加的实例方法它们也会被追加到 `data` 中，等等 `data` 包含了太多数据和功能。下面我们一行一行来看它为类处理了哪些数据管理了类的哪些功能吧⛽️⛽️！
```c++
struct objc_class : objc_object {
    // Class ISA;
    Class superclass;
    cache_t cache;             // formerly cache pointer and vtable
    
    // 注释点明了核心 class_rw_t * 加重载 rr/alloc 标记
    class_data_bits_t bits;    // class_rw_t * plus custom rr/alloc flags
    ...
};
```
### `class_data_bits_t 成员变量`
&emsp;在 `struct class_data_bits_t` 定义中声明了 `objc_class` 为其友元类，`objc_class` 可以完全访问和调用 `class_data_bits_t` 的私有成员变量和私有方法。然后是仅有的一个成员变量 `uintptr_t bits`，这里之所以把它命名为 `bits` 也是有其意义的，它通过掩码的形式保存 `class_rw_t` 指针和是否是 `swift` 类等一些标志位。   
```c++
struct class_data_bits_t {
    friend objc_class;

    // Values are the FAST_ flags above.
    uintptr_t bits;
    ...
};
```
### `class_data_bits_t private 部分`
&emsp;代码中的注释真的超详细，这里就不展开说了。主要完成对 `bits` 的 `64` 位中指定位的设置和读取操作。
```c++
private:
    // 尾部的 const 表示该方法内部不会修改 class_data_bits_t 的内部数据
    // 这里返回值是一个 bool 来，通过与操作来取出 bits 的固定位的值来进行判断
    bool getBit(uintptr_t bit) const
    {
        // 内部实现只有一个与操作，主要根据入参 bit(掩码) 来取得一些标识位
        // 如: 
        
        // class is a Swift class from the pre-stable Swift ABI
        // #define FAST_IS_SWIFT_LEGACY    (1UL<<0)
        // 使用 bit 的第一位来进行判断
        
        // class is a Swift class from the stable Swift ABI
        // bit 的 第二位 判断类是否是稳定的 Swift ABI 的 Swift 类
        // #define FAST_IS_SWIFT_STABLE (1UL<<1)
        
        return bits & bit;
    }

    // Atomically set the bits in `set` and clear the bits in `clear`.
    // 以原子方式设置 `set` 中的位，并清除 `clear` 中的位。
    // set and clear must not overlap.
    // 设置和清除不得重叠。
    
    // 以原子方式设置 bits 中的指定的标识位，然后再进行指定的标识位清理
    // （设置操作即是把指定的位置 1）
    // （清理操作即是把指定的位置 0）
    void setAndClearBits(uintptr_t set, uintptr_t clear)
    {
        // 断言，如果 set 和 clear 都是 1 的则没有执行的必要，直接执行断言
        ASSERT((set & clear) == 0);
        
        // 临时变量保存旧值
        uintptr_t oldBits;
        // 临时变量保存新值
        uintptr_t newBits;
        
        // do while 循环在 retain 和 release 里面我们已经见过了，
        // 主要是为了保证在多线程下 bits 位一定被正确设置了。
        
        // 循环条件：!StoreReleaseExclusive(&bits, oldBits, newBits)
        
        // 当 &bits 与 oldBits 相同时，把 newBits 复制到 &bits，
        // 并返回 true，由于前面的 ! 取反，此时会结束 do while 循环
        
        // 当 &bits 与 oldBits 不同时，把 oldBits 复制到 &bits，
        // 并返回 false，由于前面的 ! 取反，此时会继续 do while 循环
        
        do {
            // 以原子方式读取出 bits
            oldBits = LoadExclusive(&bits);
            
            // 这里根据运算优先级，其实也不是，是直接的小括号的最高优先级
            // 先拿 oldBits 和 set 做或操作保证所有 64 位的 1 都保留下来
            // 然后 ~clear 的操作已经把需要 clear 的位置为 0，然后无关的其他位都是 1
            // 最后和 ~clear 做与操作并把结果赋值给 newBits，
            // 此时的 newBits 和原始的 bits 比的话，正是把入参 set 位置为 1 把入参 clear 位置为 0 
            newBits = (oldBits | set) & ~clear;
            
        // while 循环正保证了 newBits 能正确的设置到的 bits 中
        } while (!StoreReleaseExclusive(&bits, oldBits, newBits)); 
    }

    // 以原子方式设置 bits 中指定的标识位
    void setBits(uintptr_t set) {
        __c11_atomic_fetch_or((_Atomic(uintptr_t) *)&bits, set, __ATOMIC_RELAXED);
    }
    
    // 以原子方式清理 bits 中指定的标识位
    void clearBits(uintptr_t clear) {
        __c11_atomic_fetch_and((_Atomic(uintptr_t) *)&bits, ~clear, __ATOMIC_RELAXED);
    }
```
### `class_data_bits_t public 部分`
```c++
public:
    
    // 最最最最重要的函数来啦，从 bits 中读出 class_rw_t 指针
    //（class_rw_t 的具体作用下面会详细讲解）
    
    // __LP64__: #define FAST_DATA_MASK 0x00007ffffffffff8UL
    // !__LP64__: #define FAST_DATA_MASK 0xfffffffcUL
    
    class_rw_t* data() const {
        // 与操作取出 class_rw_t 指针
        return (class_rw_t *)(bits & FAST_DATA_MASK);
    }
    
    // 上面是读取 class_rw_t 指针，那这里就是设置啦
    
    void setData(class_rw_t *newData)
    {   
        // 断言点明了应该何时调用 setData 来设置 class_rw_t 指针
        // 虽然这里有个 newData->flags 我们还没有遇到，但是不影响我们分析当前情况
        
        // class has started realizing but not yet completed it
        // RW_REALIZING 标识类已经开始实现但是还没有完成，即类正在实现过程中
        
        //（标识在第十九位）
        // #define RW_REALIZING (1<<19) 
        
        // class is unresolved future class
        // 类是尚未解决的未来类
        
        //（标识在第三十位）
        // #define RW_FUTURE (1<<30)
        
        // data() 不存在或者类处于 RW_REALIZING 或是 RW_FUTURE，否则执行断言
        ASSERT(!data()  ||  (newData->flags & (RW_REALIZING | RW_FUTURE)));
        
        // Set during realization or construction only. No locking needed.
        // setData 仅在 实现或构造期间 设置。不需要加锁。
        // Use a store-release fence because there may be
        // concurrent readers of data and data's contents.
        // 使用了一个 store-release fence 因为可能同时存在 数据读取器 和 数据内容读取器。
        
        // #define FAST_DATA_MASK 0x00007ffffffffff8UL
        
        // 首先是 bits 和 ~FAST_DATA_MASK 做一个与操作，
        // 即把 bits 中与 class_rw_t 指针相关的掩码位全部置为 0 同时保存其它位的值
        // 然后和 newData 做一个或操作，即把 newData 的地址有效位放到 bits 的对应的掩码位中
        // bits 的其它位则保持原值
        
        // 把结果赋值给 newBits
        uintptr_t newBits = (bits & ~FAST_DATA_MASK) | (uintptr_t)newData;
        
        // 线程围栏
        atomic_thread_fence(memory_order_release);
        
        bits = newBits;
    }

    // Get the class's ro data, even in the presence of concurrent realization.
    // 即使存在并发实现，也获取类的 ro 数据。
    
    // fixme this isn't really safe without a compiler barrier at least and probably a memory barrier when realizeClass changes the data field.
    // fixme 这至少在没有编译器障碍的情况下并不是真正安全的，而当实现Class更改数据字段时可能没有内存障碍
    
    const class_ro_t *safe_ro() {
        class_rw_t *maybe_rw = data();h9h
        if (maybe_rw->flags & RW_REALIZED) {
            // maybe_rw is rw
            return maybe_rw->ro();
        } else {
            // maybe_rw is actually ro
            return (class_ro_t *)maybe_rw;
        }
    }

    void setClassArrayIndex(unsigned Idx) {
#if SUPPORT_INDEXED_ISA
        // 0 is unused as then we can rely on zero-initialisation from calloc.
        ASSERT(Idx > 0);
        data()->index = Idx;
#endif
    }

    unsigned classArrayIndex() {
#if SUPPORT_INDEXED_ISA
        return data()->index;
#else
        return 0;
#endif
    }

    bool isAnySwift() {
        return isSwiftStable() || isSwiftLegacy();
    }

    bool isSwiftStable() {
        return getBit(FAST_IS_SWIFT_STABLE);
    }
    void setIsSwiftStable() {
        setAndClearBits(FAST_IS_SWIFT_STABLE, FAST_IS_SWIFT_LEGACY);
    }

    bool isSwiftLegacy() {
        return getBit(FAST_IS_SWIFT_LEGACY);
    }
    void setIsSwiftLegacy() {
        setAndClearBits(FAST_IS_SWIFT_LEGACY, FAST_IS_SWIFT_STABLE);
    }

    // fixme remove this once the Swift runtime uses the stable bits
    bool isSwiftStable_ButAllowLegacyForNow() {
        return isAnySwift();
    }

    _objc_swiftMetadataInitializer swiftMetadataInitializer() {
        // This function is called on un-realized classes without
        // holding any locks.
        // Beware of races with other realizers.
        return safe_ro()->swiftMetadataInitializer();
    }
```

## 参考链接
**参考链接:🔗**
+ []()
