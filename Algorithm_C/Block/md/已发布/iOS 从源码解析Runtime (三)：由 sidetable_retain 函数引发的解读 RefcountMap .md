# iOS 从源码解析Runtime (三)：由 sidetable_retain 函数引发的解读 RefcountMap   

> 在 [iOS weak 底层实现原理(四)：SideTables和SideTable](https://juejin.im/post/6865670937841238023) 已经解读过 `struct SideTable`，但是当时重点放在了 `weak` 相关内容上，由于较少涉及对象的引用计数相关内容，所以没有详细解读其中的 `RefcountMap refcnts`，那么就由本篇来解读。

```c++
// SideTable 定义
struct SideTable {
spinlock_t slock;
RefcountMap refcnts;
weak_table_t weak_table;

SideTable() {
    memset(&weak_table, 0, sizeof(weak_table));
}
...
};
```

##  `RefcountMap refcnts`
&emsp;`refcnts`（命名应该是 `reference count` 的缩写）是 `struct SideTable` 的一个成员变量，它作为一张散列表来保存对象的引用计数。`RefcountMap` 类型定义如下:
```c++
// RefcountMap disguises its pointers because we don't want the table to act as a root for `leaks`.
// RefcountMap 伪装了它的指针，因为我们不希望该表充当`leaks`的根。
typedef objc::DenseMap<DisguisedPtr<objc_object>,size_t,RefcountMapValuePurgeable> RefcountMap;
```
&emsp;看到 `DenseMap` 的前三个模版参数是: 
1. `DisguisedPtr<objc_object>` 伪装的 `objc_object` 指针。（实际是把地址值转换为整数，可参考[iOS weak 底层实现原理(一)：DisguisedPtr](https://juejin.im/post/6865468675940417550)）
2. `size_t` 表示引用计数的值。
3. `RefcountMapValuePurgeable` 一个结构体，只定义了一个静态内联函数 `isPurgeable`，入参为 `0` 是返回 `true`，否则返回 `false`。

### `RefcountMapValuePurgeable`
```c++
struct RefcountMapValuePurgeable {
    static inline bool isPurgeable(size_t x) {
        return x == 0;
    }
};
```


## 参考链接
**参考链接:🔗**
+ [Objective-C 引用计数原理](http://yulingtianxia.com/blog/2015/12/06/The-Principle-of-Refenrence-Counting/)
+ [C++语法之友元函数、友元类](https://ityongzhen.github.io/C++语法之友元函数、友元类.html/#more)
