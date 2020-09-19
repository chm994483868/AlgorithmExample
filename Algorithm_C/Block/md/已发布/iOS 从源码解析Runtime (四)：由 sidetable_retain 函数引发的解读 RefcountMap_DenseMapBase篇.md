# iOS 从源码解析Runtime (四)：由 sidetable_retain 函数引发的解读 RefcountMap_DenseMapBase篇
## 前言
&emsp;上一节我们从上到下分析了 `DenseMap` 的内容，中间已经涉及到多处 `DenseMapBase` 的使用。`DenseMap` 是 `DenseMapBase` 的子类，而 `DenseMapBase` 是 `DenseMap` 的友元类，所以两者存在多处交织调用。那下面我们就详细分析下 `DenseMapBase` 的实现吧。（这个类实在是太长了，消耗了太多时间，一度想只看下核心实现就不看细枝末节了，但是一想到它涉及到的引用计数以及修饰符相关的内容，再加上强迫症，那就认真看下去吧！⛽️⛽️）

## `DenseMapBase`
&emsp;一个拥有 `6` 个抽象参数的模版类。在 `DenseMap` 定义中，`DenseMapBase` 的第一个抽象参数 `DerivedT`（派生类型、衍生类型） 传的是 `DenseMap<KeyT, ValueT, ValueInfoT, KeyInfoT, BucketT>` 本身， 从它的名字里面我们大概可猜到一些信息，需要的是一个它的子类，那接下来的分析中我们潜意识里面就把 `DerivedT`  默认当作 `DenseMap` 来用。
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
// 为了内存大小，我们允许进行重新哈希化和表插入以删除零值，就像它是一个逻辑删除一样。

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
/// 如果 T 是指向 X 的指针，则返回指向 const X 的指针。如果不是，则返回 const T

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
&emsp;下面是给多个类型起别名，这里一定要谨记 `key_type` 表示的是 `KeyT`，在 `RefcountMap` 定义时用的是 `DisguisedPtr<objc_object>`，  `mapped_type` 表示的是 `ValueT`，在 `RefcountMap` 定义时用的是 `size_t`，而 `value_type` 表示的是 `BucketT`，就是 `DenseMap` 默认的模版参数 `typename BucketT = detail::DenseMapPair<KeyT, ValueT>` 即 `RefcountMap` 特化时的 `detail::DenseMapPair<DisguisedPtr<objc_object>, size_t>`。

**（所以进行到这里，我们大概需要把 `RefcountMap` 理解为一张 `Key` 是 `DisguisedPtr<objc_object>`，`Value` 是 `DenseMapPair<DisguisedPtr<objc_object>, size_t>` 的哈希表。）**

+ `key_type` = `KeyT` = `DisguisedPtr<objc_object>`
+ `mapped_type` = `ValueT` = `size_t`
+ `value_type` = `BucketT` = `detail::DenseMapPair<KeyT, ValueT>` = `detail::DenseMapPair<DisguisedPtr<objc_object>, size_t>`。
```c++
using size_type = unsigned;
using key_type = KeyT;
using mapped_type = ValueT;
using value_type = BucketT;

// 迭代器（默认 IsConst = false，表示 BucketT 可变）
using iterator = DenseMapIterator<KeyT, ValueT, ValueInfoT, KeyInfoT, BucketT>;
// 迭代器（指定 IsConst = true，表示 BucketT 不可变）
using const_iterator = DenseMapIterator<KeyT, ValueT, ValueInfoT, KeyInfoT, BucketT, true>;
```
&emsp;下面我们来分析出现的新类型 `DenseMapIterator`。

