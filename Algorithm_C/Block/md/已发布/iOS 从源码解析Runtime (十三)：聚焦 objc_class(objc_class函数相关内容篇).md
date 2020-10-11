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
```c++
#if __LP64__
...
// class or superclass has default new/self/class/respondsToSelector/isKindOfClass
// 类或者
#define FAST_CACHE_HAS_DEFAULT_CORE   (1<<15)
...
#else
...
// class or superclass has default new/self/class/respondsToSelector/isKindOfClass
#define RW_HAS_DEFAULT_CORE   (1<<13)
...
#endif
```

## 参考链接
**参考链接:🔗**
+ [iOS之LLDB常用命令](https://juejin.im/post/6869621360415637518)
