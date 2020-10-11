# iOS 从源码解析Runtime (十三)：聚焦 objc_class(objc_class函数相关内容篇)

> 前面已经忘记看了多少天了，终于把 `objc_object` 和 `objc_class` 的相关的数据结构都看完了，现在只剩下 `objc_class` 中定义的函数没有分析，那么就由本篇开始分析吧！⛽️⛽️

## `objc_class 函数`

### `class_rw_t *data() const`
&emsp;`data` 函数是直接调用的 `class_data_bits_t bits` 的 `data` 函数，内部实现的话也很简单，通过掩码 `#define FAST_DATA_MASK 0x00007ffffffffff8UL`（二进制第 `3`-`46` 位是 `1`，其他位都是 `0`） 从 `class_data_bits_t bits` 的 `uintptr_t bits`（`uintptr_t bits` 是 `struct class_data_bits_t` 仅有的一个成员变量）中取得 `class_rw_t` 指针，如果类处于未实现完成的状态的话返回的则是 `class_ro_t` 指针。
```c++
class_rw_t *data() const {
    return bits.data();
}
```
### `void setData(class_rw_t *newData)`
&emsp;同样 `setData` 函数调用的也是 `class_data_bits_t bits` 的 `setData` 函数，在 `struct class_data_bits_t` 的 `setData` 函数中， `uintptr_t bits` 首先和 `~FAST_DATA_MASK` 做与操作把掩码位置为 `0`，其它位保持不变，然后再和 `newData` 做或操作，把 `newData` 中值为 `1` 的位也设置到 `uintptr_t bits` 中。
```c++
void setData(class_rw_t *newData) {
    bits.setData(newData);
}
```
### `void setInfo(uint32_t set)`
&emsp;`setInfo` 函数调用的是 `class_rw_t` 的 `setFlags` 函数，通过原子的或操作把 `set` 中值为 `1` 的位也设置到 `class_rw_t` 的 `uint32_t flags` 中。（`flags` 和 `set` 都是 `uint32_t` 类型，都是 `32` 位。）
```c++
void setInfo(uint32_t set) {
    ASSERT(isFuture()  ||  isRealized());
    data()->setFlags(set);
}
```
### `void clearInfo(uint32_t clear)`
&emsp;`clearInfo` 函数调用的是 `class_rw_t` 的 `clearFlags` 函数，通过原子的与操作把 `~clear` 中值为 `0` 的位也设置到 `class_rw_t` 的 `uint32_t flags` 中。（`flags` 和 `clear` 都是 `uint32_t`  类型，都是 `32` 位。）
```c++
void clearInfo(uint32_t clear) {
    ASSERT(isFuture()  ||  isRealized());
    data()->clearFlags(clear);
}
```
### `void changeInfo(uint32_t set, uint32_t clear)`
&emsp;`changeInfo` 函数调用的同样的是 `class_rw_t` 的 `changeFlags` 函数，先取得 `class_rw_t` 的 `uint32_t flags` 赋值到一个临时变量 `uint32_t oldf` 中，然后 `oldf` 和 `uint32_t set` 做一个或操作，把 `set` 值为 `1` 的位也都设置到 `oldf` 中，然后再和 `~clear` 做与操作，把 `~clear` 中是 `0` 的位也都设置其中，并把 `!OSAtomicCompareAndSwap32Barrier(oldf, newf, (volatile int32_t *)&flags)` 作为 `do while` 循环的循环条件，保证 `uint32_t oldf` 的值都能写入 `uint32_t flags` 中。
```c++
// set and clear must not overlap
void changeInfo(uint32_t set, uint32_t clear) {
    ASSERT(isFuture()  ||  isRealized());
    ASSERT((set & clear) == 0);
    
    data()->changeFlags(set, clear);
}
```
### `FAST_HAS_DEFAULT_RR/RW_HAS_DEFAULT_RR`
&emsp;`FAST_HAS_DEFAULT_RR` 用以在 `__LP64__` 平台下判断 `objc_class` 的 `class_data_bits_t bits` 第二位的值是否为 `1`，以此表示该类或者父类是否有如下函数的默认实现。对应在 `非 __LP64` 平台下，则是使用 `RW_HAS_DEFAULT_RR`，且判断的位置发生了变化，`RW_HAS_DEFAULT_RR` 用以判断从 `objc_class` 的 `class_data_bits_t bits` 中取的得 `class_rw_t` 指针指向的 `class_rw_t` 实例的 `uint32_t flags` 的第 `14` 位的值是否为 `1`，以此表示该类或者父类是否有如下函数的默认实现：  