### `DenseMapIterator`
&emsp;看到 `DenseMapIterator` 的模版参数几乎和 `DenseMap` 如出一辙，唯一不同的是多了最后一个 `IsConst` 来表示可变和不可变，这里默认值是 `false`。
```c++
// 在 DenseMapBase 上面的一个前向声明，因为 DenseMapBase 中要使用到这个类型，
// 而 DenseMapIterator 的实现是在很下面。
template <
    typename KeyT, typename ValueT,
    typename ValueInfoT = DenseMapValueInfo<ValueT>,
    typename KeyInfoT = DenseMapInfo<KeyT>,
    typename Bucket = detail::DenseMapPair<KeyT, ValueT>,
    bool IsConst = false>
class DenseMapIterator;

template <typename KeyT, typename ValueT, typename ValueInfoT,
          typename KeyInfoT, typename Bucket, bool IsConst>
class DenseMapIterator {

  // 定义两个友元类  IsConst 分别是: true/false
  friend class DenseMapIterator<KeyT, ValueT, ValueInfoT, KeyInfoT, Bucket, true>;
  friend class DenseMapIterator<KeyT, ValueT, ValueInfoT, KeyInfoT, Bucket, false>;

  // 使用 using 声明一个类型别名，IsConst 是 true，来表示不可变的迭代器 ConstIterator
  using ConstIterator = DenseMapIterator<KeyT, ValueT, ValueInfoT, KeyInfoT, Bucket, true>;

public:
  // 下面是一组使用 using 给类型起名字，方便使用
  using difference_type = ptrdiff_t;
  // 这里使用 std::conditional 来表明类型，当 IsConst 为真时 value_type 是 const Bucket
  // 当 IsConst 为假时 value_type 是 Bucket。
  using value_type = typename std::conditional<IsConst, const Bucket, Bucket>::type;
  
  // value_type 的指针和引用
  using pointer = value_type *;
  using reference = value_type &;
  
  // 正向迭代器 forward_iterator：可多次读写，支持输入输出迭代器的所有操作
  using iterator_category = std::forward_iterator_tag;

private:
  // 两个私有成员变量（实质为 BucketT 类型指针） 
  //（BucketT 数组的起点和终点）
  pointer Ptr = nullptr;
  pointer End = nullptr;

public:
  // 默认构造函数
  DenseMapIterator() = default;
  // 构造函数，初始化成员列表
  DenseMapIterator(pointer Pos, pointer E,
                   bool NoAdvance = false)
      : Ptr(Pos), End(E) {
    // Pos 可能是一个空桶，
    // 如果 NoAdvance 为真，则 Ptr 指向这个空桶
    if (NoAdvance) return;
    // 前进跳过空桶，保证 Ptr 指向一个非空桶
    AdvancePastEmptyBuckets();
  }

  // Converting ctor from non-const iterators to const iterators. SFINAE'd out
  // for const iterator destinations so it doesn't end up as a user defined copy
  // constructor.
  // 将 ctor 从非常量迭代器转换为常量迭代器。
  // SFINAE 选择了常量迭代器目标，因此它不会最终成为用户定义的复制构造函数。
  //（有点类似复制构造函数）
  template <bool IsConstSrc,
            typename = typename std::enable_if<!IsConstSrc && IsConst>::type>
  DenseMapIterator(
      const DenseMapIterator<KeyT, ValueT, ValueInfoT, KeyInfoT, Bucket, IsConstSrc> &I)
      : Ptr(I.Ptr), End(I.End) {}

  // 解除引用操作
  reference operator*() const {
    return *Ptr;
  }
  // 访问成员操作
  pointer operator->() const {
    return Ptr;
  }

  // 重载操作符 ==
  bool operator==(const ConstIterator &RHS) const {
    return Ptr == RHS.Ptr;
  }
  
  // 重载操作符 !=
  bool operator!=(const ConstIterator &RHS) const {
    return Ptr != RHS.Ptr;
  }

  // 预增量
  inline DenseMapIterator& operator++() {  // Preincrement
    ++Ptr;
    // 如果此时 Ptr 指向空桶，跳过空桶
    AdvancePastEmptyBuckets();
    return *this;
  }
  
  // 后增量
  DenseMapIterator operator++(int) {  // Postincrement
    DenseMapIterator tmp = *this; 
    ++*this;
    return tmp;
  }

private:
  // 前进查找，并跳过空桶
  void AdvancePastEmptyBuckets() {
    // 保证 Ptr 小于等于 End
    ASSERT(Ptr <= End);
    
    // DenseMapInfo<DisguisedPtr<objc_object>> 的 getEmptyKey 函数
    // 和 getTombstoneKey 函数
    // 类型最大值值和类型最大值减 1
    const KeyT Empty = KeyInfoT::getEmptyKey();
    const KeyT Tombstone = KeyInfoT::getTombstoneKey();

    // Ptr->getFirst 是 DisguisedPtr<objc_object>
    // Ptr 是 BucketT 指针
    // BucketT 是 detail::DenseMapPair<Disguised<objc_object>, size_t>
    // first 是 Disguised<objc_object>，second 是 size_t
    while (Ptr != End && (KeyInfoT::isEqual(Ptr->getFirst(), Empty) ||
                          KeyInfoT::isEqual(Ptr->getFirst(), Tombstone)))
      ++Ptr; // 保证 Ptr 指向的是非空桶
  }
  // 后退查找，并跳过空桶
  void RetreatPastEmptyBuckets() {
    ASSERT(Ptr >= End);
    const KeyT Empty = KeyInfoT::getEmptyKey();
    const KeyT Tombstone = KeyInfoT::getTombstoneKey();

    while (Ptr != End && (KeyInfoT::isEqual(Ptr[-1].getFirst(), Empty) ||
                          KeyInfoT::isEqual(Ptr[-1].getFirst(), Tombstone)))
      --Ptr; // 保证 Ptr 指向的是非空桶
  }
};
```
### `begin、end、empty、size`
```c++
// 内联 构建 BucketT 可变的迭代器
inline iterator begin() {
  // When the map is empty, avoid the overhead of advancing/retreating past empty buckets.
  // 当 map 为空时，避免进行 advancing/retreating 去 past 一些空桶。
  if (empty())
    // 如果为空直接返回末尾
    return end();
    
  // 分别传入 buckets 起点和终点  
  return makeIterator(getBuckets(), getBucketsEnd());
}
// 内联 构建 BucketT 不可变，指向迭代器末尾
inline iterator end() {
  return makeIterator(getBucketsEnd(), getBucketsEnd(), true);
}

// 内联 构建 BucketT 不可变迭代器
inline const_iterator begin() const {
  if (empty())
    return end();
  return makeConstIterator(getBuckets(), getBucketsEnd());
}
// 内联 迭代器末尾 (BucketT 不可变)
inline const_iterator end() const {
  return makeConstIterator(getBucketsEnd(), getBucketsEnd(), true);
}

// 获取 DenseMap 的 NumEntries 判断是否为空
bool empty() const {
  return getNumEntries() == 0;
}

// 获取 DenseMap 的 NumEntries
unsigned size() const { 
  return getNumEntries(); 
}
```
```c++
iterator makeIterator(BucketT *P, BucketT *E,
                      bool NoAdvance=false) {
  return iterator(P, E, NoAdvance);
}

// 对应开局的 using 声明类型，IsConst 的值默认为 true
// 迭代器（指定 IsConst = true，表示 BucketT 不可变）
// using const_iterator = DenseMapIterator<KeyT, ValueT, ValueInfoT, KeyInfoT, BucketT, true>;
const_iterator makeConstIterator(const BucketT *P, const BucketT *E,
                                 const bool NoAdvance=false) const {
  return const_iterator(P, E, NoAdvance);
}
```
### `reserve`
```c++
/// Grow the densemap so that it can contain at least
/// NumEntries items before resizing again.
/// 增加 densemap 容量，使其在重新调整大小之前至少可以包含 NumEntries 项。
void reserve(size_type NumEntries) {
  // 返回大于（NumEntries * 4 / 3 + 1）的最小的 2 的幂
  auto NumBuckets = getMinBucketToReserveForEntries(NumEntries);
  
  // 判断是否需要扩容
  if (NumBuckets > getNumBuckets())
    grow(NumBuckets);
}
```
&emsp;`getMinBucketToReserveForEntries` 和 `grow` 函数都在上篇详细分析过，这里就不再展开了。
### `clear`
```c++
void clear() {
  // 如果 NumEntries 和 NumTombstones 都为 0，表示是初始状态，可以直接 return
  if (getNumEntries() == 0 && getNumTombstones() == 0) return;

  // If the capacity of the array is huge, and the # elements used is small, 
  // shrink the array.
  // 如果此时 map 的容量很大，但是使用容量占比却很小，则缩小它的容量，提高查询和插入时的效率。
  
  // 如果当前使用的容量占比小于总容量的 1/4，并且总容量大于最小容量（4），则缩小其容量
  if (getNumEntries() * 4 < getNumBuckets() && getNumBuckets() > MIN_BUCKETS) {
    // 可参考上篇，有详细分析
    shrink_and_clear();
    return;
  }
  
  // 取得当前的 EmptyKey 和 TombstoneKey 
  const KeyT EmptyKey = getEmptyKey(), TombstoneKey = getTombstoneKey();
  
  // C++ is_trivially_copyable: 测试类型是否是完全复制的类型。
  
  // 判断 Disguised<objc_object> 和 size_t 是否是完全复制类型
  if (is_trivially_copyable<KeyT>::value &&
      is_trivially_copyable<ValueT>::value) {
      
    // Use a simpler loop when these are trivial types.
    // 如果是完全可复制类型时使用一个简单的循环。
    
    for (BucketT *P = getBuckets(), *E = getBucketsEnd(); P != E; ++P)
      // 循环把 BucketT 的 first 置为 EmptyKey
      P->getFirst() = EmptyKey;
      
  } else {
    unsigned NumEntries = getNumEntries();
  for (BucketT *P = getBuckets(), *E = getBucketsEnd(); P != E; ++P) {
      if (!KeyInfoT::isEqual(P->getFirst(), EmptyKey)) {
        if (!KeyInfoT::isEqual(P->getFirst(), TombstoneKey)) {
          // 循环析构 BucketT 的 second 
          P->getSecond().~ValueT();
          // 自减
          --NumEntries;
        }
        // 把 BucketT 的 first 置为 EmptyKey
        P->getFirst() = EmptyKey;
      }
    }
    ASSERT(NumEntries == 0 && "Node count imbalance!");
  }
  // 置为 0
  setNumEntries(0);
  setNumTombstones(0);
}
```
### `count`
```c++
/// Return 1 if the specified key is in the map, 0 otherwise.
/// 如果指定的 key 在 map 中，则返回1，否则返回0。
size_type count(const_arg_type_t<KeyT> Val) const {
  const BucketT *TheBucket;
  // 二次探查法，从哈希表中找到指定 key 对应的 value 
  // LookupBucketFor 函数上篇已分析
  return LookupBucketFor(Val, TheBucket) ? 1 : 0;
}
```
### `find`
```c++
// 可变
iterator find(const_arg_type_t<KeyT> Val) {
  BucketT *TheBucket;
  if (LookupBucketFor(Val, TheBucket))
    return makeIterator(TheBucket, getBucketsEnd(), true);
  return end();
}
// 不可变
const_iterator find(const_arg_type_t<KeyT> Val) const {
  const BucketT *TheBucket;
  if (LookupBucketFor(Val, TheBucket))
    return makeConstIterator(TheBucket, getBucketsEnd(), true);
  return end();
}
```
&emsp;如果从 `map` 中找到了 `val` 对应的 `BucketT`，则返回以此 `BucketT` 为起点的迭代器，否则返回 `end()`。
### `find_as`
```c++
/// Alternate version of find() which allows a different, and possibly less expensive, key type.
/// find（）的替代版本，它允许使用其他且可能 less expensive 的 key 类型。

/// The DenseMapInfo is responsible for supplying methods getHashValue(LookupKeyT)
/// and isEqual(LookupKeyT, KeyT) for each key type used.
/// DenseMapInfo 负责为使用的每种 key 提供方法 getHashValue(LookupKeyT) 和 isEqual(LookupKeyT，KeyT)。

template<class LookupKeyT>
iterator find_as(const LookupKeyT &Val) {
  BucketT *TheBucket;
  if (LookupBucketFor(Val, TheBucket))
    return makeIterator(TheBucket, getBucketsEnd(), true);
  return end();
}
template<class LookupKeyT>
const_iterator find_as(const LookupKeyT &Val) const {
  const BucketT *TheBucket;
  if (LookupBucketFor(Val, TheBucket))
    return makeConstIterator(TheBucket, getBucketsEnd(), true);
  return end();
}
```
### `lookup`
```c++
/// lookup - Return the entry for the specified key,
/// or a default constructed value if no such entry exists.
/// 返回指定 key 的 entry，如果不存在则返回默认构造值

/// 返回指定 DisguisedPtr<objc_object> 对应的 size_t
ValueT lookup(const_arg_type_t<KeyT> Val) const {
  const BucketT *TheBucket;
  if (LookupBucketFor(Val, TheBucket))
    return TheBucket->getSecond();
  return ValueT();
}
```
### `insert、try_emplace`
```c++
// Inserts key,value pair into the map if the key isn't already in the map.
// 如果 key 在 map 中尚不存在，则插入 key/value 对到 map 中。

// If the key is already in the map, it returns false and doesn't update the value.
// 如果 key 在 map 中已经存在，则返回 false 并且不更新 value 值。

// 这里的返回值是 std::pair<iterator, bool>，first 是 iterator，second 是 bool,
// 一定看清，这里特别重要
// 且可把 iterator 理解是一个指向 BucketT 的指针，
// BucketT 类型是 DenseMapPair<Disguised<objc_object>, size_t>
std::pair<iterator, bool> insert(const std::pair<KeyT, ValueT> &KV) {
  return try_emplace(KV.first, KV.second);
}

// Inserts key,value pair into the map if the key isn't already in the map.
// 如果 key 在 map 中尚不存在，则插入 key/value 对到 map 中。
// If the key is already in the map, it returns false and doesn't update the value.
// 如果 key 在 map 中已经存在，则返回 false 并且不更新 value 值。

std::pair<iterator, bool> insert(std::pair<KeyT, ValueT> &&KV) {

 // 在C++11中，标准库在<utility>中提供了一个有用的函数std::move，std::move并不能移动任何东西，
 // 它唯一的功能是将一个左值强制转化为右值引用，继而可以通过右值引用使用该值，以用于移动语义。
 // 从实现上讲，std::move基本等同于一个类型转换：static_cast<T&&>(lvalue);
 
  return try_emplace(std::move(KV.first), std::move(KV.second));
}

// Inserts key,value pair into the map if the key isn't already in the map.
// 如果 key 在 map 中尚不存在，则插入 key/value 对到 map 中。

// The value is constructed in-place if the key is not in the map, otherwise it is not moved.
// 如果 key 不在 map 中，则该值就地构造，否则不移动。
template <typename... Ts>
std::pair<iterator, bool> try_emplace(KeyT &&Key, Ts &&... Args) {
  BucketT *TheBucket;
  if (LookupBucketFor(Key, TheBucket))
    // 如果 key 在 map 中已经存在
    // 返回 std::pair<iterator, false>，second 是 false
    // iterator 的 ptr 指向该 key 对应的 BucketT（DenseMapPair<Disguised<objc_object, size_t>>）
    return std::make_pair(
             makeIterator(TheBucket, getBucketsEnd(), true),
             false); // Already in map. 已经在 map 中

  // Otherwise, insert the new element.
  // 否则，插入新元素。
  TheBucket =
      InsertIntoBucket(TheBucket, std::move(Key), std::forward<Ts>(Args)...);
  // 构建 std::pair，second 是 true
  return std::make_pair(
           makeIterator(TheBucket, getBucketsEnd(), true),
           true);
}

// 同上，不同的是参数是 const KeyT &Key, Ts &&... Args

// Inserts key,value pair into the map if the key isn't already in the map.
// The value is constructed in-place if the key is not in the map, otherwise
// it is not moved.
template <typename... Ts>
std::pair<iterator, bool> try_emplace(const KeyT &Key, Ts &&... Args) {
  BucketT *TheBucket;
  if (LookupBucketFor(Key, TheBucket))
    return std::make_pair(
             makeIterator(TheBucket, getBucketsEnd(), true),
             false); // Already in map.

  // Otherwise, insert the new element.
  TheBucket = InsertIntoBucket(TheBucket, Key, std::forward<Ts>(Args)...);
  return std::make_pair(
           makeIterator(TheBucket, getBucketsEnd(), true),
           true);
}

/// Alternate version of insert() which allows a different, and possibly less expensive, key type.
/// insert() 的替代版本，它允许使用其他且可能 less expensive 键类型.

/// The DenseMapInfo is responsible for supplying methods
/// getHashValue(LookupKeyT) and isEqual(LookupKeyT, KeyT) for each key type used.
/// DenseMapInfo 负责为使用的每种 key 类型提供方法 getHashValue(LookupKeyT) 和 isEqual(LookupKeyT，KeyT)。

template <typename LookupKeyT>
std::pair<iterator, bool> insert_as(std::pair<KeyT, ValueT> &&KV,
                                    const LookupKeyT &Val) {
  BucketT *TheBucket;
  if (LookupBucketFor(Val, TheBucket))
    // 如果已经存在
    return std::make_pair(
             makeIterator(TheBucket, getBucketsEnd(), *this, true),
             false); // Already in map.

  // Otherwise, insert the new element.
  // 否则，插入新元素。
  TheBucket = InsertIntoBucketWithLookup(TheBucket, std::move(KV.first),
                                         std::move(KV.second), Val);
  return std::make_pair(
           makeIterator(TheBucket, getBucketsEnd(), *this, true),
           true);
}

/// insert - Range insertion of pairs.
/// insert - 对的范围插入。
template<typename InputIt>
void insert(InputIt I, InputIt E) {
  for (; I != E; ++I)
    insert(*I);
}
```
&emsp;在 `C++11` 中，标准库在 `<utility>` 中提供了一个有用的函数 `std::move`，`std::move` 并不能移动任何东西，它唯一的功能是将一个左值强制转化为右值引用，继而可以通过右值引用使用该值，以用于移动语义。从实现上讲，`std::move` 基本等同于一个类型转换：`static_cast<T&&>(lvalue)`;
### `compact`
```c++
// Clear if empty.
// 如果为空则清除
// Shrink if at least 15/16 empty and larger than MIN_COMPACT.
// 如果至少有 15/16 的空白且大于 MIN_COMPACT (4)，则缩小。

void compact() {
  if (getNumEntries() == 0) {
    // 如果 NumEntries 为 0，则收缩并清理
    shrink_and_clear();
  }
  else if (getNumBuckets() / 16 > getNumEntries()  &&
           getNumBuckets() > MIN_COMPACT)
  {
    // 如果总容量除以 16 后还大于 NumEntries 即，当前总容量占比少于 1/16
    // 且总容量大于 MIN_COMPACT (4)
    // 则缩小总容量（缩小总容量，把旧值复制到新空间，释放旧空间）
    grow(getNumEntries() * 2);
  }
}
```
### `erase`
```c++
// 清除指定的 KeyT
bool erase(const KeyT &Val) {
  BucketT *TheBucket;
  // 判断 key 是否在 map 中，如果不存在则 return false
  if (!LookupBucketFor(Val, TheBucket))
    return false; // not in map.

  // 如果在 map 中
  // 析构 BucketT 的 second
  TheBucket->getSecond().~ValueT();
  
  // 把 BucketT 的 first 置为 TombstoneKey
  TheBucket->getFirst() = getTombstoneKey();
  
  // NumEntries 减 1
  decrementNumEntries();
  // NumTombstones 加 1
  incrementNumTombstones();
  // 收缩总容量
  compact();
  // 返回 true
  return true;
}

// 根据入参 iterator 清除指定的 key
// 内容同上
void erase(iterator I) {
  BucketT *TheBucket = &*I;
  TheBucket->getSecond().~ValueT();
  TheBucket->getFirst() = getTombstoneKey();
  decrementNumEntries();
  incrementNumTombstones();
  compact();
}
```
### `FindAndConstruct`
```c++
// const KeyT &Key
value_type& FindAndConstruct(const KeyT &Key) {
  // 临时变量
  BucketT *TheBucket;
  // 如果找到则返回 true，并且赋值给 TheBucket,
  // 如果没有找到则返回 false，并且赋值给 TheBucket 一个空桶的位置
  if (LookupBucketFor(Key, TheBucket))
    return *TheBucket;
  // 构建一个 BucketT，size_t 不赋值
  return *InsertIntoBucket(TheBucket, Key);
}
// 取引用计数
ValueT &operator[](const KeyT &Key) {
  return FindAndConstruct(Key).second;
}

// KeyT &&Key
value_type& FindAndConstruct(KeyT &&Key) {
  BucketT *TheBucket;
  if (LookupBucketFor(Key, TheBucket))
    return *TheBucket;

  return *InsertIntoBucket(TheBucket, std::move(Key));
}
// 取引用计数
ValueT &operator[](KeyT &&Key) {
  return FindAndConstruct(std::move(Key)).second;
}
```
### `isPointerIntoBucketsArray、getPointerIntoBucketsArray`
```c++
/// isPointerIntoBucketsArray - Return true if the specified pointer points
/// somewhere into the DenseMap's array of buckets
/// (i.e. either to a key or value in the DenseMap).
/// 如果指定的指针指向 DenseMap 的存储桶数组中的某个位置（即 DenseMap 中的键或值），则返回 true。
bool isPointerIntoBucketsArray(const void *Ptr) const {
  return Ptr >= getBuckets() && Ptr < getBucketsEnd();
}

/// getPointerIntoBucketsArray() - Return an opaque pointer into the buckets array. 
// In conjunction with the previous method, 
// this can be used to determine whether an insertion caused the DenseMap to reallocate.
/// 将不透明的指针返回到buckets数组中。与前面的方法结合使用，可以用来确定插入是否导致 DenseMap 重新分配。
const void *getPointerIntoBucketsArray() const { 
  return getBuckets(); 
}
```
&emsp;到这里 `DenseMapBase` 的 `public` 部分就都看完了，下面是 `protected` 和 `private` 部分。发现几乎涉及的函数都在上一篇分析过了，这里还是再分析一遍加深记忆。⛽️

