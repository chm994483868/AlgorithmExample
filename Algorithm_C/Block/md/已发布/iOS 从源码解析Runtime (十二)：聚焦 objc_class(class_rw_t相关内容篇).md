# iOS 从源码解析Runtime (十二)：聚焦 objc_class(class_rw_t)
> 上篇我们详细分析了 `class_ro_t`，其中可能有点模糊的是  `class_data_bits_t bits` 的 `data` 函数和 `safe_ro` 函数，其中如果类是处于未实现完成状态时 `data` 函数返回的是 `class_ro_t`，当类实现完成后返回的则是 `class_rw_t`，且当类实现以后 `class_rw_t` 有一个 `ro` 函数来返回 `class_ro_t`，那这是怎么回事呢，这篇我们就来详细分析一下 ⛽️⛽️。

## `class_rw_t`
&emsp;`class_rw_t` 的成员变量。
```c++
struct class_rw_t {
    // Be warned that Symbolication knows the layout of this structure.
    // 警告符号表示知道此结构的布局。
    
    // flats 打印看到是 2148007936 = 2
    // 转为二进制的话是只有 31 位和 19 位是 1，其它位全部都是 0
    uint32_t flags;
    
    // 目击者、证人
    //（控制台打印值为 1）
    uint16_t witness;
    
#if SUPPORT_INDEXED_ISA // isa 中保存 indexcls，大概是 watchOS
    uint16_t index;
#endif

    // std::atomic<uintptr_t>
    // 原子性 unsigned long
    
    // 打印 ro_or_rw_ext 的时候挺奇怪的:
    // (lldb) p $2->ro_or_rw_ext
    // error: no member named 'ro_or_rw_ext' in 'class_rw_t'
    
    // 此值会有两种情况：
    // 1): 值是 class_ro_t *
    // 2): 值是 class_rw_ext_t *，
    //     而 class_ro_t * 作为 class_rw_ext_t 的 const class_ro_t *ro 成员变量保存
    explicit_atomic<uintptr_t> ro_or_rw_ext;

    // 当前所属类的第一个子类
    // 测试时定义了一个继承自 NSObject 的类，
    // 控制台打印看到 firstSubclass 是 nil
    Class firstSubclass;
    
    // 姊妹类、兄弟类
    // 测试时定义了一个继承自 NSObject 的类，
    // 控制台打印看到 nextSiblingClass 是 NSUUID
    Class nextSiblingClass;
    ...
};
```
### `class_rw_ext_t`
```c++
struct class_rw_ext_t {
    // 特别关注 ro 这个成员变量
    // 这个即是在类实现完成后，class_rw_t 中存放的那个 class_ro_t
    const class_ro_t *ro;
    
    // 在上一节 class_ro_t 中的：
    // 方法列表、属性列表、成员变量列表、协议列表：
    // struct method_list_t : entsize_list_tt<method_t, method_list_t, 0x3>
    // struct property_list_t : entsize_list_tt<property_t, property_list_t, 0>
    // struct ivar_list_t : entsize_list_tt<ivar_t, ivar_list_t, 0>
    // struct protocol_list_t 
    
    // 到 class_rw_t 中就变为了:
    // class method_array_t : public list_array_tt<method_t, method_list_t>
    // class property_array_t : public list_array_tt<property_t, property_list_t>
    // class protocol_array_t : public list_array_tt<protocol_ref_t, protocol_list_t>
    
    // 这里先不着急，等下会详细分析它们所使用的新的数据结构: list_array_tt
    
    // 方法列表
    method_array_t methods;
    // 属性列表
    property_array_t properties;
    // 协议列表
    protocol_array_t protocols;
    
    // 所属的类名
    char *demangledName;
    // 版本
    uint32_t version;
};
```
### `class_rw_t private`
&emsp;这里先分析一下 `class_rw_t` 的 `private` 部分。
```c++
struct class_rw_t {
    ...
private:
    // 使用 using 关键字声明一个 ro_or_rw_ext_t 类型:
    // objc::PointerUnion<const class_ro_t *, class_rw_ext_t *>
    //（可理解为一个指针联合体，只有一个指针的内存空间，
    // 一次只能保存 class_ro_t 指针或者 class_rw_ext_t 指针）
    // 此时会发现 class_rw_t 一些端倪了，在 class_ro_t 中它是直接定义不同的成员变量来保存数据，
    // 而在 class_rw_t 中，它大概是用来了一个中间人 class_rw_ext_t 来定义成员变量来保存相关的数据。
    
    // 这里的数据存储根据类是否已经完成实现而分为两种情况：
    // 1): 类未实现完成时，ro_or_rw_ext 中存储的是 class_ro_t *
    // 2): 类已完成实现时，ro_or_rw_ext 中存储的是 class_rw_ext_t *，
    //     而 class_ro_t * 存储在 class_rw_ext_t 的 const class_ro_t *ro 成员变量中
    
    // 类的 class_ro_t 类型的数据是在编译时就产生了。🌿
    
    using ro_or_rw_ext_t = objc::PointerUnion<const class_ro_t *, class_rw_ext_t *>;

    // 根据 ro_or_rw_ext 获得 ro_or_rw_ext_t 类型的变量。（内部可能是 class_ro_t * 或者 class_rw_ext_t *）
    const ro_or_rw_ext_t get_ro_or_rwe() const {
        return ro_or_rw_ext_t{ro_or_rw_ext};
    }
    
    // 以原子方式把入参 const class_ro_t *ro 保存到 ro_or_rw_ext 中
    void set_ro_or_rwe(const class_ro_t *ro) {
        ro_or_rw_ext_t{ro}.storeAt(ro_or_rw_ext, memory_order_relaxed);
    }
    
    // 先把入参 const class_ro_t *ro 赋值给入参 class_rw_ext_t *rwe 的 const class_ro_t *ro;
    // 然后以原子方式把入参 class_rw_ext_t *rwe 保存到 ro_or_rw_ext 中    
    void set_ro_or_rwe(class_rw_ext_t *rwe, const class_ro_t *ro) {
        // the release barrier is so that the class_rw_ext_t::ro initialization is visible to lockless readers.
        // (释放障碍是，无锁阅读器可以看到 class_rw_ext_t::ro 初始化)
        
        rwe->ro = ro;
        ro_or_rw_ext_t{rwe}.storeAt(ro_or_rw_ext, memory_order_release);
    }
    
    // 此处仅有声明（class_rw_ext_t 初始化）
    class_rw_ext_t *extAlloc(const class_ro_t *ro, bool deep = false);
    
    // 定义位于 objc-runtime-new.mm 中：
    class_rw_ext_t *
    class_rw_t::extAlloc(const class_ro_t *ro, bool deepCopy)
    {
        // 加锁
        runtimeLock.assertLocked();
        
        // 
        auto rwe = objc::zalloc<class_rw_ext_t>();

        rwe->version = (ro->flags & RO_META) ? 7 : 0;

        method_list_t *list = ro->baseMethods();
        if (list) {
            if (deepCopy) list = list->duplicate();
            rwe->methods.attachLists(&list, 1);
        }

        // See comments in objc_duplicateClass
        // property lists and protocol lists historically
        // have not been deep-copied
        //
        // This is probably wrong and ought to be fixed some day
        property_list_t *proplist = ro->baseProperties;
        if (proplist) {
            rwe->properties.attachLists(&proplist, 1);
        }

        protocol_list_t *protolist = ro->baseProtocols;
        if (protolist) {
            rwe->protocols.attachLists(&protolist, 1);
        }

        set_ro_or_rwe(rwe, ro);
        return rwe;
    }
    ...
};
```
#### `PointerUnion`
&emsp;这里分析模版类 `objc::PointerUnion` 基于 `objc::PointerUnion<const class_ro_t *, class_rw_ext_t *>` 来进行。`PT1` 是 `const class_ro_t *` （加了 `const`，表示 `class_ro_t` 内容不可被修改）, `PT2` 是 `class_rw_ext_t *`。

