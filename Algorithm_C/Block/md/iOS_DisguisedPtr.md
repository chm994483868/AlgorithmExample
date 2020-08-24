#  iOS_DisguisedPtr

> 为了全面透彻的理解 weak 等关键字，现在从最底层的数据结构开始挖掘，力求构建一个完整的认知体系。

## DisguisedPtr
`Project Headers/objc-private.h` Line 904

指针伪装模版类 `Disguised<T>`，与此对应的概念是**指针伪装**。

Disguised<T> 

 根据 `Disguised` 这个英文单词我们或许能猜出一部分信息，`Ptr` 是 `Pointer` （指针）的缩写，硬翻译的话可以理解为：`掩藏指针`，`封装指针`，看它的定义再直白一点话，大概就是指针本身的地址值与 `unsigned long` 来回相互转化。
```
Disguised /dɪs'ɡaɪz/
vt. 假装；掩饰；隐瞒
n. 伪装；假装；用作伪装的东西
```
这个模版类用来封装 nil ptr，让  nil 指针像 non-nil 指针那样正常运行它的操作，而不会让程序崩溃。

```c++
// DisguisedPtr<T> acts like pointer type T*, except the 
// DisguisedPtr<T> 的作用类似指针类型 T*,

// stored value is disguised to hide it from tools like `leaks`.
// 除了将存储的值隐藏起来，使其不受 `leaks` 之类的工具的影响。

// nil is disguised as itself so zero-filled memory works as expected, 
// nil 被伪装成它自己，这样零填充的内存也能如预期那样工作，

// which means 0x80..00 is also disguised as itself but we don't care.
// 意思是 Ox80...00 也伪装成它自己，但是我们不在乎。

// Note that weak_entry_t knows about this encoding.
// 注意 weak_entry_t 知道这种编码。

template <typename T>
class DisguisedPtr {
    // typedef unsigned long uintptr_t;
    uintptr_t value; // 无符号 long 类型的 value 成员变量

    static uintptr_t disguise(T* ptr) { // 指针隐藏
        // 相当于直接把 T 指针的地址转化为 unsigned long 并取负值
        return -(uintptr_t)ptr;
    }

    static T* undisguise(uintptr_t val) { // 指针显示
        // 把 val 转为指针地址，对应上面的 disguise 函数
        return (T*)-val;
    }

 public:
    DisguisedPtr() { } // 构造函数
    
    // 初始化列表，显式初始化 value 成员变量
    DisguisedPtr(T* ptr) : value(disguise(ptr)) { }
    DisguisedPtr(const DisguisedPtr<T>& ptr) : value(ptr.value) { }

    // T* 赋值函数
    DisguisedPtr<T>& operator = (T* rhs) {
        value = disguise(rhs);
        return *this;
    }
    
    // 引用赋值函数
    DisguisedPtr<T>& operator = (const DisguisedPtr<T>& rhs) {
        value = rhs.value;
        return *this;
    }

    // 这大概是重载运算符吗 ？
    operator T* () const {
        return undisguise(value);
    }
    
    T* operator -> () const { 
        return undisguise(value);
    }
    T& operator * () const { 
        return *undisguise(value);
    }
    T& operator [] (size_t i) const {
        return undisguise(value)[i];
    }

    // pointer arithmetic operators omitted 
    // 省略的指针算术运算符
    
    // because we don't currently use them anywhere
    // 因为目前我们不在任何地方使用它
};
```

**参考链接:🔗**
[Object Runtime -- Weak](https://cloud.tencent.com/developer/article/1408976)
[OC Runtime之Weak(2)---weak_entry_t](https://www.jianshu.com/p/045294e1f062)
[iOS 关联对象 - DisguisedPtr](https://www.jianshu.com/p/cce56659791b)
[Objective-C运行时-动态特性](https://zhuanlan.zhihu.com/p/59624358)
