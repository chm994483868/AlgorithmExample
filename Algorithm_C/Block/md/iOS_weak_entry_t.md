#  iOS weak_entry_t

> 为了全面透彻的理解 weak 等关键字，现在从最底层的数据结构开始挖掘，构建一个完整的认知体系。

定义位于: `Project Headers/objc-weak.h` P80，此文件只有 144 行，基本所有的内容都是围绕 `struct weak_entry_t` 和 `struct weak_table_t`。

## `weak_referrer_t`
`weak_referrer_t` 定义，可以看到它是一个 `DisguisedPtr` 模版类，且它的 T 是 `objc_object *`：
```c++
// The address of a __weak variable.
// __weak 变量的地址
//（意思是 DisguisedPtr 内部放的都是 __weak 变量吗）

// These pointers are stored disguised so memory analysis tools 
// don't see lots of interior pointers from the weak table into objects.
// 这些指针是伪装的，因此内存分析工具看不到从 weak table 到对象的大量内部指针。

// 这里如果 T 是 objc_object *，那么 DisguisedPtr 里的 T* 就是 objc_object**，即为指针的指针
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
// out_of_line_ness 字段与 inline_referrers[1] 的低两位重叠。

// inline_referrers[1] is a DisguisedPtr of a pointer-aligned address.
// inline_referrers[1] 是指针对齐地址的 DisguisedPtr。

// The low two bits of a pointer-aligned DisguisedPtr will always be 0b00 (disguised nil or 0x80..00) or 0b11 (any other address).
// 指针对齐的DisguisedPtr的低两位将始终为 0b00(如 disguised nil or 0x80..00) 或 0b11（任何其他地址）

// Therefore out_of_line_ness == 0b10 is used to mark the out-of-line state.
// 因此 out_of_line_ness == 0b10 用于标记 out-of-line 状态
#define REFERRERS_OUT_OF_LINE 2 // 这个值是用来标记在 weak_entry_t 中是用那个长度为 4 的小数组存放 weak_referrer_t（__weak 变量），还是用哈希表来存放
```

## `struct weak_entry_t`

下面正式进入 `weak_entry_t` ⛽️

`struct weak_entry_t` 定义:
```c++
struct weak_entry_t {
    // T 为 objc_object 的 DisguisedPtr
    // 那么 Disguised 中存放的就是化身为整数的 objc_object 实例的地址
    // 该成员变量名为 referent，这个名字一定牢记，
    // 后面还有很多地方都会见到
    // 其实它就是那个所有的 __weak 变量指向的原始对象
    DisguisedPtr<objc_object> referent;
    
    union {
        // 两个结构体占用内存都是 32 个字节
        struct {
            weak_referrer_t *referrers; // Disguised<objc_object *> 的指针
            
            // out_of_line_ness 和 num_refs 构成位域存储，共占 64 位，
            uintptr_t        out_of_line_ness : 2;
            uintptr_t        num_refs : PTR_MINUS_2; // PTR_MINUS_2 值为 62
            
            uintptr_t        mask;
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
    
    // 返回 true 表示用哈希表存放 __weak 变量
    // 返回 false 表示用那个长度为 4 的小数组存 __weak 变量
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

**参考链接:🔗**
[Object Runtime -- Weak](https://cloud.tencent.com/developer/article/1408976)
[OC Runtime之Weak(2)---weak_entry_t](https://www.jianshu.com/p/045294e1f062)
[iOS 关联对象 - DisguisedPtr](https://www.jianshu.com/p/cce56659791b)
[Objective-C运行时-动态特性](https://zhuanlan.zhihu.com/p/59624358)
[Objective-C runtime机制(7)——SideTables, SideTable, weak_table, weak_entry_t](https://blog.csdn.net/u013378438/article/details/82790332)
