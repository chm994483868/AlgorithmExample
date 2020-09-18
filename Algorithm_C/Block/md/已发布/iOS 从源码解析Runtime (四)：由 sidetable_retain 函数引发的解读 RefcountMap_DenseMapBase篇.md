# iOS 从源码解析Runtime (四)：由 sidetable_retain 函数引发的解读 RefcountMap_DenseMapBase篇
## 前言
&emsp;上一节我们从上到下分析了 `DenseMap` 的内容，中间已经涉及到多处 `DenseMapBase` 的使用。`DenseMap` 是 `DenseMapBase` 的子类，而 `DenseMapBase` 是 `DenseMap` 的友元类，所以两者存在多处交织调用。那下面我们就详细分析下 `DenseMapBase` 的实现吧。（这个类实在是太长了，消耗我太多时间，一度想只看下核心实现就不看细枝末节了，但是一想到它涉及到的引用计数的相关操作，再加上强迫症，那就认真看下去吧！⛽️⛽️）

## `DenseMapBase`
&emsp;一个拥有 `6` 个抽象参数的模版类。在 `DenseMap` 定义中，`DenseMapBase` 的第一个抽象参数 `DerivedT`（派生类型、衍生类型） 传递的是 `DenseMap<KeyT, ValueT, ValueInfoT, KeyInfoT, BucketT>` 本身， 从它的名字里面我们大概可猜到一些信息，需要的是一个它的子类，那接下来的分析中我们潜意识里面就把 `DerivedT`  默认当作 `DenseMap` 来用。
先看一下 `DenseMapBase` 的定义:
```c++
// ValueInfoT is used by the refcount table.
// refcount table 使用 ValueInfoT。

// A key/value pair with value==0 is not required to be stored in the refcount table;
// it could correctly be erased instead.
// value 等于 0 的 key/value 对是不需要存储在 refcount table 里的，可以正确的擦除它。

// For performance, we do keep zero values in the table when the true refcount
// decreases to 1: this makes any future retain faster.
// 为了提高性能，当真实引用计数减少到 1 时，我们确实在表中保留了零值，这使得将来的 retain 操作更快进行。

// For memory size, we allow rehashes and table insertions to remove
// a zero value as if it were a tombstone.
// 为了内存大小，我们允许进行重新哈希和表插入以删除零值，就像它是一个逻辑删除一样。

template <typename DerivedT, typename KeyT, typename ValueT,
          typename ValueInfoT, typename KeyInfoT, typename BucketT>
class DenseMapBase { ... };
```
### `const_arg_type_t`
&emsp;用于 `const_arg_type_t<KeyT>` 表示不可变的 `DisguisedPtr<objc_object>`。（模版参数 `KeyT` 是 `DisguisedPtr<objc_object>`） 
```c++
template <typename T>
using const_arg_type_t = typename const_pointer_or_const_ref<T>::type;

/// If T is a pointer to X, return a pointer to const X. If it is not, return const T.
/// 如果T是指向X的指针，则返回指向const X的指针。如果不是，则返回const T

template<typename T, typename Enable = void>
struct add_const_past_pointer { using type = const T; };

template <typename T>
struct add_const_past_pointer<
    T, typename std::enable_if<std::is_pointer<T>::value>::type> {
  using type = const typename std::remove_pointer<T>::type *;
};

template <typename T, typename Enable = void>
struct const_pointer_or_const_ref {
  using type = const T &;
};
template <typename T>
struct const_pointer_or_const_ref<
    T, typename std::enable_if<std::is_pointer<T>::value>::type> {
  using type = typename add_const_past_pointer<T>::type;
};
```
&emsp;给多个类型起别名，这里一定要谨记 `key_type` 表示的是 `KeyT`，就是 `RefcountMap` 定义时用的 `DisguisedPtr<objc_object>`，  `mapped_type` 表示的是 `ValueT`，就是 `RefcountMap` 定义时用的 `size_t`。而 `value_type` 表示的是 `BucketT`，就是 `DenseMap` 默认的模版参数 `typename BucketT = detail::DenseMapPair<KeyT, ValueT>` 即 `detail::DenseMapPair<DisguisedPtr<objc_object>, size_t>`。

