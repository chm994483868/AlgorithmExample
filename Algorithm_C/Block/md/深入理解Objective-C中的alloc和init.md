# 深入理解Objective-C中的alloc和init
## runtime 定义

runtime 是 C/C++/汇编 实现的一套 API，目的是为 Objective-C 增加运行时功能。

## 关于 `+ (id)alloc;` 和 `- (id)init;` 底层做了什么
示例代码:
```
HHStaff *staff = [HHStaff alloc];
HHStaff *staff1 = [staff init];
HHStaff *staff2 = [staff init];
NSLog(@"🙇‍♀️🙇‍♀️🙇‍♀️ staff: %p staff1: %p staff2: %p", staff, staff1, staff2);
```
打印结果:
```
🙇‍♀️🙇‍♀️🙇‍♀️ staff: 0x100565c80 staff1: 0x100565c80 staff2: 0x100565c80
```
看到三处对象地址完全一样，那么可以确定的是 `init` 函数没有新创建对象。三个对象指针指向了同一个地址。 

选中 Debug -> Debug Workflow -> Always show Disassembly，然后在 `HHStaff *staff = [HHStaff alloc];` 处打一个断点:

```
// 代码执行到这里看到了 symbol stub for: objc_alloc
>  0x100000ccb <+27>:  movq   0x184e(%rip), %rdi        ; (void *)0x0000000100002780: HHStaff
0x100000cd2 <+34>:  movq   %rax, -0x40(%rbp)
0x100000cd6 <+38>:  callq  0x100000e1c               ; symbol stub for: objc_alloc
```
方法二、符号断点法
1. 在 `HHStaff *staff = [HHStaff alloc];` 添加断点。
2. 添加符号断点，Symbol 里面输入 alloc。
3. 运行程序，定位到如下：
```
// 看到执行到了: _objc_rootAlloc，上面是执行到 objc_alloc，到底是谁先执行的呢？
libobjc.A.dylib`+[NSObject alloc]:
->  0x7fff6de48d7f <+0>:  jmp    0x7fff6de48d93            ; _objc_rootAlloc
```

## alloc 底层调用实现

alloc: Source => NSObject.mm (P2320)
```
+ (id)alloc {
    return _objc_rootAlloc(self);
}
```
_objc_rootAlloc: Source => NSObject.mm (P1718)
```
// Base class implementation of +alloc. cls is not nil.
// Calls [cls allocWithZone:nil].
id
_objc_rootAlloc(Class cls)
{
    return callAlloc(cls, false/*checkNil*/, true/*allocWithZone*/);
}
```
callAlloc: Source => NSObject.mm (P1698)
```
// Call [cls alloc] or [cls allocWithZone:nil], with appropriate 
// shortcutting optimizations.
// 适当的快捷优化
// ALWAYS_INLINE 告诉编译器尽最大可能编译为内联函数...

static ALWAYS_INLINE id
callAlloc(Class cls, bool checkNil, bool allocWithZone=false)
{
#if __OBJC2__
    if (slowpath(checkNil && !cls)) return nil;
    if (fastpath(!cls->ISA()->hasCustomAWZ())) {
        return _objc_rootAllocWithZone(cls, nil);
    }
#endif

    // No shortcuts available.
    if (allocWithZone) {
        return ((id(*)(id, SEL, struct _NSZone *))objc_msgSend)(cls, @selector(allocWithZone:), nil);
    }
    return ((id(*)(id, SEL))objc_msgSend)(cls, @selector(alloc));
}
```
## init 底层实现
同样的方法，`init` 内部调用了 `_objc_rootInit `, 然后在 `_objc_rootInit` 什么都没有做直接 `return obj;`。

init: Source => NSObject.mm (P2333)
```
- (id)init {
    return _objc_rootInit(self);
}
```
_objc_rootInit: Source => NSObject.mm (P1832)
```
id
_objc_rootInit(id obj)
{
    // In practice, it will be hard to rely on this function.
    // Many classes do not properly chain -init calls.
    return obj;
}
```
## new 底层实现
我们经常说 `new` 相当于 `alloc init`，在我们不重写 `init` 函数的情况下，它们确实是等价的。
（其实这个情况更复杂，等下再刨开讲，这里涉及多个函数重写问题）
```
+ (id)new {
    // callAlloc 第三个参数默认是 false
    return [callAlloc(self, false/*checkNil*/) init];
}
```
`[Class new] == [[Class alloc] init]`

### 什么情况下重写 init 函数
init 函数什么时候会用到，重写该方法的时候，一般是分两种情况：
1. 子类仅重写父类的 init 函数，在初始化的时候对成员变量赋默认初始化值。
2. 子类定义 initWithCustom: 传自定义的初始值进来，初始化某些成员变量，且在内部调用 `[super init]`。
### LLVM (编译器)

> 结构体的sizeof（） 长度 用 **补、偏、长**的方式 是一个很好理解和计算的方法。

简单型结构体:
```
struct JEObject_IMPL {
    NSString    *x; // 补0  偏0  长8
    int         a;  // 补0  偏8  长12
    char        y;  // 补0  偏12  长13
    char        t;  // 补0  偏13  长14     14不是8(最长是NSString * 的8位)的倍数 内存对齐为16
};
```
复合型结构体:
```
struct JETemp_IMPL {
    NSString *a;     // 补0  偏0  长8
    int b;           // 补0  偏8  长12      12不是8的倍数  内存对齐为16
};

