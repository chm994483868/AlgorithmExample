#  深入理解Objective-C中的Class
> **OC -> C/C++ -> 汇编 -> 机器码**

在 `main.m` 中输入如下，然后在终端中使用如下命令，将 `main.m` 中代码编译成 `C/C++` 代码（在当前文件夹下生成 `main.cpp`）：
```
clang -rewrite-objc main.m -o main.cpp // -o main.cpp 可以忽略
```
在 main.cpp 中发现下边的结构体，从 objc4 库的命名习惯可推断应该是 NSObject 的底层实现:
```
// IMPL 应该是 implementation 的缩写
// NSObject_IMPL => NSObject implementation 
struct NSObject_IMPL {
    Class isa;
};
```
在 **usr/include => objc => NSObject.h** 中看到其声明：
```
OBJC_AVAILABLE(10.0, 2.0, 9.0, 1.0, 2.0)
OBJC_ROOT_CLASS
OBJC_EXPORT
@interface NSObject <NSObject> {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-interface-ivars"
    Class isa  OBJC_ISA_AVAILABILITY;
#pragma clang diagnostic pop
}

// 暂时忽略前缀修饰，以及中间的消除警告的代码
// 以及 NSObject 协议里面的代理方法，以及实例方法和类方法

// 主要看 Class isa; 这个成员变量和上面 NSObject_IMPL 
// 结构体的成员变量如出一辙，这里进一步印证了 NSObject_IMPL 
// 结构体是 NSObject 的底层结构的推断。
@interface NSObject <NSObject> {
    Class isa  OBJC_ISA_AVAILABILITY;
}
```
按住 command 点击 Class 跳转到 **usr/include => objc => objc.h** 
中看到 Class：
```
/// An opaque type that represents an Objective-C class.
typedef struct objc_class *Class;
```
Class 是 `objc_class` 结构体指针，即 isa 实际上是一个指向 `objc_class` 的结构体的指针。
`objc_class` 是 Class 的底层结构。









**参考链接:**
[深入理解 Objective-C ☞ Class](https://www.jianshu.com/p/241e8be676a9?utm_campaign=maleskine&utm_content=note&utm_medium=reader_share&utm_source=weixin)