+ `retain/release/autorelease/retainCount`
+ `_tryRetain/_isDeallocating/retainWeakReference/allowsWeakReference`

```c++
// Values for class_rw_t->flags (RW_*), cache_t->_flags (FAST_CACHE_*), 
// or class_t->bits (FAST_*).
// 当从 class_rw_t->flags 取值时，使用 RW_* 做前缀，
// 当从 cache_t->_flags 取值时，使用 FAST_CACHE_* 做前缀，
// 当从 class_data_bits_t->bits 取值时，使用 FAST_* 做前缀。 

// FAST_* and FAST_CACHE_* are stored on the class, 
// reducing pointer indirection.
// FAST_* 和 FAST_CACHE_* 前缀开头的值，
// 分别保存在 objc_class 的 class_data_bits_t bits 和 cache_t cache 两个成员变量中，
// 直接减少了指针间接寻值，
// RW_* 前缀开头的值首先要从 class_data_bits_t bits 中找到 class_rw_t 的指针，
// 然后根据指针再去寻值。

#if __LP64__
...
// class or superclass has default retain/release/autorelease/retainCount/
//   _tryRetain/_isDeallocating/retainWeakReference/allowsWeakReference
// 类或者其父类
// 在 class_data_bits_t bits 的 uintptr_t bits 中判断。

#define FAST_HAS_DEFAULT_RR     (1UL<<2)
...
#else
...
// class or superclass has default retain/release/autorelease/retainCount/
//   _tryRetain/_isDeallocating/retainWeakReference/allowsWeakReference
// 类或者父类
// 在 class_rw_t 的 uint32_t flags 中判断。 

#define RW_HAS_DEFAULT_RR     (1<<14)
...
#endif
```
### `hasCustomRR/setHasDefaultRR/setHasCustomRR`
&emsp;在`__LP64__` 平台和其它平台下的判断、设置、清除 `objc_class` 的默认 `RR` 函数的标记位。
```c++
#if FAST_HAS_DEFAULT_RR

    // 直接判断 class_data_bits_t bits 的 uintptr_t bits 二进制表示的第 2 位的值是否为 1。
    bool hasCustomRR() const {
        return !bits.getBit(FAST_HAS_DEFAULT_RR);
    }
    
    // 以原子方式把 class_data_bits_t bits 的 uintptr_t bits 二进制表示的第 2 位设置为 1。
    void setHasDefaultRR() {
        bits.setBits(FAST_HAS_DEFAULT_RR);
    }
    
    // Default 和 Custom 是相反的，如果 objc_class 有 CustomRR 则没有 DefaultRR。
    // 以原子方式把 class_data_bits_t bits 的 uintptr_t bits 二进制表示的第 2 位设置为 0，
    // 表示 objc_class 有自定义的 RR 函数，
    // 即 objc_class 的 RR 函数已经被重载了。
    
    void setHasCustomRR() {
        bits.clearBits(FAST_HAS_DEFAULT_RR);
    }
#else

    // 直接判断 class_rw_t 的 flags 二进制表示的第 14 位的值是否为 1。
    bool hasCustomRR() const {
        return !(bits.data()->flags & RW_HAS_DEFAULT_RR);
    }
    
    // 以原子方式把 class_rw_t 的 uint32_t flags 二进制表示的第 14 位设置为 1。
    void setHasDefaultRR() {
        bits.data()->setFlags(RW_HAS_DEFAULT_RR);
    }
    
    // Default 和 Custom 是相反的，如果 objc_class 有 CustomRR 则没有 DefaultRR。
    // 以原子方式把 class_rw_t 的 uint32_t flags 二进制表示的第 14 位设置为 0，
    // 表示 objc_class 有自定义的 RR 函数，
    // 即 objc_class 的 RR 函数已经被重载了。
    
    void setHasCustomRR() {
        bits.data()->clearFlags(RW_HAS_DEFAULT_RR);
    }
#endif
```
### `FAST_CACHE_HAS_DEFAULT_AWZ/RW_HAS_DEFAULT_AWZ`
&emsp;`FAST_CACHE_HAS_DEFAULT_AWZ` 用以在 `__LP64__` 平台下判断 `objc_class` 的 `cache_t cache` 的 `uint16_t _flags` 二进制表示时第 `14` 位的值是否为 `1`，以此表示该类或者父类是否有 `alloc/allocWithZone` 函数的默认实现。（注意，这里和上面的 `RR` 不同，`RR` 是一组实例方法保存在类中，而 `alloc/allocWithZone` 是一组类方法保存在元类中。）而在 `非 __LP64__` 平台下，则是使用 `RW_HAS_DEFAULT_AWZ`，且判断的位置发生了变化，`RW_HAS_DEFAULT_AWZ` 用以判断从 `objc_class` 的 `class_data_bits_t bits` 中取的得 `class_rw_t` 指针指向的 `class_rw_t` 实例的 `uint32_t flags` 的第 `16` 位的值是否为 `1`，以此表示该类或者父类是否有 `alloc/allocWithZone` 函数的默认实现。 
```c++

// Values for class_rw_t->flags (RW_*), cache_t->_flags (FAST_CACHE_*), 
// or class_t->bits (FAST_*).
// 当从 class_rw_t->flags 取值时，使用 RW_* 做前缀，
// 当从 cache_t->_flags 取值时，使用 FAST_CACHE_* 做前缀，
// 当从 class_data_bits_t->bits 取值时，使用 FAST_* 做前缀 

// FAST_* and FAST_CACHE_* are stored on the class, 
// reducing pointer indirection.
// FAST_* 和 FAST_CACHE_* 前缀开头的值分别保存在
// objc_class 的 class_data_bits_t bits 和 cache_t cache 两个成员变量中，
// 直接减少了指针间接寻值，
// RW_* 前缀开头的值首先要从 class_data_bits_t bits 中找到 class_rw_t 的指针，
// 然后根据指针再去寻值。

#if __LP64__
...
// class or superclass has default alloc/allocWithZone: implementation.
// 类或父类有默认的 alloc/allocWithZone: 函数实现。
// Note this is is stored in the metaclass.
// 注意，alloc/allocWithZone: 都是类方法，它们都是保存在元类中的。

#define FAST_CACHE_HAS_DEFAULT_AWZ    (1<<14)
...
#else
...
// class or superclass has default alloc/allocWithZone: implementation.
// 类或父类有默认的 alloc/allocWithZone: 函数实现。
// Note this is is stored in the metaclass.
// 注意，alloc/allocWithZone: 都是类方法，它们都是保存在元类中的。

#define RW_HAS_DEFAULT_AWZ    (1<<16)
...

#endif
```
### `hasCustomAWZ/setHasDefaultAWZ/setHasDefaultAWZ`
&emsp;在 `__LP64__` 平台和其它平台下判断、设置、清除 `objc_class` 的默认 `AWZ` 函数的标记位。
```c++
#if FAST_CACHE_HAS_DEFAULT_AWZ
        
    // 直接判断 cache_t cache 的 uint16_t _flags 二进制表示的第 14 位的值是否为 1。
    bool hasCustomAWZ() const {
        return !cache.getBit(FAST_CACHE_HAS_DEFAULT_AWZ);
    }
    
    // 以原子方式把 cache_t cahce 的 uint16_t _flags 二进制表示的第 14 位设置为 1。
    void setHasDefaultAWZ() {
        cache.setBit(FAST_CACHE_HAS_DEFAULT_AWZ);
    }
    
    // Default 和 Custom 是相反的，如果 objc_class 有 CustomAWZ 则没有 DefaultAWZ。
    // 以原子方式把 cache_t cache 的 uint16_t _flags 二进制表示的第 14 位设置为 0，
    // 表示 objc_class 有自定义的 alloc/allocWithZone: 函数，
    // 即 objc_class 的 alloc/allocWithZone: 函数已经被重载了。
    
    void setHasCustomAWZ() {
        cache.clearBit(FAST_CACHE_HAS_DEFAULT_AWZ);
    }
#else
    // 直接判断 class_rw_t 的 flags 二进制表示的第 16 位的值是否为 1。
    bool hasCustomAWZ() const {
        return !(bits.data()->flags & RW_HAS_DEFAULT_AWZ);
    }
    
    // 以原子方式把 class_rw_t 的 uint32_t flags 二进制表示的第 16 位设置为 1。
    void setHasDefaultAWZ() {
        bits.data()->setFlags(RW_HAS_DEFAULT_AWZ);
    }
    
    // Default 和 Custom 是相反的，如果 objc_class 有 CustomAWZ 则没有 DefaultAWZ。
    // 以原子方式把 class_rw_t 的 uint32_t flags 二进制表示的第 16 位设置为 0，
    // 表示 objc_class 有自定义的 alloc/allocWithZone: 函数，
    // 即 objc_class 的 alloc/allocWithZone: 函数已经被重载了。
    
    void setHasCustomAWZ() {
        bits.data()->clearFlags(RW_HAS_DEFAULT_AWZ);
    }
#endif
```
### `FAST_CACHE_HAS_DEFAULT_CORE/RW_HAS_DEFAULT_CORE`
&emsp;`FAST_CACHE_HAS_DEFAULT_CORE` 用以在 `__LP64__` 平台下判断 `objc_class` 的 `cache_t cache` 的 `uint16_t _flags` 二进制表示时第 `15` 位的值是否为 `1`，以此表示该类或者父类是否有 `new/self/class/respondsToSelector/isKindOfClass` 函数的默认实现。而在 `非 __LP64__` 平台下，则是使用 `RW_HAS_DEFAULT_CORE`，且判断的位置发生了变化，`RW_HAS_DEFAULT_CORE` 用以判断从 `objc_class` 的  `class_data_bits_t bits` 中取得 `class_rw_t` 指针指向的 `class_rw_t` 实例的 `uint32_t flags` 的第 `13` 位的值是否为 `1`，以此表示该类或者父类是否有 `new/self/class/respondsToSelector/isKindOfClass` 函数的默认实现。
```c++
#if __LP64__
...
// class or superclass has default new/self/class/respondsToSelector/isKindOfClass
// 类或者父类有默认的 new/self/class/respondsToSelector/isKindOfClass

#define FAST_CACHE_HAS_DEFAULT_CORE   (1<<15)
...
#else
...
// class or superclass has default new/self/class/respondsToSelector/isKindOfClass
// 类或者父类有默认的 new/self/class/respondsToSelector/isKindOfClass

#define RW_HAS_DEFAULT_CORE   (1<<13)
...
#endif
```
### `hasCustomCore/setHasDefaultCore/setHasCustomCore`
&emsp;在 `__LP64__` 平台和其它平台下判断、设置、清除 `objc_class` 的默认 `Core` 函数的标记位。
```c++
#if FAST_CACHE_HAS_DEFAULT_CORE
    // 直接判断 cache_t cache 的 uint16_t _flags 二进制表示的第 15 位的值是否为 1。
    bool hasCustomCore() const {
        return !cache.getBit(FAST_CACHE_HAS_DEFAULT_CORE);
    }
    
    // 以原子方式把 cache_t cache 的 uint16_t _flags 二进制表示的第 15 位设置为 1。
    void setHasDefaultCore() {
        return cache.setBit(FAST_CACHE_HAS_DEFAULT_CORE);
    }
    
    // Default 和 Custom 是相反的，如果 objc_calss 有 CustomCore 则没有 DefaultCore。
    // 以原子方式把 cache_t cache 的 uint16_t _flags 二进制表示的第 15 位设置为 0，
    // 表示 objc_calss 有自定义的 new/self/class/respondsToSelector/isKindOfClass 函数，
    // 即 objc_class 的 new/self/class/respondsToSelector/isKindOfClass 函数已经被重载了。 
    
    void setHasCustomCore() {
        return cache.clearBit(FAST_CACHE_HAS_DEFAULT_CORE);
    }
#else
    // 直接判断 class_rw_t 的 uint32_t flags 二进制表示的第 13 位的值是否为 1。
    bool hasCustomCore() const {
        return !(bits.data()->flags & RW_HAS_DEFAULT_CORE);
    }
    
    // 以原子方式把 class_rw_t 的 uint32_t flags 二进制表示的第 13 位设置为 1。
    void setHasDefaultCore() {
        bits.data()->setFlags(RW_HAS_DEFAULT_CORE);
    }
    
    // Default 和 Custom 是相反的，如果 objc_class 有 CustomCore 则没有 DefaultCore。
    // 以原子方式把 class_rw_t 的 uint32_t flags 二进制表示的第 13 位设置为 0，
    // 表示 objc_class 有自定义的 new/self/class/respondsToSelector/isKindOfClass 函数，
    // 即 objc_class 的 new/self/class/respondsToSelector/isKindOfClass 函数已经被重载了。
    void setHasCustomCore() {
        bits.data()->clearFlags(RW_HAS_DEFAULT_CORE);
    }
#endif
```
### `FAST_CACHE_HAS_CXX_CTOR/RW_HAS_CXX_CTOR/FAST_CACHE_HAS_CXX_DTOR/RW_HAS_CXX_DTOR`
&emsp;`FAST_CACHE_HAS_CXX_CTOR` 用以在 `__LP64__` 平台下判断 `objc_class` 的 `cache_t cache` 的 `uint16_t _flags` 二进制表示时第 `1` 位的值是否为 `1`，以此表示该类或者父类是否有 `.cxx_construct` 函数实现。而在 `非 __LP64__` 平台下，则是使用 `RW_HAS_CXX_CTOR`，且判断的位置发生了变化，`RW_HAS_CXX_CTOR` 用以判断从 `objc_class` 的 `class_data_bits_t bits` 中取得 `class_rw_t` 指针指向的 `class_rw_t` 实例的 `uint32_t flags` 的第 `18` 位的值是否为 `1`，以此表示该类或者父类是否有 `.cxx_construct` 函数实现。对应的 `FAST_CACHE_HAS_CXX_DTOR` 和 `RW_HAS_CXX_DTOR` 表示该类或者父类是否有 `.cxx_destruct` 函数实现。
这里需要注意的是在 `__LP64__ && __arm64__` 平台下 `FAST_CACHE_HAS_CXX_DTOR` 是 `1<<0`，而在 `__LP64__ && !__arm64__` 平台下 `FAST_CACHE_HAS_CXX_DTOR` 是 `1<<2`。 
```c++
#if __LP64__
...
#if __arm64__
// class or superclass has .cxx_construct/.cxx_destruct implementation。
// 类或者父类有 .cxx_construct/.cxx_destruct 函数实现。

// FAST_CACHE_HAS_CXX_DTOR is the first bit so that setting
// it in isa_t::has_cxx_dtor is a single bfi.
// FAST_CACHE_HAS_CXX_DTOR 是第一位，
// 因此在 isa_t::has_cxx_dtor 中进行设置仅是一个位字段。
// uintptr_t has_cxx_dtor      : 1;

// __LP64__ && __arm64__ 平台下

#define FAST_CACHE_HAS_CXX_DTOR       (1<<0)
#define FAST_CACHE_HAS_CXX_CTOR       (1<<1)
...
#else
...
// class or superclass has .cxx_construct/.cxx_destruct implementation.
// 类或者父类有 .cxx_construct/.cxx_destruct 函数实现。

// FAST_CACHE_HAS_CXX_DTOR is chosen to alias with isa_t::has_cxx_dtor.
// 选择 FAST_CACHE_HAS_CXX_DTOR 作为 isa_t::has_cxx_dtor 的别名。

// __LP64__ && !__arm64__ 平台下

#define FAST_CACHE_HAS_CXX_CTOR       (1<<1)
#define FAST_CACHE_HAS_CXX_DTOR       (1<<2)
#endif
...
#else
...
// class or superclass has .cxx_construct implementation.
// 类或者父类有 .cxx_construct 函数实现。
#define RW_HAS_CXX_CTOR       (1<<18)

// class or superclass has .cxx_destruct implementation
// 类或者父类有 .cxx_destruct 函数实现。 
#define RW_HAS_CXX_DTOR       (1<<17)
...
#endif
```
### `hasCxxCtor/setHasCxxCtor/hasCxxDtor/setHasCxxDtor`
&emsp;在 `__LP64__` 平台和其它平台下判断、设置（注意这里没有清除）`objc_class` 的 `.cxx_construct/.cxx_destruct` 函数实现的标记位。
```c++
#if FAST_CACHE_HAS_CXX_CTOR
    // 直接判断 cache_t cache 的 uint16_t _flags 二进制表示的第 1 位的值是否为 1。
    bool hasCxxCtor() {
        ASSERT(isRealized());
        return cache.getBit(FAST_CACHE_HAS_CXX_CTOR);
    }
    
    // 以原子方式把 cache_t cache 的 uint16_t _flags 二进制表示的第 1 位设置为 1。
    void setHasCxxCtor() {
        cache.setBit(FAST_CACHE_HAS_CXX_CTOR);
    }
#else
    // 直接判断 class_rw_t 的 flags 二进制表示的第 18 位的值是否为 1。
    bool hasCxxCtor() {
        ASSERT(isRealized());
        return bits.data()->flags & RW_HAS_CXX_CTOR;
    }
    
    // 以原子方式把 class_rw_t 的 uint32_t flags 二进制表示的第 18 位设置为 1。
    void setHasCxxCtor() {
        bits.data()->setFlags(RW_HAS_CXX_CTOR);
    }
#endif

#if FAST_CACHE_HAS_CXX_DTOR
    // 在 __LP64__ 的 __arm64__ 下是 0，在 !__arm64__ 下是 2。
    // 直接判断 cache_t cache 的 uint16_t _flags 二进制表示的第 0/2 位的值是否为 1。
    bool hasCxxDtor() {
        ASSERT(isRealized());
        return cache.getBit(FAST_CACHE_HAS_CXX_DTOR);
    }
    
    // 以原子方式把 cache_t cache 的 uint16_t _flags 二进制表示的第 0/2 位设置为 1。
    void setHasCxxDtor() {
        cache.setBit(FAST_CACHE_HAS_CXX_DTOR);
    }
#else
    // 直接判断 class_rw_t 的 uint32_t flags 二进制表示的第 17 位的值是否为 1。
    bool hasCxxDtor() {
        ASSERT(isRealized());
        return bits.data()->flags & RW_HAS_CXX_DTOR;
    }
    
    // 以原子方式把 class_rw_t 的 uint32_t flags 二进制表示的第 17 位设置为 1。
    void setHasCxxDtor() {
        bits.data()->setFlags(RW_HAS_CXX_DTOR);
    }
#endif
```
### `FAST_CACHE_REQUIRES_RAW_ISA/RW_REQUIRES_RAW_ISA`
&emsp;`FAST_CACHE_REQUIRES_RAW_ISA` 用以在 `__LP64__` 平台下判断 `objc_class` 的 `cache_t cache` 的 `uint16_t _flags` 二进制表示时第 `13` 位的值是否为 `1`，以此表示类实例对象（此处是指类对象，不是使用类构建的实例对象，一定要记得）是否需要原始的 `isa`。而在 `非 __LP64__` 且 `SUPPORT_NONPOINTER_ISA` 的平台下，则是使用 `RW_REQUIRES_RAW_ISA`，且判断的位置发生了变化，`RW_REQUIRES_RAW_ISA` 用以判断从 `objc_class` 的 `class_data_bits_t bits` 中取得 `class_rw_t` 指针指向的 `class_rw_t` 实例的 `uint32_t flags` 的第 `15` 位的值是否为 `1`，以此表示类实例对象（此处是指类对象，不是使用类构建的实例对象，一定要记得）是否需要原始的 `isa`。
```c++
#if __LP64__
...
// class's instances requires raw isa.
// 类实例需要 raw isa。

#define FAST_CACHE_REQUIRES_RAW_ISA   (1<<13)
...

#else
...
// class's instances requires raw isa.
// 类实例需要 raw isa。

#if SUPPORT_NONPOINTER_ISA
#define RW_REQUIRES_RAW_ISA   (1<<15)
#endif
...

#endif
```
### `instancesRequireRawIsa/setInstancesRequireRawIsa`
&emsp;在 `__LP64__` 平台和其它平台下判断、设置类实例（此处是指类对象，不是使用类构建的实例对象，一定要记得）需要原始 `isa` 的标记位。
```c++
#if FAST_CACHE_REQUIRES_RAW_ISA
    // 直接判断 cache_t cache 的 uint16_t _flags 二进制表示的第 13 位的值是否为 1。
    bool instancesRequireRawIsa() {
        return cache.getBit(FAST_CACHE_REQUIRES_RAW_ISA);
    }
    
    // 以原子方式把 cache_t cache 的 uint16_t _flags 二进制表示的第 13 位设置为 1。
    void setInstancesRequireRawIsa() {
        cache.setBit(FAST_CACHE_REQUIRES_RAW_ISA);
    }
#elif SUPPORT_NONPOINTER_ISA
    // 直接判断 class_rw_t 的 uint32_t flags 二进制表示的第 15 位的值是否为 1。
    bool instancesRequireRawIsa() {
        return bits.data()->flags & RW_REQUIRES_RAW_ISA;
    }
    
    // 以原子方式把 class_rw_t 的 uint32_t flags 二进制表示的第 15 位设置为 1。
    void setInstancesRequireRawIsa() {
        bits.data()->setFlags(RW_REQUIRES_RAW_ISA);
    }
#else
    // 当 isa 是原始 isa 时，直接返回 true。
    bool instancesRequireRawIsa() {
        return true;
    }
    void setInstancesRequireRawIsa() {
        // nothing
    }
#endif
```
+ 当 `! __LP64__` 平台下，所有掩码都存储在 `class_rw_t` 的 `uint32_t flags`。
+ `__LP64__` 平台下，`FAST_HAS_DEFAULT_RR` 存储在 `class_data_bits_t bits` 的 `uintptr_t bits`。（`1UL<<2`）（`retain/release/autorelease/retainCount/_tryRetain/_isDeallocating/retainWeakReference/allowsWeakReference`）
+ `__LP64__` 平台下，`FAST_CACHE_HAS_DEFAULT_AWZ` 存储在 `cache_t cache` 的 `uint16_t _flags` 下。（`1<<14`）（`alloc/allocWithZone:`）
+ `__LP64__` 平台下，`FAST_CACHE_HAS_DEFAULT_CORE` 存储在 `cache_t cache` 的 `uint16_t _flags` 下。（`1<<15`）（`new/self/class/respondsToSelector/isKindOfClass`）
+ `__LP64__` 平台下，`FAST_CACHE_HAS_CXX_CTOR` 存储在 `cache_t cache` 的 `uint16_t _flags` 下。（`1<<1`）（`.cxx_construct`）
+ `__LP64__` 平台下，`FAST_CACHE_HAS_CXX_DTOR` 存储在 `cache_t cache` 的 `uint16_t _flags` 下。（`1<<2` / `1<<0`）（`.cxx_destruct`）
+ `__LP64__` 平台下，`FAST_CACHE_REQUIRES_RAW_ISA` 存储在 `cache_t cache` 的 `uint16_t _flags` 下。（`1<<13`）（`requires raw isa`）