struct JEObject_IMPL {    
    struct JETemp_IMPL x; // 补0  偏0  长16
    char y;               // 补0  偏16  长17
    char t;               // 补0  偏17  长18
    CGSize  z;            // 补6  偏24  长40     40是8(最长是JETemp_IMPL中NSString * 的8位)的倍数
                           // 这里为什么是补6 ？  因为 CGSize 是结构体，里面最长位是8 按照 8 的倍数来补齐
};
```

## 如何取得对象的内存大小
定义如下 `HHStaff` 类:
```
@interface HHStaff : NSObject {
    // Class isa; 补0 偏0 长8 //在 继承的 NSObject 中还有一个 Class isa; 成员变量
    int _age; // 补0 偏8 长12
    int _height; // 补0 偏12 长16
    NSString *_name; // 补0 偏16 长24
}

HHStaff *staff = [[HHStaff alloc] init];
NSLog(@"🧚‍♂️🧚‍♂️🧚‍♂️ %zu", class_getInstanceSize([staff class]));
NSLog(@"🧚‍♂️🧚‍♂️🧚‍♂️ %lu", sizeof(staff));
// 打印:
🧚‍♂️🧚‍♂️🧚‍♂️ 24
🧚‍♂️🧚‍♂️🧚‍♂️ 8 // 这里是 pinter 在 64 位机器中长度是 8，（32位是4）
```
注意 `sizeof()` 的区别。
问题：创建一个 `HHStaff` 对象:
1. 对象内存占多少？
2. 系统为其分配了多少内存？

这里涉及的对象的本质，`NSObject` 的本质就是一个包含一个指针（这个指针是指向结构体的指针）的结构体。
可以用一个命令将 OC 代码转换成 C：
```
xcrun -sdk iphoneos clang -arch arm64 -rewrite-objc HHStaff.m -o out.cpp

// 失败时可以用：
clang -rewrite-objc main.m
```
在 `main.cpp` 中可以看到 `HHStaff_IMPL` 结构体:

```
struct NSObject_IMPL {
    // typedef struct objc_class *Class; 指向结构体的指针
    Class isa;
};
```
```
struct HHStaff_IMPL {
    struct NSObject_IMPL NSObject_IVARS;
    // Class isa;
    int _age;
    int _height;
    NSString *_name;
};
```
有 3 个函数是获取大小的，但是具体表示的意义不一样:
1. malloc_size()
```
// 系统为其分配的内存大小，传入一个 void* 指针，返回这个指针所指向内存空间的大小
extern size_t malloc_size(const void *ptr);
/* Returns size of given ptr */
```
2. sizeof()
```
// 获取对象所占内存大小
// 这是一个运算符 而不是方法，在编译的时候就是一个确定的数据
```
3. class_getInstanceSize()
```
// 返回类实例的大小，Nil 是返回 0，（返回一个类的底层结构体所占用内存空间的大小）
// 在内存对齐原则下获取类的实例对象的成员变量所占用的大小。
// malloc_size() 返回的才是系统创建实例对象时所分配的实际空间大小。
/** 
 * Returns the size of instances of a class.
 * 
 * @param cls A class object.
 * 
 * @return The size in bytes of instances of the class \e cls, or \c 0 if \e cls is \c Nil.
 */
OBJC_EXPORT size_t
class_getInstanceSize(Class _Nullable cls) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);
```
## alloc 函数执行步骤分析


