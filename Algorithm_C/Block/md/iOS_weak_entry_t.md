#  iOS_weak_entry_t

> 为了全面透彻的理解 weak 等关键字，现在从最底层的数据结构开始挖掘，构建一个完整的认知体系。

定义位于: `Project Headers/objc-weak.h` P80，此文件只有 144 行，基本所有的内容都是围绕 `struct weak_entry_t` 和 `struct weak_table_t`。

下面要用到的 `weak_referrer_t` 类型定义，可以看到它是一个 `DisguisedPtr` 模版类，且它的 T 是 `objc_object 指针`：

```c++
// The address of a __weak variable.
// __weak 变量的地址
//（意思是 DisguisedPtr 内部放的都是 __weak 变量吗）

// These pointers are stored disguised so memory analysis tools 
// don't see lots of interior pointers from the weak table into objects.
// 这些指针是伪装的，因此内存分析工具看不到从 weak table 到对象的大量内部指针。

typedef DisguisedPtr<objc_object *> weak_referrer_t;
```

`struct weak_entry_t` 定义:
```c++
/**
 * The internal structure stored in the weak references table. 
 * It maintains and stores
 * a hash set of weak references pointing to an object.
 * If out_of_line_ness != REFERRERS_OUT_OF_LINE then the set
 * is instead a small inline array.
 */
#define WEAK_INLINE_COUNT 4

struct weak_entry_t {
    // T 为 objc_object 的模版类 DisguisedPtr，
    // 该成员变量名为 referent，这个名字一定牢记，
    // 后面还有很多地方都会见到
    DisguisedPtr<objc_object> referent;
    
    // 下面是一个 union
    // 
    union {
        struct {
            weak_referrer_t *referrers;
            uintptr_t        out_of_line_ness : 2;
            uintptr_t        num_refs : PTR_MINUS_2;
            uintptr_t        mask;
            uintptr_t        max_hash_displacement;
        };
        struct {
            // out_of_line_ness field is low bits of inline_referrers[1]
            weak_referrer_t  inline_referrers[WEAK_INLINE_COUNT];
        };
    };

    bool out_of_line() {
        return (out_of_line_ness == REFERRERS_OUT_OF_LINE);
    }

    weak_entry_t& operator=(const weak_entry_t& other) {
        memcpy(this, &other, sizeof(other));
        return *this;
    }

    weak_entry_t(objc_object *newReferent, objc_object **newReferrer)
        : referent(newReferent)
    {
        inline_referrers[0] = newReferrer;
        for (int i = 1; i < WEAK_INLINE_COUNT; i++) {
            inline_referrers[i] = nil;
        }
    }
};
```