### `void printInstancesRequireRawIsa(bool inherited)`
&emsp;打印类对象需要原始 `isa`，当环境变量 `OBJC_PRINT_RAW_ISA` `Value` 为 `true` 时会调用该函数，`inherited` 表示该类是否是一个子类。
`OPTION( PrintRawIsa, OBJC_PRINT_RAW_ISA, "log classes that require raw pointer isa fields")`
```c++
void
objc_class::printInstancesRequireRawIsa(bool inherited)
{
    // 打印标识开启，否则执行断言
    ASSERT(PrintRawIsa);
    // 类对象需要原始的 isa，否则执行断言 
    ASSERT(instancesRequireRawIsa());
    
    // 控制台输出需要原始 isa 的类名等信息
    _objc_inform("RAW ISA:  %s%s%s", nameForLogging(), 
                 isMetaClass() ? " (meta)" : "", 
                 inherited ? " (inherited)" : "");
}
```

### `void setInstancesRequireRawIsaRecursively(bool inherited = false)`
&emsp;将此类及其所有子类标记为需要原始 `isa` 指针，标记函数 `setInstancesRequireRawIsa` 很简单，上面我们已经分析过了， 这里涉及到一个更重要的知识点，就是我们如何才能获取一个类的所有子类呢 ？这里正式使用到了 `struct class_rw_t` 的两个成员变量 `Class firstSubclass` 和 `Class nextSiblingClass`，下面我们跟着函数调用流程一起来分析一下吧。

