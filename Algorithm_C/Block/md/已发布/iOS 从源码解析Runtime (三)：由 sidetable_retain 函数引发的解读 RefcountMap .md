# iOS 从源码解析Runtime (三)：由 sidetable_retain 函数引发的解读 RefcountMap   

> 在 [iOS weak 底层实现原理(四)：SideTables和SideTable](https://juejin.im/post/6865670937841238023) 已经解读过 `struct SideTable`，但是当时重点放在了 `weak` 相关内容上，由于较少涉及对象的引用计数相关内容，所以没有详细解读其中的 `RefcountMap refcnts`，那么就由本篇来解读。
```c++
// SideTable 定义
struct SideTable {
// 自旋锁（内部已经由互斥锁替换）
spinlock_t slock;

// refcnts 与 weak_table 超级重要
RefcountMap refcnts;
weak_table_t weak_table;

SideTable() {
    memset(&weak_table, 0, sizeof(weak_table));
}
...
};
```
##  `RefcountMap refcnts`
&emsp;`refcnts`（应该是 `reference count` 的缩写）是 `struct SideTable` 的一个成员变量，它作为一张散列表来保存对象的引用计数。`RefcountMap` 类型定义如下:
```c++
// RefcountMap disguises its pointers because we
// don't want the table to act as a root for `leaks`.
// RefcountMap 伪装了它的指针，因为我们不希望该表充当`leaks`的根。
typedef objc::DenseMap<DisguisedPtr<objc_object>,size_t,RefcountMapValuePurgeable> RefcountMap;

// 看到 DenseMap 的超长模版定义，不免有些头皮发麻...
// 下面我们分析的时候就根据它的模版参数的顺序来一个一个解析 ⛽️
template <typename KeyT, typename ValueT,
          typename ValueInfoT = DenseMapValueInfo<ValueT>,
          typename KeyInfoT = DenseMapInfo<KeyT>,
          typename BucketT = detail::DenseMapPair<KeyT, ValueT>>
class DenseMap : public DenseMapBase<DenseMap<KeyT, ValueT, ValueInfoT, KeyInfoT, BucketT>,
                                     KeyT, ValueT, ValueInfoT, KeyInfoT, BucketT> {
                                     ...
                                     ...
};
```
## `DenseMap`
&emsp;只看 `RefcountMap` 的 `typedef` 语句的话，我们可以直白的把 `RefcountMap` 理解为一个 `key` 是我们的对象指针 `value` 是该对象的引用计数的哈希表。（深入下去 `DenseMap` 涉及的数据结构真的超多，为了秉持完成 `runtime` 每行代码都要看的通透，那我们硬着头看下去。）`DenseMap` 是在 `llvm` 中用的非常广泛的数据结构，它本身的实现是一个基于`Quadratic probing`（二次探查）的散列表，键值对本身是 `std::pair<KeyT, ValueT>`。`DenseMap` 有四个成员变量: `Buckets`、`NumEntries`、`NumTombstones`、`NumBuckets` 分别用于表示散列桶的起始地址（一块连续的内存）、已存储的数据的个数、`Tombstone` 个数（二次探查法删除数据时需要设置 `deleted` 标识）、桶的总个数。
&emsp;`DenseMap<>` 继承自 `DenseMapBase<>`，`DenseMapBase` 是 `2012` 年 `Chandler Carruth` 添加的，为了实现 `SmallDenseMap<>`，将 `DenseMap` 的哈希逻辑抽象到了 `DenseMapBase` 中，而内存管理的逻辑留在了 `DenseMap` 和 `SmallDenseMap` 实现。

`DenseMap` 的前三个模版参数是:  
1. `DisguisedPtr<objc_object>` 伪装的 `objc_object` 指针。（实际是把地址值转换为整数，可参考[iOS weak 底层实现原理(一)：DisguisedPtr](https://juejin.im/post/6865468675940417550)）
2. `size_t` 表示引用计数的值。
3. `RefcountMapValuePurgeable` 一个结构体，只定义了一个静态内联函数 `isPurgeable`，入参为 `0` 时返回 `true`，否则返回 `false`。

### `RefcountMapValuePurgeable` 和 `DenseMapValueInfo`
&emsp;`RefcountMapValuePurgeable` 是在 `NSObject.mm` 文件中 `RefcountMap` 上面定义的一个结构体，直接作为了 `DenseMap` 的第三个模版参数，在 `Project Headers/llvm-DenseMapInfo.h` 中 `class DenseMap` 定义中该位置的模版参数是有一个默认值的: `DenseMapValueInfo`，它的内部也是只有一个静态内联函数 `isPurgeable` 但它是默认返回 `false`。
```c++
struct RefcountMapValuePurgeable {
    static inline bool isPurgeable(size_t x) {
        return x == 0;
    }
};

template<typename T>
struct DenseMapValueInfo {
    // 作为 DenseMap 的默认模版参数时 T 的类型是 size_t 
    static inline bool isPurgeable(const T &value) {
        return false;
    }
};
```
### `DenseMapInfo`
&emsp;`typename KeyInfoT = DenseMapInfo<KeyT>`。`DenseMapInfo` 是一个模版结构体，其内部只有四个静态函数，分别用于 `empty key`、`tombstone key` 以及哈希值的计算，它定义在 `Project Headers/llvm-DenseMapInfo.h` 中，该文件只有 `200` 行，文件前面的注释 `This file defines DenseMapInfo traits for DenseMap.` (该文件用来定义 `DenseMap` 的 `DenseMapInfo` 特征，取自 `llvmCore-3425.0.31`，（后期会深入学习 `LLVM`）。) 表明其核心作用，文件下面提供了针对常见类型的 `DenseMapInfo<>` 的特化版本，例如指针类型、整型等，这里我们主要使用 `DenseMapInfo<DisguisedPtr<T>>`。
&emsp;关于模版内部实现，对于 `empty key` 来说，基本上都是该类型所能表示的最大值，`tombstone key` 都是 `empty key` 减 1，哈希的值的计算则都是乘法计算，每个 `hash seed` 都是 `37`。哈希函数中普遍都使用质数作为哈希种子，质数能够有效的避免哈希碰撞的发生，这里选择 `37` 大概是在测试过程中有比较好的性能表现。

```c++
template<typename T>
struct DenseMapInfo {
  //static inline T getEmptyKey();
  //static inline T getTombstoneKey();
  //static unsigned getHashValue(const T &Val);
  //static bool isEqual(const T &LHS, const T &RHS);
};
```
&emsp;下面是针对 `struct DenseMapInfo` 的特化版本：
#### `DenseMapInfo<T*>`
```c++
// Provide DenseMapInfo for all pointers.
// 为所有的指针提供 DenseMapInfo
template<typename T>
struct DenseMapInfo<T*> {
  static inline T* getEmptyKey() {
    // typedef unsigned long uintptr_t; 无符号 long
    // static_cast <type-id>( expression )
    // 把 expression 转换为 type-id 类型，但没有运行时类型检查来保证转换的安全性 
    // reinterpret_cast<type-id> (expression)
    // 操作符修改了操作数类型,但仅仅是重新解释了给出的对象的比特模型而没有进行二进制转换。
    
    // -1 转化为 unsigned long 的最大值 18446744073709551615
    uintptr_t Val = static_cast<uintptr_t>(-1);
    // 把 18446744073709551615 转换为 T 指针
    return reinterpret_cast<T*>(Val);
  }
  static inline T* getTombstoneKey() {
    // -2 转化为 18446744073709551614
    uintptr_t Val = static_cast<uintptr_t>(-2);
    // 把 18446744073709551614 转化为 T 指针
    return reinterpret_cast<T*>(Val);
  }
  static unsigned getHashValue(const T *PtrVal) {
      // 指针哈希函数
      return ptr_hash((uintptr_t)PtrVal);
  }
  // 判断是否相等(T 类型可能重写 ==)
  static bool isEqual(const T *LHS, const T *RHS) { return LHS == RHS; }
};
```
&emsp;`static_cast` 和 `reinterpret_cast` 的区别可参考: [《reinterpret_cast》](https://baike.baidu.com/item/reinterpret_cast/9303204?fr=aladdin)。
#### `DenseMapInfo<DisguisedPtr<T>>`
```c++
// Provide DenseMapInfo for disguised pointers.
// 为伪装的指针提供 DenseMapInfo。
// 基本和 T* 保持相同
template<typename T>
struct DenseMapInfo<DisguisedPtr<T>> {
  static inline DisguisedPtr<T> getEmptyKey() {
    // DisguisedPtr 内部: DisguisedPtr(T* ptr) : value(disguise(ptr)) { }
    // static uintptr_t disguise(T* ptr) { return -(uintptr_t)ptr; }
    // (uintptr_t)-1 是 unsigned long 的最大值
    // 又被 -(uintptr_t)ptr 转化为 1, 即 DisguisedPtr 的 value 值为 1
    
    return DisguisedPtr<T>((T*)(uintptr_t)-1);
  }
  static inline DisguisedPtr<T> getTombstoneKey() {
    // 同上 DisguisedPtr 的 value 值为 2
    return DisguisedPtr<T>((T*)(uintptr_t)-2);
  }
  static unsigned getHashValue(const T *PtrVal) {
      // 指针 hash 函数
      return ptr_hash((uintptr_t)PtrVal);
  }
  static bool isEqual(const DisguisedPtr<T> &LHS, const DisguisedPtr<T> &RHS) {
      // 判等
      return LHS == RHS; 
  }
};
```
#### `DenseMapInfo<const char*>`
```c++
// Provide DenseMapInfo for cstrings.
// 为 cstrings 提供 DenseMapInfo。
template<> struct DenseMapInfo<const char*> {
  static inline const char* getEmptyKey() {
    // typedef __darwin_intptr_t intptr_t;
    // typedef long __darwin_intptr_t;
    // 把 -1 转化为 const char *
    return reinterpret_cast<const char *>((intptr_t)-1); 
  }
  static inline const char* getTombstoneKey() { 
    // 把 -2 转化为 const char *
    return reinterpret_cast<const char *>((intptr_t)-2); 
  }
  static unsigned getHashValue(const char* const &Val) { 
    // 哈希函数，下面解析
    return _objc_strhash(Val); 
  }
  static bool isEqual(const char* const &LHS, const char* const &RHS) {
    if (LHS == RHS) {
      return true;
    }
    
    // 任一值为 getEmptyKey 或 getTombstoneKey 都返回 false
    if (LHS == getEmptyKey() || RHS == getEmptyKey()) {
      return false;
    }
    if (LHS == getTombstoneKey() || RHS == getTombstoneKey()) {
      return false;
    }
    
    // 字符串比较
    return 0 == strcmp(LHS, RHS);
  }
};
```
#### `_objc_strhash`
```c++
static __inline uint32_t _objc_strhash(const char *s) {
    uint32_t hash = 0;
    for (;;) {
    
    // 从 s 起点开始每次读取一个字节的数据
    int a = *s++;
    
    if (0 == a) break;
    // 每次把 hash 的值左移 8 位给 a 留出空间，再加 a
    // 再加 hash
    hash += (hash << 8) + a;
    }
    return hash;
}
```
#### `DenseMapInfo<char>`
&emsp;下面的一组 `DenseMapInfo<unsigned>`、`DenseMapInfo<unsigned long>`、`DenseMapInfo<unsigned long long>`、`DenseMapInfo<int>`、`DenseMapInfo<long>`、`DenseMapInfo<long long>` 几乎都一模一样，`getEmptyKey` 都是取该抽象类型的最大值，`getTombstoneKey` 都是最大值减 1，`getHashValue` 都是乘以 `37`，`isEqual` 函数都是直接直接 `==`。
#### `DenseMapInfo<std::pair<T, U> >`
```c++
// Provide DenseMapInfo for all pairs whose members have info.
// 为成员具有信息的所有配对提供 DenseMapInfo。
template<typename T, typename U>
struct DenseMapInfo<std::pair<T, U> > {
  typedef std::pair<T, U> Pair;
  
  typedef DenseMapInfo<T> FirstInfo;
  typedef DenseMapInfo<U> SecondInfo;

  static inline Pair getEmptyKey() {
    return std::make_pair(FirstInfo::getEmptyKey(),
                          SecondInfo::getEmptyKey());
  }
  static inline Pair getTombstoneKey() {
    return std::make_pair(FirstInfo::getTombstoneKey(),
                          SecondInfo::getTombstoneKey());
  }
  static unsigned getHashValue(const Pair& PairVal) {
  
   // 把 first 的哈希值(32位 int)左移 32 位和 second 的哈希值(32位 int)做或运算，
   // 即把 first 和 second 的哈希值合并到一个 64 位 int 中
    uint64_t key = (uint64_t)FirstInfo::getHashValue(PairVal.first) << 32
          | (uint64_t)SecondInfo::getHashValue(PairVal.second);
    
    // 然后那上面的 64 位 int 做 移位 取反 相加 异或 操作
    key += ~(key << 32);
    key ^= (key >> 22);
    key += ~(key << 13);
    key ^= (key >> 8);
    key += (key << 3);
    key ^= (key >> 15);
    key += ~(key << 27);
    key ^= (key >> 31);
    
    return (unsigned)key;
  }
  static bool isEqual(const Pair &LHS, const Pair &RHS) {
    return FirstInfo::isEqual(LHS.first, RHS.first) &&
           SecondInfo::isEqual(LHS.second, RHS.second);
  }
};
```
#### `std::pair<T, U>`
```c++
template <class _T1, class _T2>
struct _LIBCPP_TEMPLATE_VIS pair {
    typedef _T1 first_type;
    typedef _T2 second_type;

    _T1 first;
    _T2 second;

#if !defined(_LIBCPP_CXX03_LANG)
    pair(pair const&) = default;
    pair(pair&&) = default;
#else
  // Use the implicitly declared copy constructor in C++03
#endif
...
};
```
&emsp;`std::pair` 是一个结构体模板，其可于一个单元内存储两个相异对象，是 `std::tuple` 的拥有两个元素的特殊情况。一般来说，`pair` 可以封装任意类型的对象，可以生成各种不同的 `std::pair<T1, T2>` 对象，可以是数组对象或者包含 `std::pair<T1,T2>` 的 `vector` 容器。`pair` 还可以封装两个序列容器或两个序列容器的指针。具体细节可参考：[STL std::pair基本用法](https://www.cnblogs.com/phillee/p/12099504.html)

### `DenseMapPair`
&emsp;第五个模版参数。
```c++
// We extend a pair to allow users to override the bucket
// type with their own implementation without requiring two members.
// 我们扩展了 pair，允许用户使用自己的实现覆盖存储桶类型，而无需两个成员

// 公开继承自 std::pair
template <typename KeyT, typename ValueT>
struct DenseMapPair : public std::pair<KeyT, ValueT> {

  // FIXME: Switch to inheriting constructors when we drop support for older
  // clang versions.
  // 当我们放弃对较旧的 clang 版本的支持时，请切换到继承构造函数。
  // NOTE: This default constructor is declared with '{}' rather than
  // '= default' to work around a separate bug in clang-3.8. 
  // This can also go when we switch to inheriting constructors.
  // 此默认构造函数使用 '{}' 而不是 '= default' 声明，以解决 clang-3.8 中的一个单独的错误。
  // 当我们切换到继承构造函数时，这也可以进行。
  DenseMapPair() {}

  // 初始化列表内使用 Key 和 Value 初始化 std::pair
  DenseMapPair(const KeyT &Key, const ValueT &Value)
      : std::pair<KeyT, ValueT>(Key, Value) {}

  // KeyT && ValueT && 通用引用
  // 初始化列表初始化 std::pair
  DenseMapPair(KeyT &&Key, ValueT &&Value)
      : std::pair<KeyT, ValueT>(std::move(Key), std::move(Value)) {}

  // 实现调用函数去推导正确的模板函数版本
  // 以下两个函数大概都是保证模版特化正常...
  template <typename AltKeyT, typename AltValueT>
  DenseMapPair(AltKeyT &&AltKey, AltValueT &&AltValue,
               typename std::enable_if<
                   std::is_convertible<AltKeyT, KeyT>::value &&
                   std::is_convertible<AltValueT, ValueT>::value>::type * = 0)
      : std::pair<KeyT, ValueT>(std::forward<AltKeyT>(AltKey),
                                std::forward<AltValueT>(AltValue)) {}

  template <typename AltPairT>
  DenseMapPair(AltPairT &&AltPair,
               typename std::enable_if<std::is_convertible<
                   AltPairT, std::pair<KeyT, ValueT>>::value>::type * = 0)
      : std::pair<KeyT, ValueT>(std::forward<AltPairT>(AltPair)) {}

  // 返回 first 的引用
  KeyT &getFirst() { return std::pair<KeyT, ValueT>::first; }
  
  // 返回 const first 的引用
  const KeyT &getFirst() const { return std::pair<KeyT, ValueT>::first; }
  
  // 返回 second 引用
  ValueT &getSecond() { return std::pair<KeyT, ValueT>::second; }
  const ValueT &getSecond() const { return std::pair<KeyT, ValueT>::second; }
};
```
&emsp;`std::move` 右值引用可具体参考：[C++右值引用（std::move）](https://zhuanlan.zhihu.com/p/94588204)。涉及到大量 `C++ 11` 相关的内容，但总体还是继承 `std::pair` 新建 `struct DenseMapPair` 方便我们使用 `std::pair` 特性。

下面分析 `DenseMap` 的代码实现：
&emsp;`DenseMap` 的内存管理，主要是通过 `operator new` 分配内存，通过 `operator delete` 释放内存。
```c++
template <typename KeyT, typename ValueT,
          typename ValueInfoT = DenseMapValueInfo<ValueT>,
          typename KeyInfoT = DenseMapInfo<KeyT>,
          typename BucketT = detail::DenseMapPair<KeyT, ValueT>>
class DenseMap : public DenseMapBase<DenseMap<KeyT, ValueT, ValueInfoT, KeyInfoT, BucketT>,
                                     KeyT, ValueT, ValueInfoT, KeyInfoT, BucketT> {
                                       
  // 友元类，DenseMapBase 能访问 DenseMap 的私有成员变量
  friend class DenseMapBase<DenseMap, KeyT, ValueT, ValueInfoT, KeyInfoT, BucketT>;

  // Lift some types from the dependent base class into this class for
  // simplicity of referring to them.
  // 为了简化引用，将某些类型从依赖基类提升到此类。
  using BaseT = DenseMapBase<DenseMap, KeyT, ValueT, ValueInfoT, KeyInfoT, BucketT>;
                                       
  // 散列桶的起始地址（一块连续的内存）
  BucketT *Buckets;
  // 已存储的数据的个数
  unsigned NumEntries;
  // Tombstone 个数（二次探查法删除数据时需要设置 deleted 标识）
  unsigned NumTombstones;
  // 桶的总个数
  unsigned NumBuckets;

public:
  /// Create a DenseMap wth an optional \p InitialReserve that guarantee that
  /// this number of elements can be inserted in the map without grow()
  /// 使用可选的 InitialReserve 创建一个 DenseMap，
  /// 以确保可以将这些数量的元素插入到 map 中，而无需调用 grow（）
  explicit DenseMap(unsigned InitialReserve = 0) { init(InitialReserve); }

  DenseMap(const DenseMap &other) : BaseT() {
    init(0);
    copyFrom(other);
  }

  DenseMap(DenseMap &&other) : BaseT() {
    init(0);
    swap(other);
  }

  template<typename InputIt>
  DenseMap(const InputIt &I, const InputIt &E) {
    init(std::distance(I, E));
    this->insert(I, E);
  }

  DenseMap(std::initializer_list<typename BaseT::value_type> Vals) {
    init(Vals.size());
    this->insert(Vals.begin(), Vals.end());
  }

  ~DenseMap() {
    // 销毁 KeyT ValueT
    this->destroyAll();
    operator delete(Buckets);
  }

  // 交换
  void swap(DenseMap& RHS) {
    std::swap(Buckets, RHS.Buckets);
    std::swap(NumEntries, RHS.NumEntries);
    std::swap(NumTombstones, RHS.NumTombstones);
    std::swap(NumBuckets, RHS.NumBuckets);
  }
                                       
  // 重载赋值操作符
  DenseMap& operator=(const DenseMap& other) {
    if (&other != this)
      copyFrom(other);
    return *this;
  }

  DenseMap& operator=(DenseMap &&other) {
    this->destroyAll();
    operator delete(Buckets);
    init(0);
    swap(other);
    return *this;
  }

  void copyFrom(const DenseMap& other) {
    // 销毁旧值
    this->destroyAll();
    // 释放内存
    operator delete(Buckets);
    
    // copy 新值
    if (allocateBuckets(other.NumBuckets)) {
      this->BaseT::copyFrom(other);
    } else {
      NumEntries = 0;
      NumTombstones = 0;
    }
  }

  // DenseMap 的初始化分为如下三步：
  // 针对初始元素数，计算初始最小桶的数量
  // 针对桶的个数，分配内存
  // 初始化
  
  // 由于 DenseMap 对桶的数量有两个标准：
  // 桶的数量必须是2次幂
  // 如果 DenseMap 的 load factor > 3/4 或者空桶数量 < 1/8，
  // 则说明需要增加桶的数量
  
  // 为了满足这两个标准，getMinBucketToReserveForEntries() 首先将元素数量 * 4/3，
  // 然后计算大于元素数量 * 4/3 的最小的 2 次幂，计算 2 次幂的方法为 NextPowerOf2()。
  
  // 为桶分配内存的方法是 allocateBuckets()，
  // 该方法就是调用 operator new() 分配一块堆内存，用于存放数据。
  // 最后是信息的初始化，初始化空桶的方法是 initEmpty()。
  
  void init(unsigned InitNumEntries) {
    // 获取需要分配的桶数
    auto InitBuckets = BaseT::getMinBucketToReserveForEntries(InitNumEntries);
    // new(sizeof(BucketT) * NumBuckets) 申请空间，如果成功返回 true
    if (allocateBuckets(InitBuckets)) {
      // 执行 DenseMapBase 的 initEmpty 函数
      this->BaseT::initEmpty();
    } else {
      NumEntries = 0;
      NumTombstones = 0;
    }
  }

  // 增长
  // DenseMap 在初始化阶段，会进行初始桶数量的计算，桶的分配，以及empty key的初始化。
  // 当桶的数量不够时，标准是 load factor > 3/4 或者空桶数量 < 1/8，说明需要分配新的桶来存储数据。
  // 为 DenseMap 增加桶数量的方法是 grow()。
  // 增长过程和 std::vector 很相似，分为计算新的桶数量并分配内存，拷贝数据，释放旧的桶。
  // 计算桶的数量同样使用的是 NextPowerOf2() 方法。
  
  void grow(unsigned AtLeast) {
    unsigned OldNumBuckets = NumBuckets;
    BucketT *OldBuckets = Buckets;

    // 计算新的桶数量并分配内存
    allocateBuckets(std::max<unsigned>(MIN_BUCKETS, static_cast<unsigned>(NextPowerOf2(AtLeast-1))));
    ASSERT(Buckets);
    if (!OldBuckets) {
      this->BaseT::initEmpty();
      return;
    }

    // 拷贝数据
    this->moveFromOldBuckets(OldBuckets, OldBuckets+OldNumBuckets);

    // Free the old table.
    // 释放旧的桶
    operator delete(OldBuckets);
  }

  // 清理
  // 清理操作是由 shrink_and_clear() 方法实现的，主要是将重新分配一块内存，
  // 然后进行初始化，然后将原有的内存释放，类似于容器中的 clear() 方法。
  void shrink_and_clear() {
    unsigned OldNumEntries = NumEntries;
    this->destroyAll();

    // Reduce the number of buckets.
    // 减少桶的数量。
    unsigned NewNumBuckets = 0;
    if (OldNumEntries)
      NewNumBuckets = std::max(MIN_BUCKETS, 1 << (Log2_32_Ceil(OldNumEntries) + 1));
    
    if (NewNumBuckets == NumBuckets) {
      // 初始化
      this->BaseT::initEmpty();
      return;
    }
    
    // 释放旧数据
    operator delete(Buckets);
    // 重新分配一块内存，然后进行初始化
    init(NewNumBuckets);
  }

private:
  unsigned getNumEntries() const {
    return NumEntries;
  }

  void setNumEntries(unsigned Num) {
    NumEntries = Num;
  }

  unsigned getNumTombstones() const {
    return NumTombstones;
  }

  void setNumTombstones(unsigned Num) {
    NumTombstones = Num;
  }

  BucketT *getBuckets() const {
    return Buckets;
  }

  unsigned getNumBuckets() const {
    return NumBuckets;
  }

  bool allocateBuckets(unsigned Num) {
    NumBuckets = Num;
    if (NumBuckets == 0) {
      Buckets = nullptr;
      return false;
    }
    // 申请 sizeof(BucketT) * NumBuckets 个字节的空间
    Buckets = static_cast<BucketT*>(operator new(sizeof(BucketT) * NumBuckets));
    return true;
  }
};
```
## `DenseMapBase` 
&emsp;由于 `DenseMapBase` 篇幅过于庞大，我们这里只分析与我们关系最紧密的查找、插入和删除 部分。


## 参考链接
**参考链接:🔗**
+ [llvm中的数据结构及内存分配策略 - DenseMap](https://blog.csdn.net/dashuniuniu/article/details/80043852)
+ [构造哈希表之二次探测法](https://blog.csdn.net/xyzbaihaiping/article/details/51607770)
+ [Objective-C 引用计数原理](http://yulingtianxia.com/blog/2015/12/06/The-Principle-of-Refenrence-Counting/)
+ [C++语法之友元函数、友元类](https://ityongzhen.github.io/C++语法之友元函数、友元类.html/#more)
+ [static_cast](https://baike.baidu.com/item/static_cast/4472966?fr=aladdin)
+ [reinterpret_cast](https://baike.baidu.com/item/reinterpret_cast/9303204?fr=aladdin)
+ [浅谈std::forward](https://zhuanlan.zhihu.com/p/92486757)
+ [C++11 std::move和std::forward](https://www.jianshu.com/p/b90d1091a4ff)
+ [实现 std::is_convertible](https://zhuanlan.zhihu.com/p/98384465)