```c++
protected:
// protected 开头就是一个默认的构造函数
DenseMapBase() = default;
```

### `destroyAll`
```c++
// 销毁所有 Buckets 数组中的数据（ DenseMapPair<Disguised<objc_object>, size_t> ）
void destroyAll() {
  // 如果 Buckets 为空则直接返回
  if (getNumBuckets() == 0) // Nothing to do.
    return;

  // 遍历析构 BucketT
  const KeyT EmptyKey = getEmptyKey(), TombstoneKey = getTombstoneKey();
  for (BucketT *P = getBuckets(), *E = getBucketsEnd(); P != E; ++P) {
    if (!KeyInfoT::isEqual(P->getFirst(), EmptyKey) &&
        !KeyInfoT::isEqual(P->getFirst(), TombstoneKey))
      P->getSecond().~ValueT();
    P->getFirst().~KeyT();
  }
}
```
### `initEmpty`
```c++
// 初始化空状态
void initEmpty() {
  // NumEntries 置为 0
  setNumEntries(0);
  // NumTombstones 置为 0
  setNumTombstones(0);

  // 断言 初始存储桶容量必须是 2 的幂
  // 2 的幂的二进制最后一位必为 0，只有一位是 1，
  // 如 0b1000 减 1 后是 0b0111 两者与运算的结果必为 0
  ASSERT((getNumBuckets() & (getNumBuckets()-1)) == 0 &&
         "# initial buckets must be a power of two!");
         
  const KeyT EmptyKey = getEmptyKey();
  // 遍历 Buckets 中的 BucketT，把 first 置为 EmptyKey
  for (BucketT *B = getBuckets(), *E = getBucketsEnd(); B != E; ++B)
    ::new (&B->getFirst()) KeyT(EmptyKey);
}
```
### `getMinBucketToReserveForEntries`
```c++
/// Returns the number of buckets to allocate to ensure that the
/// DenseMap can accommodate NumEntries without need to grow().
/// 返回要分配的 buckets 的容量，以确保 DenseMap 可以容纳 NumEntries 而不需要调用 grow() 进行扩容。
unsigned getMinBucketToReserveForEntries(unsigned NumEntries) {
  // Ensure that "NumEntries * 4 < NumBuckets * 3"
  // 确保 NumEntries 小于 NumBuckets 的 3/4
  if (NumEntries == 0)
    return 0;
  
  // +1 is required because of the strict equality.
  // 由于严格的相等性，因此需要 +1。
  
  // For example if NumEntries is 48, we need to return 401.
  // 例如，如果 NumEntries 为48，则需要返回 401。
  
  return NextPowerOf2(NumEntries * 4 / 3 + 1);
}

/// NextPowerOf2 - Returns the next power of two (in 64-bits) 
/// that is strictly greater than A.  Returns zero on overflow.
/// 返回严格大于 A 的 2 的下一个幂（64位）。溢出时返回零。

// 把 A 的二进制位从最高位到最低位全部置为 1，然后再加 1，
// 就是大于 A 的最小的 2 的幂。
//（如果 A 值较小，所有位都是 1 后，后面的移位或操作都是浪费的）

// 如下 A 起始是：32
// 1
// 0x100000
// 0x010000 => 0x110000

// 2
// 0x110000
// 0x001100 => 0x111100

// 4
// 0x111100
// 0x001111 => 0x111111

...

// return 0x111111 + 1 

inline uint64_t NextPowerOf2(uint64_t A) {
  A |= (A >> 1);
  A |= (A >> 2);
  A |= (A >> 4);
  A |= (A >> 8);
  A |= (A >> 16);
  A |= (A >> 32);
  return A + 1;
}
```
### `moveFromOldBuckets`
```c++
void moveFromOldBuckets(BucketT *OldBucketsBegin, BucketT *OldBucketsEnd) {
  // 初始化空状态
  initEmpty();

  // Insert all the old elements.
  // 插入所有旧元素。
  const KeyT EmptyKey = getEmptyKey();
  const KeyT TombstoneKey = getTombstoneKey();
  
  for (BucketT *B = OldBucketsBegin, *E = OldBucketsEnd; B != E; ++B) {
    if (ValueInfoT::isPurgeable(B->getSecond())) {
      // Free the value.
      B->getSecond().~ValueT();
    } else if (!KeyInfoT::isEqual(B->getFirst(), EmptyKey) &&
        !KeyInfoT::isEqual(B->getFirst(), TombstoneKey)) {
      // Insert the key/value into the new table.
      BucketT *DestBucket;
      bool FoundVal = LookupBucketFor(B->getFirst(), DestBucket);
      (void)FoundVal; // silence warning.
      ASSERT(!FoundVal && "Key already in new map?");
      DestBucket->getFirst() = std::move(B->getFirst());
      ::new (&DestBucket->getSecond()) ValueT(std::move(B->getSecond()));
      incrementNumEntries();

      // Free the value.
      B->getSecond().~ValueT();
    }
    B->getFirst().~KeyT();
  }
}
```