```c++
/*
* Mark this class and all of its subclasses as requiring raw isa pointers.
* 将此类及其所有子类标记为需要原始 isa 指针。
*/
void objc_class::setInstancesRequireRawIsaRecursively(bool inherited)
{
    // struct objc_class 的函数内部的 this 自然是 objc_class *。
    
    // 取得 objc_class 指针。
    Class cls = (Class)this;
    
    // 加锁，加锁失败则执行断言。
    runtimeLock.assertLocked();
    
    // 如果此类已经被标记为需要原始 isa，则直接 return。
    if (instancesRequireRawIsa()) return;
    
    // 枚举一个类及其所有已实现的子类。
    //（foreach_realized_class_and_subclass 函数是我们要重点关注的对象，正是它获取了所有子类）
    foreach_realized_class_and_subclass(cls, [=](Class c){
        
        if (c->instancesRequireRawIsa()) {
            // 如果此类已经被标记为需要原始 isa，则直接 return false，跳过该类继续遍历下一个子类。
            return false;
        }
        
        // 把 c 标记为需要原始 isa。
        c->setInstancesRequireRawIsa();
        
        // 是否在控制台打印
        if (PrintRawIsa) c->printInstancesRequireRawIsa(inherited || c != cls);
        
        // return true 继续执行遍历。
        return true;
    });
}
```
#### `foreach_realized_class_and_subclass`
```c++
// Enumerates a class and all of its realized subclasses.
// 枚举一个类及其所有已实现的子类。
static void
foreach_realized_class_and_subclass(Class top, bool (^code)(Class) __attribute((noescape)))
{
    unsigned int count = unreasonableClassCount();

    foreach_realized_class_and_subclass_2(top, count, false, code);
}
```
#### `unreasonableClassCount`
```c++
/*
* unreasonableClassCount
* Provides an upper bound for any iteration of classes, to prevent spins when runtime metadata is corrupted.
* 为类的任何迭代提供上限，以防止在运行时元数据损坏时发生死循环。
*/
static unsigned unreasonableClassCount()
{
    // 加锁
    runtimeLock.assertLocked();
    
    int base = NXCountMapTable(gdb_objc_realized_classes) +
    getPreoptimizedClassUnreasonableCount();

    // Provide lots of slack here. Some iterations touch metaclasses too.
    // 在此处提供大量的余地。一些迭代也涉及元类。
    // Some iterations backtrack (like realized class iteration).
    // 一些迭代回溯（例如实现的类迭代）。
    // We don't need an efficient bound, merely one that prevents spins.
    // 我们不需要有效的界限，只需防止旋转即可。
    
    return (base + 1) * 16;
}
```
#### `foreach_realized_class_and_subclass_2`
```c++
/*
* Class enumerators 类枚举器
* The passed in block returns `false` if subclasses can be skipped.
* 如果可以跳过子类，则传入的块将返回 "false"。
* Locking: runtimeLock must be held by the caller.
* runtimeLock 必须由调用方持有。
*/

// foreach_realized_class_and_subclass_2(top, count, false, code);
// top 是当前类，skip_metaclass 值是 false，code 就是我们枚举时的块 
// __attribute((noescape)) 想到了 Swift 中的闭包

static inline void
foreach_realized_class_and_subclass_2(Class top, unsigned &count,
                                      bool skip_metaclass,
                                      bool (^code)(Class) __attribute((noescape)))
{
    Class cls = top;

    runtimeLock.assertLocked();
    ASSERT(top);

    while (1) {
        if (--count == 0) {
            _objc_fatal("Memory corruption in class list.");
        }

        bool skip_subclasses;

        if (skip_metaclass && cls->isMetaClass()) {
            skip_subclasses = true;
        } else {
            skip_subclasses = !code(cls);
        }

        if (!skip_subclasses && cls->data()->firstSubclass) {
            cls = cls->data()->firstSubclass;
        } else {
            while (!cls->data()->nextSiblingClass  &&  cls != top) {
                cls = cls->superclass;
                if (--count == 0) {
                    _objc_fatal("Memory corruption in class list.");
                }
            }
            if (cls == top) break;
            cls = cls->data()->nextSiblingClass;
        }
    }
}
```
### `bool canAllocNonpointer()`
&emsp;表示 `objc_class` 的 `isa` 是非指针，即类对象不需要原始 `isa` 时，能根据该函数返回值设置 `isa_t isa` 的 `uintptr_t nonpointer : 1` 字段，标记该类的 `isa` 是非指针。
```c++
bool canAllocNonpointer() {
    ASSERT(!isFuture());
    return !instancesRequireRawIsa();
}
```
### `bool isSwiftStable()`
&emsp;调用 `class_data_bits_t bits` 的 `isSwiftStable` 函数，内部实现是通过与操作判断 `uintptr_t bits` 的二进制表示的第 `1` 位是否是 `1`，表示该类是否是有稳定的 `Swift ABI` 的 `Swift` 类。
```c++
// class is a Swift class from the stable Swift ABI.
// class 是一个有稳定的 Swift ABI 的 Swift类。
// #define FAST_IS_SWIFT_STABLE    (1UL<<1)

bool isSwiftStable() {
    return bits.isSwiftStable();
}
```
### `bool isSwiftLegacy()`
```c++
bool isSwiftLegacy() {
    return bits.isSwiftLegacy();
}
```

# 方法查找的慢速查找的知识点需要补充。

## 参考链接
**参考链接:🔗**
+ [iOS之LLDB常用命令](https://juejin.im/post/6869621360415637518)
