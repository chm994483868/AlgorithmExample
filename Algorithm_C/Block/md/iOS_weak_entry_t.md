#  iOS weak_entry_t

> 定义位于: `Project Headers/objc-weak.h` Line 80，此文件只有 144 行，基本所有的内容都是围绕 `struct weak_entry_t` 和 `struct weak_table_t`。

## `weak_referrer_t`
`weak_referrer_t` 定义，可以看到它是一个 `DisguisedPtr` 模版类，且它的 T 是 `objc_object *`：
```c++
// The address of a __weak variable.
// __weak 变量的地址（指针的指针）

// These pointers are stored disguised so memory analysis tools 
// don't see lots of interior pointers from the weak table into objects.
// 这些指针是伪装的，因此内存分析工具看不到从 weak table 到对象的大量内部指针。

// 这里 T 是 objc_object *，那么 DisguisedPtr 里的 T* 就是 objc_object**，即为指针的指针
typedef DisguisedPtr<objc_object *> weak_referrer_t;
```

## `PTR_MINUS_2`
`PTR_MINUS_2` 宏定义
```c++
#if __LP64__
#define PTR_MINUS_2 62 // 当前是 __LP64__
#else
#define PTR_MINUS_2 30
#endif
```

## `WEAK_INLINE_COUNT`
```c++
/**
 * The internal structure stored in the weak references table. 
 * internal structure(内部结构) 存储在弱引用表中。
 * It maintains and stores a hash set of weak references pointing to an object.
 * 它维护并存储了指向对象的弱引用的哈希表。（对象只有一个，指向该对象的 __weak 变量可以有多个）
 * If out_of_line_ness != REFERRERS_OUT_OF_LINE then the set is instead a small inline array.
 * 如果 out_of_line_ness != REFERRERS_OUT_OF_LINE(值为 2 的宏)的话，这个哈希表用一个长度为 4 的内部数组代替。
 * 数组里存放的和哈希表的 value 值的类型都是上面的 weak_referrer_t 
 */
#define WEAK_INLINE_COUNT 4 // 这个值固定为 4 表示内部小数组的长度是 4
```

## `REFERRERS_OUT_OF_LINE`
```c++
// out_of_line_ness field overlaps with the low two bits of inline_referrers[1].
// out_of_line_ness 字段与 inline_referrers[1] 的低两位内存空间重叠。

// inline_referrers[1] is a DisguisedPtr of a pointer-aligned address.
// inline_referrers[1] 是一个遵循指针对齐的 DisguisedPtr。

// The low two bits of a pointer-aligned DisguisedPtr will always be 0b00 (disguised nil or 0x80..00) or 0b11 (any other address).
// 且指针对齐的 DisguisedPtr 的低两位将始终为 0b00(如 disguised nil or 0x80..00) 或 0b11（任何其他地址）

// Therefore out_of_line_ness == 0b10 is used to mark the out-of-line state.
// 因此我们可以使用 out_of_line_ness == 0b10 用于标记 out-of-line 状态，表示使用的是定长数组还是动态数组。

#define REFERRERS_OUT_OF_LINE 2 
// 这个值是用来标记在 weak_entry_t 中是用那个长度为 4 的定长数组存放 weak_referrer_t（__weak 变量），还是用 hash 数组来存放
```

## `struct weak_entry_t`

下面正式进入 `weak_entry_t` ⛽️
`weak_entry_t` 的结构和 `weak_table_t` 很像，同样是一个 `hash` 表，其存储的元素是 `weak_referrer_t`，实质上是弱引用该对象的指针的指针，即 `objc_object **new_referrer`，通过操作指针的指针，就可以使得 weak 引用的指针在对象析构后，指向 `nil`。