### `InsertIntoBucket、InsertIntoBucketWithLookup、InsertIntoBucketImpl`
```c++
template <typename KeyArg, typename... ValueArgs>
BucketT *InsertIntoBucket(BucketT *TheBucket, KeyArg &&Key,
                          ValueArgs &&... Values) {
  // 返回一个 BucketT
  TheBucket = InsertIntoBucketImpl(Key, Key, TheBucket);

  // 更新 first
  TheBucket->getFirst() = std::forward<KeyArg>(Key);
  // 更新 second
  ::new (&TheBucket->getSecond()) ValueT(std::forward<ValueArgs>(Values)...);
  
  return TheBucket;
}

template <typename LookupKeyT>
BucketT *InsertIntoBucketWithLookup(BucketT *TheBucket, KeyT &&Key,
                                    ValueT &&Value, LookupKeyT &Lookup) {
  // 返回一个 BucketT
  TheBucket = InsertIntoBucketImpl(Key, Lookup, TheBucket);
  
  // 更新 first
  TheBucket->getFirst() = std::move(Key);
  // 更新 second
  ::new (&TheBucket->getSecond()) ValueT(std::move(Value));
  
  return TheBucket;
}

template <typename LookupKeyT>
BucketT *InsertIntoBucketImpl(const KeyT &Key, const LookupKeyT &Lookup,
                              BucketT *TheBucket) {
  // If the load of the hash table is more than 3/4, or if fewer than 1/8 
  // of the buckets are empty (meaning that many are filled with tombstones), grow the table.
  // 如果哈希表的负载大于 3/4，或者少于 1/8 的存储桶为空（意思是容量占比超出了限制），对哈希表进行扩容。
  
  // The later case is tricky.  
  // 后一种情况比较棘手。
  // For example, if we had one empty bucket with tons of tombstones, 
  // failing lookups (e.g. for insertion) would have to probe almost the
  // entire table until it found the empty bucket.
  // 例如，如果我们有一个空的桶，里面有很多 tombstones，
  // 那么失败的查找（例如，插入操作）将不得不探查几乎整个表，直到找到空的桶。
  
  // If the table completely filled with tombstones, 
  // no lookup would ever succeed, causing infinite loops in lookup.
  // 如果表完全被 tombstones 填满，则任何查询都不会成功，从而导致查询无限循环。
  
  // 获取当前 entry 数量并加 1
  unsigned NewNumEntries = getNumEntries() + 1;
  // 获取当前总的 Buckets 
  unsigned NumBuckets = getNumBuckets();
  
  // #define LLVM_UNLIKELY slowpath
  // #define LLVM_LIKELY fastpath
  
  if (LLVM_UNLIKELY(NewNumEntries * 4 >= NumBuckets * 3)) {
    // 如果当前占比超过了 3/4，则进行扩容
    this->grow(NumBuckets * 2);
    // 查找当前 map 中是否存在指定的 BucketT
    LookupBucketFor(Lookup, TheBucket);
    // 更新 NumBuckets 当前总容量的值
    NumBuckets = getNumBuckets();
  } else if (LLVM_UNLIKELY(NumBuckets-(NewNumEntries+getNumTombstones()) <=
                           NumBuckets/8)) {
    // // 
    this->grow(NumBuckets);
    LookupBucketFor(Lookup, TheBucket);
  }
  ASSERT(TheBucket);

  // Only update the state after we've grown our bucket space appropriately
  // so that when growing buckets we have self-consistent entry count.
  // 只有在适当增加存储桶空间后才更新状态，这样，在增加 buckets 时，我们拥有一致的条目计数。
  // If we are writing over a tombstone or zero value, remember this.
  // 如果我们要覆盖一个逻辑删除值或零值，请记住这一点。
  
  if (KeyInfoT::isEqual(TheBucket->getFirst(), getEmptyKey())) {
    // Replacing an empty bucket.
    // 更换空桶
    // 增加 NumEntries
    incrementNumEntries();
  } else if (KeyInfoT::isEqual(TheBucket->getFirst(), getTombstoneKey())) {
    // Replacing a tombstone.
    // 更换 tombstone。
    // 增加 NumEntries
    incrementNumEntries();
    // 减少 NumTombstones
    decrementNumTombstones();
  } else {
    // we should be purging a zero. No accounting changes.
    // 我们应该清除零。没有会计变更。
    ASSERT(ValueInfoT::isPurgeable(TheBucket->getSecond()));
    TheBucket->getSecond().~ValueT();
  }

  return TheBucket;
}

```

## 参考链接
**参考链接:🔗**
+ [std::enable_if 的几种用法](https://yixinglu.gitlab.io/enable_if.html)
+ [C++小心得之7： C++11新特性之利用std::conditional实现变量的多类型](https://blog.csdn.net/asbhunan129/article/details/86609897)
+ [STL源码学习系列四： 迭代器(Iterator)](https://blog.csdn.net/qq_34777600/article/details/80427463)
+ [《STL源码剖析》学习之迭代器](https://blog.csdn.net/shudou/article/details/11099931?utm_medium=distribute.pc_relevant_t0.none-task-blog-BlogCommendFromMachineLearnPai2-1.add_param_isCf&depth_1-utm_source=distribute.pc_relevant_t0.none-task-blog-BlogCommendFromMachineLearnPai2-1.add_param_isCf)

