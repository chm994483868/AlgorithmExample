# iOS 从源码解析Runtime (十三)：聚焦 objc_class(objc_class函数相关内容篇)

> 前面已经忘记看了多少天了，终于把 `objc_object` 和 `objc_class` 的相关的数据结构都看完了，现在只剩下 `objc_class` 中定义的函数没有分析，那么就由本篇开始分析吧！⛽️⛽️


## `objc_class 函数`

### `class_rw_t *data() const`
&emsp;`data` 函数是直接调用的 `bits` 的 `data` 函数，内部实现的话也很简单，通过掩码 `#define FAST_DATA_MASK 0x00007ffffffffff8UL`（二进制第 `3`-`46` 位是 `1`，其他位都是 `0`） 从 `bits` 中取得 `class_rw_t` 指针，如果类处于未实现完成的状态的话返回的则是 `class_ro_t` 指针。
```c++
class_rw_t *data() const {
    return bits.data();
}
```
### `void setData(class_rw_t *newData)`
&emsp;同样 `setData` 函数调用的也是 `bits` 的 `setData` 函数，在 `class_data_bits_t` 的 `setData` 函数中， `bits` 首先和 `~FAST_DATA_MASK` 做与操作把掩码位全部置为 `0` 其它位保持不变，然后再和 `newData` 做或操作，把 `newData` 中值为 `1` 的位对应到 `bits` 也置为 `1`。
```c++
void setData(class_rw_t *newData) {
    bits.setData(newData);
}
```
### `void setInfo(uint32_t set)`
&emsp;`setInfo` 函数调用的是 `class_rw_t` 的 `setFlags` 函数，通过原子的或操作把 `set` 的位设置到 `class_rw_t` 的 `flags` 中。（`flags` 和 `set` 都是 `uint32_t` 类型，都是 `32` 位。）
```c++
void setInfo(uint32_t set) {
    ASSERT(isFuture()  ||  isRealized());
    data()->setFlags(set);
}
```
### `void clearInfo(uint32_t clear)`
&emsp;`clearInfo` 函数调用的是 `class_rw_t` 的 `clearFlags` 函数，通过原子的与操作把 `~clear` 中是 `0` 的位也设置到 `class_rw_t` 的 `flags` 中。（`flags` 和 `clear` 都是 `uint32_t`  类型，都是 `32` 位。）
```c++
void clearInfo(uint32_t clear) {
    ASSERT(isFuture()  ||  isRealized());
    data()->clearFlags(clear);
}
```
### `void changeInfo(uint32_t set, uint32_t clear)`
&emsp;`changeInfo` 函数调用的同样的是 `class_rw_t` 的 `changeFlags` 函数，先取得 `class_rw_t` 的 `flags` 赋值到一个临时变量 `oldf` 中，然后 `oldf` 和 `set` 做一个或操作，把 `set` 值为 `1` 的位也都设置到 `oldf` 中，然后再和 `~clear` 与操作，把 `~clear` 中是 `0` 的位也都设置其中，然后通过 `OSAtomicCompareAndSwap32Barrier` 保证 `oldf` 的值都能写入 `flags` 中。
```c++
// set and clear must not overlap
void changeInfo(uint32_t set, uint32_t clear) {
    ASSERT(isFuture()  ||  isRealized());
    ASSERT((set & clear) == 0);
    
    data()->changeFlags(set, clear);
}
```
### `FAST_HAS_DEFAULT_RR`
```c++
#if __LP64__

...

// class or superclass has default retain/release/autorelease/retainCount/
//   _tryRetain/_isDeallocating/retainWeakReference/allowsWeakReference
#define FAST_HAS_DEFAULT_RR     (1UL<<2)
...

#endif
```
### `bool hasCustomRR() const`
```c++
#if FAST_HAS_DEFAULT_RR
    bool hasCustomRR() const {
        return !bits.getBit(FAST_HAS_DEFAULT_RR);
    }
    
    ...
    
#else
    bool hasCustomRR() const {
        return !(bits.data()->flags & RW_HAS_DEFAULT_RR);
    }
    
    ...
    
#endif
```

## 参考链接
**参考链接:🔗**
+ [iOS之LLDB常用命令](https://juejin.im/post/6869621360415637518)
