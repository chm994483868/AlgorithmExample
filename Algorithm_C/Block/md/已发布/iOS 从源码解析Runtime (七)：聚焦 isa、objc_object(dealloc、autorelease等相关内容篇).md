# iOS 从源码解析Runtime (七)：聚焦 isa、objc_object(dealloc、autorelease等相关内容篇)

> 上一篇我们非常非常详尽的分析了自动释放池的相关的源码，这篇我们继续学习 `objc_object` 剩余的函数。


## `rootAutorelease`
&emsp;看到 `if (prepareOptimizedReturn(ReturnAtPlus1)) return (id)this;` 原来并不是所有对象都会被放进自动释放池的（Tagged Pointer  除外）。
```c++
// Base autorelease implementation, ignoring overrides.
inline id 
objc_object::rootAutorelease()
{
    // 如果是 Tagged Pointer 则直接返回 (id)this
    if (isTaggedPointer()) return (id)this;
    
    // 如果 prepareOptimizedReturn(ReturnAtPlus1) 返回 true，则直接返回 (id)this
    if (prepareOptimizedReturn(ReturnAtPlus1)) return (id)this;
    
    // 否则正常调用 AutoreleasePoolPage::autorelease((id)this) 把 this 放进自动释放池
    return rootAutorelease2();
}
```
### `ReturnDisposition`
&emsp;如果硬翻的话，可以理解为 “返回布置/安排”，而枚举的两个值分别是 “加 0”/“加 1”。
```c++
enum ReturnDisposition : bool {
    ReturnAtPlus0 = false, 
    ReturnAtPlus1 = true
};
```
### `prepareOptimizedReturn`
```c++
// Try to prepare for optimized return with the given disposition (+0 or +1).
// Returns true if the optimized path is successful.
// Otherwise the return value must be retained and/or autoreleased as usual.
static ALWAYS_INLINE bool 
prepareOptimizedReturn(ReturnDisposition disposition)
{
    ASSERT(getReturnDisposition() == ReturnAtPlus0);

    if (callerAcceptsOptimizedReturn(__builtin_return_address(0))) {
        if (disposition) setReturnDisposition(disposition);
        return true;
    }

    return false;
}
```

## 参考链接
**参考链接:🔗**
+ []()
+ [操作系统内存管理(思维导图详解)](https://blog.csdn.net/hguisu/article/details/5713164)