`struct weak_entry_t` 定义:
```c++
struct weak_entry_t {
    // T 为 objc_object 的 DisguisedPtr
    // 那么 Disguised 中存放的就是化身为整数的 objc_object 实例的地址
    // 被 __weak 变量弱引用的对象
    DisguisedPtr<objc_object> referent;
    
    // 引用该对象的 __weak 变量列表
    // 当引用个数小于等于 4 时用 weak_referrer_t  inline_referrers[WEAK_INLINE_COUNT]
    // 大于 4 时用 hash 数组 weak_referrer_t *referrers;
    union {
        // 两个结构体占用内存都是 32 个字节
        struct {
            weak_referrer_t *referrers; // 弱引用该对象的对象指针地址的 hash 数组
            
            // out_of_line_ness 和 num_refs 构成位域存储，共占 64 位，
            uintptr_t        out_of_line_ness : 2; // 标记是否使用动态 hash 数组
            uintptr_t        num_refs : PTR_MINUS_2; // PTR_MINUS_2 值为 62。hash 数组中的元素个数
            
            uintptr_t        mask; // hash 数组长度减 1，会参与 hash 计算
            
            // 可能会发生 hash 冲突的最大次数，用于判断是否出现了逻辑错误（hash 表中的冲突次数绝对不会超过该值）
            uintptr_t        max_hash_displacement;
        };
        struct {
            // out_of_line_ness field is low bits of inline_referrers[1]
            // out_of_line_ness 字段是 inline_referrers[1] 的低位
            // 长度为 4 的 weak_referrer_t（Dsiguised<objc_object *>）数组
            // 其实存放的就是那些 __weak 变量的地址，（__weak 变量实质是指向原始对象类型的指针）
            weak_referrer_t  inline_referrers[WEAK_INLINE_COUNT];
        };
    };
    
    // 返回 true 表示用 hash 数组存放 __weak 变量的指针
    // 返回 false 表示用那个长度为 4 的小数组存 __weak 变量的指针
    bool out_of_line() {
        return (out_of_line_ness == REFERRERS_OUT_OF_LINE);
    }
    
    // 赋值操作，直接使用 memcpy 拷贝内存地址里面的内容，并返回 *this
    weak_entry_t& operator=(const weak_entry_t& other) {
        memcpy(this, &other, sizeof(other));
        return *this;
    }

    // struct weak_entry_t 的构造函数
    // newReferent，是我们的原始对象的指针
    // newReferrer，则是我们的 __weak 变量的指针，即 objc_object 的指针的指针
    //（这里总是觉的说 __weak 变量，好像缺点什么，其实只要谨记它本质也是一个 objc_object 指针就好了）
    // 初始化列表直接把 newReferent 赋值给 referent
    // 此时会调用: DisguisedPtr(T* ptr) : value(disguise(ptr)) { }
    // 调用 disguise 函数把 newReferent 地址转化为一个整数赋值给 value
    weak_entry_t(objc_object *newReferent, objc_object **newReferrer)
        : referent(newReferent)
    {
        // newReferrer 放在数组 0 位，并把其他位置为 nil
        inline_referrers[0] = newReferrer;
        for (int i = 1; i < WEAK_INLINE_COUNT; i++) {
            inline_referrers[i] = nil;
        }
    }
};
```
`weak_entry_t` 的结构比较清晰:
+ `DisguisedPtr<objc_object> referent;` 弱引用对象的地址转为整数并取负。
+ `union` 用两种形式，长度为 4 的固定数组：`weak_referrer_t  inline_referrers[WEAK_INLINE_COUNT];` 和 动态数组 `weak_referrer_t *referrers;` ，这两个数组用来存储弱引用该对象的指针的指针，同样使用 `DisguisedPtry` 的形式存储。当弱引用该对象的 __weak 变量的数目小于等于 `WEAK_INLINE_COUNT` 时，使用 `inline_referrers`，否则使用动态数组，并且把定长数组中的元素都转移到动态数组中，并之后都是使用动态数组存储。
+ `bool out_of_line()` 该方法用来判断当前的 `weak_entry_t` 是使用定长数组还是动态数组。返回 `true` 是动态数组，返回 `false` 是定长数组，而且看到 `out_of_line_ness` 的内存是和 `inline_referrers` 数组的第二个元素低两位内存是重合的（`inline_referrers[1]` 低两位正由此来）。
+ `weak_entry_t& operator=(const weak_entry_t& other)` 赋值函数则直接调用了 `memcpy` 对内存空间直接拷贝。
+ `weak_entry_t(objc_object *newReferent, objc_object **newReferrer)` 构造函数，则把定长数组空位初始化位 `nil`。

**之所以使用定长/动态数组的切换，应该是考虑到某对象弱引用的个数一般不会超过 `WEAK_INLINE_COUNT` 个，这时候使用定长数组不需要动态的申请内存空间，（union 中两个结构体共用 32  个字节内存）而是一次分配一块连续的内存空间，这会得到运行效率上的提升。**

关于 `weak_entry_t` 是使用定长数组还是动态 `hash` 数组:
```c++
bool out_of_line() {
    // #define REFERRERS_OUT_OF_LINE 2
    return (out_of_line_ness == REFERRERS_OUT_OF_LINE);
}
```
根据 `REFERRERS_OUT_OF_LINE` 宏定义上面的注释:
```c++
// out_of_line_ness field overlaps with the low two bits of inline_referrers[1].
// out_of_line_ness 字段与 inline_referrers[1] 的低两位内存空间重叠。

// inline_referrers[1] is a DisguisedPtr of a pointer-aligned address.
// inline_referrers[1] 是一个遵循指针对齐的 DisguisedPtr。

// The low two bits of a pointer-aligned DisguisedPtr will always be 0b00 (disguised nil or 0x80..00) or 0b11 (any other address).
// 且指针对齐的 DisguisedPtr 的低两位将始终为 0b00(如 disguised nil or 0x80..00) 或 0b11（任何其他地址）

// Therefore out_of_line_ness == 0b10 is used to mark the out-of-line state.
// 因此我们可以使用 out_of_line_ness == 0b10 用于标记 out-of-line 状态，表示使用的是定长数组还是动态数组。

#define REFERRERS_OUT_OF_LINE 2 // 二进制表示为 0b10
// 这个值是用来标记在 weak_entry_t 中是用那个长度为 4 的定长数组存放 weak_referrer_t（__weak 变量），还是用 hash 数组来存放
```
关于 `weak_entry_t` 是如何插入元素的，是使用 `append_referrer` 函数，具体解析可以参考 `weak` 原理那篇，这里不再重复。

**参考链接:🔗**
[Object Runtime -- Weak](https://cloud.tencent.com/developer/article/1408976)
[OC Runtime之Weak(2)---weak_entry_t](https://www.jianshu.com/p/045294e1f062)
[iOS 关联对象 - DisguisedPtr](https://www.jianshu.com/p/cce56659791b)
[Objective-C运行时-动态特性](https://zhuanlan.zhihu.com/p/59624358)
[Objective-C runtime机制(7)——SideTables, SideTable, weak_table, weak_entry_t](https://blog.csdn.net/u013378438/article/details/82790332)