+ `key_type` = `KeyT` = `DisguisedPtr<objc_object>`
+ `mapped_type` = `ValueT` = `size_t`
+ `value_type` = `BucketT` = `detail::DenseMapPair<KeyT, ValueT>` = `detail::DenseMapPair<DisguisedPtr<objc_object>, size_t>`。
```c++
using size_type = unsigned;
using key_type = KeyT;
using mapped_type = ValueT;
using value_type = BucketT;

// 迭代器
using iterator = DenseMapIterator<KeyT, ValueT, ValueInfoT, KeyInfoT, BucketT>;
// 不可变的迭代器
using const_iterator = DenseMapIterator<KeyT, ValueT, ValueInfoT, KeyInfoT, BucketT, true>;
```
### `DenseMapIterator`
&emsp;
```c++
template <typename KeyT, typename ValueT, typename ValueInfoT,
          typename KeyInfoT, typename Bucket, bool IsConst>
class DenseMapIterator {

  // 定义两个友元类  IsConst 分别是: true/false
  friend class DenseMapIterator<KeyT, ValueT, ValueInfoT, KeyInfoT, Bucket, true>;
  friend class DenseMapIterator<KeyT, ValueT, ValueInfoT, KeyInfoT, Bucket, false>;

  // 类型别名，IsConst 是 true，表示不可变的迭代器 ConstIterator
  using ConstIterator = DenseMapIterator<KeyT, ValueT, ValueInfoT, KeyInfoT, Bucket, true>;

public:
  using difference_type = ptrdiff_t;
  // 
  using value_type = typename std::conditional<IsConst, const Bucket, Bucket>::type;
  using pointer = value_type *;
  using reference = value_type &;
  using iterator_category = std::forward_iterator_tag;

private:
  pointer Ptr = nullptr;
  pointer End = nullptr;

public:
  DenseMapIterator() = default;

  DenseMapIterator(pointer Pos, pointer E,
                   bool NoAdvance = false)
      : Ptr(Pos), End(E) {
    if (NoAdvance) return;
    AdvancePastEmptyBuckets();
  }

  // Converting ctor from non-const iterators to const iterators. SFINAE'd out
  // for const iterator destinations so it doesn't end up as a user defined copy
  // constructor.
  template <bool IsConstSrc,
            typename = typename std::enable_if<!IsConstSrc && IsConst>::type>
  DenseMapIterator(
      const DenseMapIterator<KeyT, ValueT, ValueInfoT, KeyInfoT, Bucket, IsConstSrc> &I)
      : Ptr(I.Ptr), End(I.End) {}

  reference operator*() const {
    return *Ptr;
  }
  pointer operator->() const {
    return Ptr;
  }

  bool operator==(const ConstIterator &RHS) const {
    return Ptr == RHS.Ptr;
  }
  bool operator!=(const ConstIterator &RHS) const {
    return Ptr != RHS.Ptr;
  }

  inline DenseMapIterator& operator++() {  // Preincrement
    ++Ptr;
    AdvancePastEmptyBuckets();
    return *this;
  }
  DenseMapIterator operator++(int) {  // Postincrement
    DenseMapIterator tmp = *this; ++*this; return tmp;
  }

private:
  void AdvancePastEmptyBuckets() {
    ASSERT(Ptr <= End);
    const KeyT Empty = KeyInfoT::getEmptyKey();
    const KeyT Tombstone = KeyInfoT::getTombstoneKey();

    while (Ptr != End && (KeyInfoT::isEqual(Ptr->getFirst(), Empty) ||
                          KeyInfoT::isEqual(Ptr->getFirst(), Tombstone)))
      ++Ptr;
  }

  void RetreatPastEmptyBuckets() {
    ASSERT(Ptr >= End);
    const KeyT Empty = KeyInfoT::getEmptyKey();
    const KeyT Tombstone = KeyInfoT::getTombstoneKey();

    while (Ptr != End && (KeyInfoT::isEqual(Ptr[-1].getFirst(), Empty) ||
                          KeyInfoT::isEqual(Ptr[-1].getFirst(), Tombstone)))
      --Ptr;
  }
};
```




## 参考链接
**参考链接:🔗**
+ [std::enable_if 的几种用法](https://yixinglu.gitlab.io/enable_if.html)
+ [C++小心得之7： C++11新特性之利用std::conditional实现变量的多类型](https://blog.csdn.net/asbhunan129/article/details/86609897)