```c++
// PT1: const class_ro_t *
// PT2: class_rw_ext_t *

template <class PT1, class PT2>
class PointerUnion {
    // 仅有一个成员变量 _value，
    // 只能保存 const class_ro_t * 或 class_rw_ext_t * 
    uintptr_t _value;

    // 两个断言，PT1 和 PT2 内存对齐
    static_assert(alignof(PT1) >= 2, "alignment requirement");
    static_assert(alignof(PT2) >= 2, "alignment requirement");

    // 定义结构体 IsPT1，内部仅有一个静态不可变 uintptr_t 类型的值为 0 的 Num。
    //（用于 _value 的类型判断, 此时是 class_ro_t *）
    struct IsPT1 {
      static const uintptr_t Num = 0;
    };
    
    // 定义结构体 IsPT2，内部仅有一个静态不可变 uintptr_t 类型的值为 1 的 Num。
    //（用于 _value 的类型判断，此时是 class_rw_ext_t *）
    struct IsPT2 {
      static const uintptr_t Num = 1;
    };
    
    template <typename T> struct UNION_DOESNT_CONTAIN_TYPE {};

    // 把 _value 最后一位置为 0 其它位保持不变的值返回
    uintptr_t getPointer() const {
        return _value & ~1;
    }
    
    // 返回 _value 最后一位的值
    uintptr_t getTag() const {
        return _value & 1;
    }

public:
    // PointerUnion 的构造函数
    // 初始化列表原子操作，初始化 _value
    explicit PointerUnion(const std::atomic<uintptr_t> &raw)
    : _value(raw.load(std::memory_order_relaxed))
    { }
    
    // PT1 正常初始化
    PointerUnion(PT1 t) : _value((uintptr_t)t) { }
    
    // PT2 初始化时把 _value 的最后一位置为 1 
    PointerUnion(PT2 t) : _value((uintptr_t)t | 1) { }

    // 根据指定的 order 以原子方式把 raw 保存到 _value 中
    void storeAt(std::atomic<uintptr_t> &raw, std::memory_order order) const {
        raw.store(_value, order);
    }

    // 极重要的函数，在 class_rw_t 中判断 ro_or_rw_ext 当前是 class_rw_ext_t * 还是 class_ro_t *
    template <typename T>
    bool is() const {
        using Ty = typename PointerUnionTypeSelector<PT1, T, IsPT1, 
        PointerUnionTypeSelector<PT2, T, IsPT2, UNION_DOESNT_CONTAIN_TYPE<T>>>::Return;
        return getTag() == Ty::Num;
    }
    
    // 获取指针 class_ro_t 或者 class_rw_ext_t 指针
    template <typename T> T get() const {
        
      // 确保当前的类型和 T 是匹配的
      ASSERT(is<T>() && "Invalid accessor called");
      
      // getPointer 函数会把 _value 末尾置回 0
      return reinterpret_cast<T>(getPointer());
    }

    // 几乎同上，但是加了一层判断逻辑，
    // get 函数中如果当前 _value 类型和 T 不匹配的话，会返回错误类型的指针
    // dyn_cast 则始终都返回 T 类型的指针
    template <typename T> T dyn_cast() const {
      // 如果 T 和当前实际类型对应，则直接返回
      if (is<T>())
        return get<T>();
      
      // 否则返回 T 类型的值（调用了 T 类型的构造函数）
      return T();
    }
};
```

### `list_array_tt`
&emsp;继续之前先看下一个


## 参考链接
**参考链接:🔗**
+ [iOS之LLDB常用命令](https://juejin.im/post/6869621360415637518)
