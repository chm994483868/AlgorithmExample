# iOS 从源码解析Runtime (十二)：聚焦 objc_class(class_rw_t)
> 上篇我们详细分析了 `class_ro_t`，其中可能有点模糊的是  `class_data_bits_t bits` 的 `data` 函数和 `safe_ro` 函数，其中如果类是处于未实现完成状态时 `data` 函数返回的是 `class_ro_t`，当类实现完成后返回的则是 `class_rw_t`，且当类实现以后 `class_rw_t` 有一个 `ro` 函数来返回 `class_ro_t`，那这是怎么回事呢，这篇我们就来详细分析一下 ⛽️⛽️。

## `class_rw_t`
&emsp;`class_rw_t` 的成员变量。
```c++
struct class_rw_t {
    // Be warned that Symbolication knows the layout of this structure.
    // 警告符号表示知道此结构的布局
    uint32_t flags;
    
    // 目击者、证人
    uint16_t witness;
    
#if SUPPORT_INDEXED_ISA // isa 中保存 indexcls，大概是 watchOS
    uint16_t index;
#endif

    // std::atomic<uintptr_t>
    // 原子性 unsigned long
    explicit_atomic<uintptr_t> ro_or_rw_ext;

    // 当前所属类的第一个子类
    Class firstSubclass;
    // 姊妹类、兄弟类
    Class nextSiblingClass;
    ...
};
```
### `class_rw_t private`
```c++

```

### `list_array_tt`
&emsp;继续之前先看下一个


## 参考链接
**参考链接:🔗**
+ [iOS之LLDB常用命令](https://juejin.im/post/6869621360415637518)
