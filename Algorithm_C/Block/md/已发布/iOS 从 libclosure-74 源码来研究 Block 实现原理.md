# iOS 从 libclosure-74 源码来研究 Block 实现原理

# Blocks

> 本篇主要介绍 `OS X Snow Leopard(10.6)` 和 `iOS 4` 引入的 `C` 语言扩充功能 `Blocks`，究竟是如何扩充 `C` 语言的，扩充之后又有哪些优点呢？下面通过其实现来一步一步探究。

## 什么是 Blocks 
&emsp;`Blocks` 是 `C` 语言的扩充功能。可以用一句话来表示 `Blocks` 的扩充功能：带有自动变量（局部变量）的匿名函数。(对于程序员而言，命名就是工作的本质。)
何谓匿名，下面的示例代码可解释：
```c++
typedef void(^Blk_T)(int);
typedef int(^Ret_T)(void);

int i = 10;
// 注意等号右边的 block 定义(匿名)
Blk_T a = ^void (int event) {
    printf("i = %d event = %d\n", i, event);
};

// 等号右边的 block 省略了返回值类型
Ret_T b = ^{
    return 5;
};

// 函数定义
void Func(int event) {
    printf("buttonID: %d event = %d\n", i, event);
};
void (*funcPtr)(int) = &Func;
```
&emsp;匿名是针对有名而言的，如上代码 `Blk_T a` 等号后面的 `block` 定义是不需要取名的，而下面的 `Func` 函数定义必须给它一个函数名。
> `block` 可以省略返回值类型的，省略返回值类型时，如果表达式中有 `return` 语句就使用该返回值的类型，如果表达式没有 `return` 语句就使用 `void` 类型。表达式中含有多个 `return` 语句时，所有的 `return` 的返回值类型必须相同。
（以前一直忽略了 `block` 是可以省略返回值类型的，以为是直到 `swift` 的闭包才支持省略返回值类型。）

完整形式的 `block` 语法与一般的 `C` 语言函数定义相比，仅有两点不同：

1. 没有函数名。
2. 带有 `^`。

`block` 定义范式如下:
`^ 返回值类型 参数列表 表达式`
“返回值类型” 同 `C` 语言函数的返回值类型，“参数列表” 同 `C` 语言函数的参数列表，“表达式” 同 `C` 语言函数中允许使用的表达式。

&emsp;在 `block` 语法下，可将 `block` 语法赋值给声明为 `block` 类型的变量中。即源代码中一旦使用 `block` 语法就相当于生成了可赋值给 `block` 类型变量的 “值”。`Blocks` 中由 `Block` 语法生成的值也称为 `block`。`block` 既指源代码中的 `block` 语法，也指由 `block` 语法所生成的值。
使用 `block` 语法将 `block` 赋值为 `block` 类型变量。
```c++
int (^blk)(int) = ^(int count) { return count + 1; };

// 返回值是 block 类型的函数
// func() 和 { } 之外是描述返回类型的
void (^ func() )(int) {
    return ^(int count) { return count + 1; }; 
}
```
`block` 类型变量可完全像通常的 `C` 语言变量一样使用，因此也可以使用指向 `block` 类型变量的指针，即 `block` 的指针类型变量。
```c++
typedef int (^blk_t)(int);
blk_t blk = ^(int count) { return count + 1; };

// 指针赋值
blk_t* blkPtr = &blk;

// 执行 block
(*blkPrt)(10);
```
`block` 类型变量与一般的 `C` 语言变量完全相同，可作为以下用途使用：
+ 自动变量
+ 函数参数
+ 静态变量
+ 静态全局变量
+ 全局变量

通过 `block` 类型变量调用 `block` 与 `C` 语言通常的函数调用没有区别。

## 截获外部变量值
&emsp;`block` 是带有自动变量（局部变量）的匿名函数，**带有自动变量** 在 `block` 中表现为 **截获外部变量值**。
```c++
// 示例 🌰 1：
int val = 10;
const char* fmt = "val = %d\n";
void (^blk)(void) = ^{
    // block 内截获的是 10 和 fmt 指针指向的地址
    printf(fmt, val);
};

// blk 只是截获了 val 的瞬间值(10)去初始化 block 结构体的 val 成员变量，
// val 的值无论再怎么改写，都与 block 结构体内的值再无瓜葛
val = 2;

// 修改了 fmt 指针的指向，blk 对应 block 结构体只是截获了 fmt 指针原始指向的 char 字符串，
// 所以 blk 内打印使用的还是 "val = %d\n"
fmt = "These values were changed. val = %d\n";

blk();

// 打印结果：
// val = 10

// 示例 🌰 2：
int temp = 10;
int* val = &temp;
const char* fmt = "val = %d\n";
void (^blk)(void) = ^{
    // block 内截获的是 fmt 指针指向的地址以及 val 指针指向的地址
    printf(fmt, *val);
};

// 直接改写 val 指向的地址内的值，blk 内截获的是 val 指向的地址
*val = 20; 
fmt = "These values were changed. val = %d\n";

blk();
// 打印结果：
// val = 20

// 示例 🌰 3：
int temp = 10;
int* val = &temp;
const char* fmt = "val = %d\n";
void (^blk)(void) = ^{
    // 这里可以直接通过指针修改 val 的值
    *val = 22;
    printf(fmt, *val);
};

// 直接改写 val 指向的地址内的值，blk 截获的 val 指向的地址
*val = 20; 

fmt = "These values were changed. val = %d\n";

blk();
// 打印结果：
// val = 22

// 示例 🌰 4：
__block int val = 10;
const char* fmt = "val = %d\n";
void (^blk)(void) = ^{
    printf(fmt, val);
};

// val 用 __block 修饰后，类型已经不是 int，它已转变为结构体类型，具体细节会在下面展开
// blk 内部持有的也是 val 的地址，
// 这里也代表着修改了内存地址里面存放的值，
// 所以 blk 执行时，读出来的也是这个 2
val = 2;
fmt = "These values were changed. val = %d\n";

blk();
// 打印结果：
// val = 2

// 示例 🌰 5:
int a = 10;
__block int b = a;
void (^blk)(void) = ^{
    NSLog(@"⛈⛈⛈ block 内部 b 修改前: b = %d", b);
    b = 20; // 修改后外部 b 也是 20
};

a = 30; // 此处 a 修改了与 b 无关，b 还是原始值
NSLog(@"⛈⛈⛈ b = %d", b);

blk();

NSLog(@"⛈⛈⛈ b = %d", b);
NSLog(@"⛈⛈⛈ a = %d", a);
// 打印结果：
⛈⛈⛈ b = 10
⛈⛈⛈ block 内部 b 修改前: b = 10
⛈⛈⛈ b = 20
⛈⛈⛈ a = 30

// 示例 🌰 6:
// 从上到下操作的一直都是 a 变量地址空间里的内容
int a = 10;
__block int* b = &a;
void (^blk)(void) = ^{
    NSLog(@"⛈⛈⛈ block 内部 b 修改前: b = %d", *b);
    *b = 20; // 修改后外部 b 也是 20
};

NSLog(@"⛈⛈⛈ b = %d", *b);
a = 30;
NSLog(@"⛈⛈⛈ b = %d", *b);

blk();

NSLog(@"⛈⛈⛈ b = %d", *b);
NSLog(@"⛈⛈⛈ a = %d", a);
// 打印结果:
⛈⛈⛈ b = 10
⛈⛈⛈ b = 30
⛈⛈⛈ block 内部 b 修改前: b = 30
⛈⛈⛈ b = 20
⛈⛈⛈ a = 20
```

> **无论 `block` 定义在哪，啥时候执行。当 `block` 执行时，用的值都是它之前截获（可以理解为拿外部变量赋值给 `block` 结构体的成员变量）的基本变量或者是截获的内存地址，如果是内存地址的话，从定义到执行这段时间，不管里面保存的值有没有被修改了， `block` 执行时，都是读出来的当时内存里保存的值。** 

### `__block` 说明符
&emsp;`block` 截获外部变量值，截获的是 `block` 语法定义时此外部变量瞬间的值，保存后就不能改写该值。这个不能改写该值是 `block` 的语法规定（实际在 `block` 花括号内使用的值早已不再是外部的同名的值，虽然它们的变量名完全一样），如果截获的是指针变量的话，可以通过指针来修改内存空间里面的值。比如传入 `NSMutableArray` 变量，可以往里面添加对象，但是不能对该 `NSMutableArray` 变量进行赋值。传入 `int* val` 也可以直接用 `*val = 20` 来修改 `val` 指针指向的内存里面保存的值，并且如果截获的是指针变量的话，在 `block` 内部修改其指向内存里面的内容后，在 `block` 外部读取该指针指向的值时也与 `block` 内部的修改都是同步的。**因为本身它们操作的就是同一块内存地址**。
这里之所以语法定为不能修改，可能的原因是因为修改了值以后是无法传出去的，只是在 `block` 内部使用，是没有意义的。就比如 `block` 定义里面截获了变量 `val`，你看着这时用的是 `val` 这个变量，其实只是把 `val` 变量的值赋值给了 `block` 结构体的 `val` 成员变量。这时在 `block` 内部修改 `val` 的值，可以理解为只是修改 `block` 结构体 `val` 成员变量的值，与 `block` 外部的 `val` 已经完全无瓜葛了，然后截获指针变量也是一样的，其实截获的只是指针变量所指向的地址，在 `block` 内部修改的只是 `block` 结构体成员变量的指向，这种修改针对外部变量而言都是毫无瓜葛的。
```c++
// 示例 🌰：
int dmy = 256;
int temp = 10;
int* val = &temp;

printf("🎉🎉 val 初始值：= %d\n", *val);

const char* fmt = "🎉 Block 内部：val = %d\n";
void (^blk)(void) = ^{
    printf(fmt, *val);
    int temp2 = 30;
    // !!!!!!!!! 这里报错 
    // Variable is not assignable (missing __block type specifier)
    val = &temp2;
    *val = 22;
};

*val = 20; // 修改 val
fmt = "These values were changed. val = %d\n";

blk();

printf("🎉🎉 val = %d\n", *val); // block 执行时把 *val 修改为 22
// 运行结果：
// 🎉🎉 val 初始值：= 10
// 🎉 Block 内部：val = 20
// 🎉🎉 val = 22
```
以上不能修改（或者理解为为其赋值）时，可以用 `__block` 说明符来修饰该变量，该变量称为 `__block` 变量。
> 注意：在 `block` 内部不能使用 `C` 语言数组，这是因为现在的 `block` 截获外部变量的方法并没有实现对 `C` 语言数组的截获，实质是因为 `C` 语言规定，数组不能直接赋值，可用 `char*` 代替。
```c++
const char text[] = "Hello"; 
void (^blk)(void) = ^{ 
  // Cannot refer to declaration with an array type inside block 
  // 这是因为现在的 Blocks 截获自动变量的方法并没有实现对 C 语言数组的截获。
  // 实质是因为 C 语言规定，数组不能直接赋值，可用 char* 代替
  printf("%c\n", text[0]);
}; 
```

## block 的实质
&emsp;`block` 是 “带有自动变量的匿名函数”，但 `block` 究竟是什么呢？语法看上去很特别，但它实际上是作为 **极普通的 `C` 语言源码** 来处理的。通过 **支持 `block` 的编译器**，含有 `block` 语法的源代码转换为一般 `C` 语言编译器能够处理的源代码，并作为极为普通的 `C` 语言源代码被编译。
&emsp;这不过是概念上的问题，在实际编译时无法转换成我们能够理解的源代码，但 `clang(LLVM 编译器)` 具有转换为我们可读源代码的功能。通过 `-rewrite-objc` 选项就能将含有 `block` 语法的源代码转换为 `C++` 的源代码。说是 `C++`，其实也 **仅仅是使用了 `struct` 结构，其本质是 `C` 语言源代码**。

`clang -rewrite-objc 源代码文件名`，如下源代码通过 `clang` 可变换为: 
```c++
int main() {
    void (^blk)(void) = ^{ printf("Block\n"); };
    blk();

    return 0;
}
```
+ **__block_impl**
 ```c++
 struct __block_impl {
   void *isa;
   int Flags;
   int Reserved;
   void *FuncPtr;
 };
 ```
 + **__main_block_impl_0**
 ```c++
 struct __main_block_impl_0 {
   struct __block_impl impl;
   struct __main_block_desc_0* Desc;
   
   // 结构体构造函数 
   __main_block_impl_0(void *fp, struct __main_block_desc_0 *desc, int flags=0) {
     impl.isa = &_NSConcreteStackBlock;
     impl.Flags = flags;
     impl.FuncPtr = fp;
     Desc = desc;
   }
 };
 ```
 + **__main_block_func_0**
 ```c++
 static void __main_block_func_0(struct __main_block_impl_0 *__cself) {
     printf("Block\n");
 }
 ```
 + **__main_block_desc_0**
 ```c++
 static struct __main_block_desc_0 {
   size_t reserved;
   size_t Block_size;
 } __main_block_desc_0_DATA = { 0, sizeof(struct __main_block_impl_0)};
 ```
+ **main 函数内部**
```c++
int main(int argc, const char * argv[]) {
    /* @autoreleasepool */ { __AtAutoreleasePool __autoreleasepool; 
    NSLog((NSString *)&__NSConstantStringImpl__var_folders_24_5w9yv8jx63bgfg69gvgclmm40000gn_T_main_948e6f_mi_0);
        
    // 首先是等号左边是一个返回值和参数都是 void 的函数指针: void (*blk)(void)，
    // 等号右边去掉 &(取地址符) 前面的强制类型转换后，可看到后面是创建了一个，
    // __main_block_impl_0 结构体实例，所以此处可以理解为在栈上创建了一个 Block 结构体实例，
    // 并把它的地址转化为了一个函数指针。
    void (*blk)(void) = ((void (*)())&__main_block_impl_0((void *)__main_block_func_0, &__main_block_desc_0_DATA));
    
    // 取出 __block_impl 里面的 FuncPtr 函数执行。
    // __main_block_func_0 函数的参数是类型是 struct __main_block_impl_0 指针，
    // 但是这里用了 __block_impl * 做强制类型转换，
    // 是因为 struct __main_block_impl_0 的第一个成员变量是 struct __block_impl impl，
    // 地址起始空间的类型是一致的（本例暂时没有用到 __cself）
    ((void (*)(__block_impl *))((__block_impl *)blk)->FuncPtr)((__block_impl *)blk);
    }

    return 0;
}

// 生成 Block 结构体（struct __main_block_impl_0）去掉转换部分可以理解为：
// 第一个参数是由 Block 语法转换的 C 语言函数指针
// 第二个参数是作为静态全局变量初始化的 __main_block_desc_0 结构体实例指针
struct __main__block_impl_0 tmp = __main_block_impl_0(__main_block_func_0, &__main_block_desc_0_DATA);
struct __main_block_impl_0 *blk = &tmp;

// 该源代码将 __main_block_impl_0 结构体类型的自动变量，即栈上生成的 __main_block_impl_0 结构体实例的指针，赋值 __main_block_impl_0 结构体指针类型的变量 blk。

void (^blk)(void) = ^{printf("Block\n");};

// 将 Block 语法生成的 Block 赋给 Block 类型变量 blk。
// 它等同于将 __main_block_impl_0 结构体实例的指针赋给变量 blk。
// 源代码中的 Block 就是 __main_block_impl_0 结构体类型的自动变量，
// 即栈上生成的 __main_block_impl_0 结构体实例。

// 执行 Block 去掉转换部分可以理解为：
(*blk->impl.FuncPtr)(blk);
// 参数 __cself 即是 Block
```
&emsp;分析下上面转换出的相关结构体，`struct __block_impl` 名字中的 `impl` 即 `implementation` 的缩写，换句话说这一部分是 `block` 的实现部分结构体，`void *isa` `C` 语言中 `void *` 为 “不确定类型指针”，`void *` 可以用来声明指针。看到 `isa` 就会联想到 `objc_class` 结构体，因此我们的 `block` 本质上也是一个对象，而且是个类对象，我们知道 **实例对象->类对象->元类** 构成了 `isa` 链中的一条，而这个 `__block_impl` 结构体占据的是中间类对象的位置，实例对象应该是生成的 `block` 变量，个人认为因此这里的 `isa` 指针会指向元类，这里的元类主要是为了说明这个 `block` 的存储区域，`int Flags` 标识符，在实现 `block` 的内部操作时会用到 `int Reserved` 注明今后版本升级所需区域大小，`Reserved` 一般就是填个 `0`。`void *FuncPtr` 函数指针 **实际执行的函数，也就是 `block` 定义中花括号里面的代码内容，最后是转化成一个 `C` 语言函数执行的**。

&emsp;如变换后的源代码所示，通过 `block` 使用的匿名函数实际上 **被作为简单的 C 语言函数来处理**( `__main_block_func_0` 函数)。另外，**根据 `block` 语法所属的函数名（此处为 `main`）和该 `block` 语法在该函数出现的顺序值（此处为 `0`）来给经 `clang` 变换的函数命名**。该函数的参数 `__cself` 相当于 `C++` 实例方法中指向实例自身的变量 `this`，或是 `Objective-c` 实例方法中指向对象自身的变量 `self`，即参数 `__cself` 为指向 `block` 值的变量。（`__main_block_impl_0` 实例）

> `static void __main_block_func_0(struct __main_block_impl_0* __cself)` 与 `C++` 的 `this` 和 `OC` 的 `self` 相同，参数 `__cself` 是 `__main_block_impl_0` 结构体的指针。

> `isa = &_NSConcreteStackBlock` 将 `block` 指针赋给 `block` 的结构体成员变量 `isa`。为了理解它，首先要理解 `OC` 类和对象的实质。其实，所谓 `block` 就是 `OC` 对象。

```c++
// 如果把 __main_block_impl_0 展开的话，
// 把 struct __block_impl impl 的成员变量直接展开，

// 已经几乎和 OC 对象相同
struct __main_block_impl_0 {
void* isa; // isa 是类的实例对象指向所属类的指针
int Flags; // 后面是类定义中添加的成员变量
int Reserved;
void* FuncPtr;

struct __main_block_desc_0* Desc;
};

// OC 中的实例对象和类对象所使用的数据结构 struct objc_object 和 struct objc_class
typedef struct objc_object* id;
typedef struct objc_class* Class;

// objc_object 结构体和 objc_class 结构体归根结底是在各个对象和类的实现中使用的最基本的数据结构。
```

## `block` 截获外部变量值的实质

&emsp;上一节为了观察 `block` 的最原始的形态在 `block` 中没有截获任何变量，下面我们看一下 `block` 截获外部变量时的样子。通过 `clang -rewrite-objc` 转换如下 `block` 定义：
```c++
int dmy = 256; // 此变量是为了对比，未使用的变量不会被 block 截获
int val = 10;
int* valPtr = &val;
const char* fmt = "val = %d\n";

void (^blk)(void) = ^{
    // block 截获了三个变量，类型分别是: int、int *、const char *
    printf(fmt, val); // 此处是 val 变量的原始值 10
    printf("valPtr = %d\n", *valPtr); // val 变量当前内存空间里的数据
};

// val 修改为 2，valPtr 指针也跟着指为 2（直接修改了 valPtr 所指的内存区域内的数据）， 
// blk 内部调用是 *valPtr 也是 2，
// Block 的结构体实例中的 valPtr 成员变量，初始值传入的就是外部的 valPtr 指针，
// 所以它们两者指向的内存地址是一样的。

val = 2;
fmt = "These values were changed. val = %d\n";

blk();

// 打印结果:
val = 10
valPtr = 2
```
转换后的代码:
`__block_impl` 结构不变：
```c++
struct __block_impl {
  void *isa;
  int Flags;
  int Reserved;
  void *FuncPtr;
};
```
`__main_block_impl_0` 成员变量增加了，`block` 语法表达式中使用的外部变量（看似，其实只是同名）被作为成员变量追加到了 `__main_block_impl_0` 结构体中，且类型与外部变量完全相同。`__main_block_impl_0` 构造函数具体内容就是对 `impl` 中相应的内容进行赋值，要说明的是 `impl.isa = &_NSConcreteStackBlock` 这个是指 `block` 的存储域 和 当前 `block` 的元类，被 `block` 截获的外部变量值被放入到该结构体的成员变量中，构造函数也发生了变化，初始化列表内要给 `fmt`、`val`、`valPtr` 赋值，这里我们就能大概猜出截获外部变量的原理了，被使用的外部变量值会被存入 `block` 结构体中，而在 `block` 表达式中看似是使用外部变量其实是使用了一个名字一模一样的 `block` 结构体实例的成员变量，所以我们不能对它进行赋值操作，看似操作的是外部变量值，其实是 `block` 结构体实例的成员变量。
```c++
struct __main_block_impl_0 {
  struct __block_impl impl;
  struct __main_block_desc_0* Desc;
  
  // Block 截获三个外部变量，然后增加了自己对应的成员变量，
  // 且和外部的自动变量的类型是完全一致的，
  //（这里加深记忆，后面学习 __block 变量被转化为结构体时可与其进行比较）
  const char *fmt;
  int val;
  int *valPtr;
  
  // 初始化列表里面 : fmt(_fmt), val(_val), valPtr(_valPtr)
  // 构造结构体实例时会用截获的外部变量的值进行初始化，看到参数类型也与外部变量完全相同
  __main_block_impl_0(void *fp,
                      struct __main_block_desc_0 *desc,
                      const char *_fmt,
                      int _val,
                      int *_valPtr,
                      int flags=0) : fmt(_fmt), val(_val), valPtr(_valPtr) {
    impl.isa = &_NSConcreteStackBlock;
    impl.Flags = flags;
    impl.FuncPtr = fp;
    Desc = desc;
  }
};
```
`__main_block_func_0` 函数内也使用到了 `__cself` 参数：
```c++
static void __main_block_func_0(struct __main_block_impl_0 *__cself) {

    // 可以看到通过函数传入 __main_block_impl_0 实例读取对应的截获的外部变量的值 
    const char *fmt = __cself->fmt; // bound by copy
    int val = __cself->val; // bound by copy
    int *valPtr = __cself->valPtr; // bound by copy

    printf(fmt, val);
    printf("valPtr = %d\n", *valPtr);
}
```
`__main_block_desc_0` 保持不变：
```c++
static struct __main_block_desc_0 {
  size_t reserved;
  size_t Block_size;
} __main_block_desc_0_DATA = { 0, sizeof(struct __main_block_impl_0)};
```
`main` 函数里面，`__main_block_impl_0` 结构体实例构建和 `__main_block_func_0` 函数执行保持不变：
```c++
int main(int argc, const char * argv[]) {
    /* @autoreleasepool */ { __AtAutoreleasePool __autoreleasepool; 

        NSLog((NSString *)&__NSConstantStringImpl__var_folders_24_5w9yv8jx63bgfg69gvgclmm40000gn_T_main_4ea116_mi_0);

        int dmy = 256;
        int val = 10;
        int* valPtr = &val;
        const char* fmt = "val = %d\n";
        
        // 根据传递给构造函数的参数对 struct __main_block_impl_0 中由自动变量追加的成员变量进行初始化
        void (*blk)(void) = ((void (*)())&__main_block_impl_0((void *)__main_block_func_0, &__main_block_desc_0_DATA, fmt, val, valPtr));

        val = 2;
        fmt = "These values were changed. val = %d\n";
        
        // 执行 __block_impl 中的 FuncPtr 函数，入参正是 __main_block_impl_0 实例变量 blk
        ((void (*)(__block_impl *))((__block_impl *)blk)->FuncPtr)((__block_impl *)blk);
    }

    return 0;
}
```
**总的来说，所谓 “截获外部变量值” 意味着在执行 `block` 语法时，`block` 语法表达式使用的与外部变量同名的变量其实是 `block` 的结构体实例（即 `block` 自身）的成员变量，而这些成员变量的初始化值则来自于截获的外部变量的值。** 这里前面提到的 `Block` 不能直接使用 `C` 语言数组类型的自动变量，如前所述，截获外部变量时，将值传递给结构体的构造函数进行保存，如果传入的是 `C` 数组，假设是 `a[10]`，那构造函数内部发生的赋值是 `int b[10] = a` 这是 `C` 语言规范所不允许的，`block` 是完全遵循 `C` 语言规范的。

### `__block` 说明符的实质
回顾前面截获外部变量值的例子：
```c++
// block 定义
^{ printf(fmt, val); };

// 转换后是:
static void __main_block_func_0(struct __main_block_impl_0* __cself) {
    const char* fmt = __cself->fmt;
    int val = __cself->val;

    printf(fmt, val);
}
```
&emsp;看完转换后的源码，`block` 中所使用的被截获外部变量就如 “带有自动变量值的匿名函数” 所说，仅截获外部变量的值。在 `block` 的结构体实例中重写该成员变量也不会改变原先截获的外部变量。当试图在 `block ` 表达式内部改变同名于外部变量的成员变量时，会发生编译错误。因为在实现上不能改写被截获外部变量的值，所以当编译器在编译过程中检出给被截获外部变量赋值的操作时，便产生编译错误。理论上 `block` 内部的成员变量已经和外部变量完全无瓜葛了，理论上 `block` 结构体的成员变量是能修改的，但是这里修改的仅是结构体自己的成员变量，且又和外部完全同名，如果修改了内部成员变量开发者会误以为连带外部变量一起修改了，索性直接发生编译错误更好！（而 `__block` 变量就是为了在 `block` 表达式内修改外部变量而生的）。

在 `block` 表达式中修改外部变量的办法有两种，（这里忽略上面很多例子中出现的直接传递指针来修改变量的值）：
1. `C` 语言中有变量类型允许 `block` 改写值:
 + 静态变量
 + 静态全局变量
 + 全局变量
 
 虽然 `block` 语法的匿名函数部分简单转换为了 `C`  语言函数，但从这个变换的函数中访问 **静态全局变量/全局变量** 并没有任何改变，可直接使用。**但是静态局部变量的情况下，转换后的函数原本就设置在含有 `block` 语法的函数之外，所以无法从变量作用域直接访问静态局部变量。在我们用 `clang -rewrite-objc` 转换的 `C++` 代码中可以清楚的看到静态局部变量定义在 `main` 函数内，而 `static void __main_block_func_0(struct __main_block_impl_0 *__cself){ ... }` 则是完全在外部定义的一个静态函数。**
 
 **这里的静态变量的访问，作用域之外，应该深入思考下，虽然代码写在了一起，但是转换后并不在同一个作用域内，能跨作用域访问数据只能靠指针了。**
 
 代码验证:
 ```c++
 int global_val = 1; // 全局变量
 static int static_global_val = 2; // 静态全局变量
 
 int main(int argc, const char * argv[]) {
 @autoreleasepool {
     // insert code here...
     
     // 这里如果静态局部变量是指针类型的话，
     // 那么在 block 结构体中会被转化为指向指针的指针，
     // 例如: NSMutableArray **static_val;
     
     static int static_val = 3; // 静态局部变量
     
     // 这里看似 block 表达式和 static_val 是同一个作用域的，
     // 其实它们两个完全不是同一作用域的
     void (^blk)(void) = ^{
     
        // 直接在 block 内修改两种不同的类型的外部变量
        global_val *= 2;
        static_global_val *= 2;
        
        // 静态变量则是通过指针来修改的
        static_val *= 3;
     };
     
     static_val = 12;
     blk();
                
     // static_val = 111;
     printf("static_val = %d, global_val = %d, static_global_val = %d\n", static_val, global_val, static_global_val);
 }
}
// 打印结果:
// static_val = 36, global_val = 2, static_global_val = 4

// 看到 static_val 是 36， 即 blk 执行前 static_val 修改为了 12
// 然后 blk 执行时 static_val = 12 * 3 => static_val = 36
// block 内部可以修改 static_val 且 static_val 外部的修改也会
// 传递到 blk 内部
 ```
 clang 转换后的源代码:
 `__main_block_impl_0` 追加了 `static_val` 指针为成员变量:
 ```c++
 struct __main_block_impl_0 {
   struct __block_impl impl;
   struct __main_block_desc_0* Desc;
   
   // int *，初始化列表传递进来的是 static_val 的指针 
   int *static_val;
   
   __main_block_impl_0(void *fp, struct __main_block_desc_0 *desc, int *_static_val, int flags=0) : static_val(_static_val) {
     impl.isa = &_NSConcreteStackBlock;
     impl.Flags = flags;
     impl.FuncPtr = fp;
     Desc = desc;
   }
 };
 ```
 `__main_block_func_0`： 
 ```c++
 static void __main_block_func_0(struct __main_block_impl_0 *__cself) {
 
    // 从 block 结构体实例中取出 static_val 指针
    int *static_val = __cself->static_val; // bound by copy
    
    global_val *= 2;
    static_global_val *= 2;
    (*static_val) *= 3;
}
 ```
 `main` 函数：
 ```c++
 int main(int argc, const char * argv[]) {
     /* @autoreleasepool */ { __AtAutoreleasePool __autoreleasepool; 

         NSLog((NSString *)&__NSConstantStringImpl__var_folders_24_5w9yv8jx63bgfg69gvgclmm40000gn_T_main_54420a_mi_0);
         
         // static_val 初始化
         static int static_val = 3;
         
         // 看到 _static_val 入参是 &static_val
         void (*blk)(void) = ((void (*)())&__main_block_impl_0((void *)__main_block_func_0, &__main_block_desc_0_DATA, &static_val));
         
         // 这里的赋值只是赋值，可以和 __block 的 forwarding 指针方式寻值进行比较思考
         static_val = 12;
         
         ((void (*)(__block_impl *))((__block_impl *)blk)->FuncPtr)((__block_impl *)blk);

         printf("static_val = %d, global_val = %d, static_global_val = %d\n", static_val, global_val, static_global_val);
     }

     return 0;
 }
 ```
&emsp;可看到在 `__main_block_func_0` 内 `global_val` 和 `static_global_val` 的访问和转换前完全相同。静态变量 `static_val` 则是通过指针对其进行访问修改，在 `__main_block_impl_0` 结构体的构造函数的初始化列表中 `&static_val` 赋值给 `struct __main_block_impl_0` 的 `int *static_val` 这个成员变量，这种是通过地址在超出变量作用域的地方访问和修改变量。

> 静态变量的这种方法似乎也适用于自动变量的访问，但是为什么没有这么做呢？

实际上，在由 `block` 语法生成的值 `block` 上，可以存有超过其变量作用域的被截获对象的自动变量，但是如果 `block` 不强引用该自动变量的话，变量作用域结束的同时，该自动变量很可能会释放并销毁，而此时再去访问该自动变量的话会直接因为野指针访问而 `crash`。**而访问静态局部变量不会 `crash` 的原因在于，静态变量是存储在静态变量区的，在程序结束前它一直都会存在，之所以会被称为局部，只是说出了作用域无法直接通过变量名访问它了（对比全局变量在整个模块的任何位置都可以直接访问），并不是说这块数据不存在了，因此我们只要有一个指向该静态变量的指针，那么出了作用域依然能正常访问到它；而对于自动变量，`block` 并不持有它的话，那么一旦出了作用域，自动变量很可能直接释放并销毁，如果此时再访问的话会直接 `crash`，所以针对自动变量 `block` 并不能采用和静态局部变量一样的处理方式。**

2. 第二种是使用 `__block` 说明符。更准确的表达方式为 "`__block` 存储域说明符"（`__block storage-class-specifier`）。

`C` 语言中有以下存储域类说明符:
+ `typedef`
+ `extern`
+ `static`
+ `auto`
+ `register`

`__block` 说明符类似于 `static`、`auto` 和 `register` 说明符，他们用于指定将变量设置到哪个存储域中。例如: `auto` 表示作为自动变量存储在栈中，`static` 表示作为静态变量存储在数据区。

**对于使用 `__block` 修饰的变量，不管在 `block` 中有没有使用它，都会相应的给它生成一个结构体实例。**

在前面编译错误的源代码的自动变量声明上追加 `__block` 说明符：
```c++
int main(int argc, const char* argv[]) {
const char* fmt = "val = %d\n";
__block int val = 10;
void (^blk)(void) = ^{
    val = 20; // 此处能正常修改变量
    printf(fmt, val);
};

blk();
return 0;
}
```
根据 `clang -rewrite-objc` 转换结果发现，`__block val` 被转化为了 `struct __Block_byref_val_0` （`0` 表示当前是第几个 `__block` 变量）结构体实例。
（`__Block_byref_val_0` 命名规则是 `__Block` 做前缀，然后是 `byref` 表示是被 `__block` 修饰的变量，`val` 表示原始的变量名，`0` 表示当前是第几个 `__block` 变量）
```c++
struct __Block_byref_val_0 {
  void *__isa;
__Block_byref_val_0 *__forwarding; // 指向自己的指针
 int __flags;
 int __size;
 int val;
};
```
如果 `__block` 修饰的是对象类型的话，则 `struct __Block_byref_val_0` 会多两个函数指针类型的成员变量: `__Block_byref_id_object_copy`、`__Block_byref_id_object_dispose` 。
```c++
struct __Block_byref_m_Parray_1 {
  void *__isa;
__Block_byref_m_Parray_1 *__forwarding;
 int __flags;
 int __size;
 
 void (*__Block_byref_id_object_copy)(void*, void*);
 void (*__Block_byref_id_object_dispose)(void*);
 
 NSMutableArray *m_Parray;
};
```
`__block_impl`，作为一个被复用的结构体，保持不变
```c++
struct __block_impl {
  void *isa;
  int Flags;
  int Reserved;
  void *FuncPtr;
};
```
`__main_block_impl_0`
```c++
struct __main_block_impl_0 {
  struct __block_impl impl;
  struct __main_block_desc_0* Desc;
  
  // 看到新增了两个成员变量
  // 已知在 block 定义中截获了 fmt 和 val 两个外部变量
  // fmt 和前面的转换没有区别
  const char *fmt;
  
  // val 是一个 __Block_byref_val_0 结构体指针
  __Block_byref_val_0 *val; // by ref
  
  // 首先看到的是 __Block_byref_val_0 * _val 参数，
  // 但是在初始化列表中用的是 val(_val->forwarding 指针)
  // 初始化用的 _val->forwarding
  
  __main_block_impl_0(void *fp, struct __main_block_desc_0 *desc, const char *_fmt, __Block_byref_val_0 *_val, int flags=0) : fmt(_fmt), val(_val->__forwarding) {
    impl.isa = &_NSConcreteStackBlock;
    impl.Flags = flags;
    impl.FuncPtr = fp;
    Desc = desc;
  }
};
```
`__main_block_func_0`
```c++
static void __main_block_func_0(struct __main_block_impl_0 *__cself) {

// 首先从 __main_block_impl_0 结构体实例中取出 val 和 fmt
__Block_byref_val_0 *val = __cself->val; // bound by ref
const char *fmt = __cself->fmt; // bound by copy

// 又看到 val->forwarding-val 
// 先找到 forwarding 然后又取 val 然后给它赋值 20
(val->__forwarding->val) = 20;

// 这里看到实际用 val 截获下来的就是一个 __Block_byref_val_0 结构体实例，
// 对它进行赋值的时候需要通过 forwarding 指针进行，且下面的使用也是通过 forwarding 指针。

printf(fmt, (val->__forwarding->val));

}
```
继续往下看转换后的 `.cpp` 文件，见到了两个新函数：`__main_block_copy_0` 和 `__main_block_dispose_0`：  （`BLOCK_FIELD_IS_BYREF` 后面会讲） 

目前已发现的有如下情况时会生成下面这一对 `copy` 和 `dispose` 函数：

1. 当 `block` 截获对象类型变量时（如：`NSObject` `NSMutableArray`）会有如下的 `copy` 和 `dispose` 函数生成。
2. 当使用 `__block` 变量时会有如下的 `copy` 和 `dispose` 函数生成。
3. 当函数返回值和参数类型都是 `block` 类型时也会有如下的 `copy` 和 `dispose` 函数

`__main_block_copy_0`
```c++
// _Block_object_assign 用的第一个参数: (void*)&dst->val 第二个参数: (void*)src->val
static void __main_block_copy_0(struct __main_block_impl_0*dst,
                                struct __main_block_impl_0*src) {
                                
  _Block_object_assign((void*)&dst->val,
  (void*)src->val, 8/*BLOCK_FIELD_IS_BYREF*/);
  
}
```
`__main_block_dispose_0`
```c++
// 入参: (void*)src->val
static void __main_block_dispose_0(struct __main_block_impl_0*src) {

  _Block_object_dispose((void*)src->val, 8/*BLOCK_FIELD_IS_BYREF*/);
}
```
`__main_block_desc_0` 新增了成员变量：
```c++
static struct __main_block_desc_0 {
  size_t reserved;
  size_t Block_size;
  
  // copy 函数指针
  void (*copy)(struct __main_block_impl_0*, struct __main_block_impl_0*);
  //  dispose 函数指针
  void (*dispose)(struct __main_block_impl_0*);
  
  // 看到下面的静态全局变量初始化用的是上面两个新增的函数 
} __main_block_desc_0_DATA = { 0, sizeof(struct __main_block_impl_0), __main_block_copy_0, __main_block_dispose_0};
```
`main` 函数内部：
```c++
int main(int argc, const char * argv[]) {
    /* @autoreleasepool */ { __AtAutoreleasePool __autoreleasepool; 

        NSLog((NSString *)&__NSConstantStringImpl__var_folders_24_5w9yv8jx63bgfg69gvgclmm40000gn_T_main_3821a8_mi_0);
        
        // fmt 定义不变
        const char* fmt = "val = %d\n";
        
        // 由 val 创建 __Block_byref_val_0 结构体实例，
        // 成员变量 __isa、__forwarding、__flags、__size、val
        // 一手 (void*)0，把 0 转成一个 void* 指针
        // __forwarding 用的是该结构体自己的地址
        // size 就是 sizeof(__Block_byref_val_0)
        // val 是截获的外部自动变量
        __attribute__((__blocks__(byref))) __Block_byref_val_0 val = {(void*)0,(__Block_byref_val_0 *)&val, 0, sizeof(__Block_byref_val_0), 10};
        
        // 如前所示的 __main_block_impl_0 结构体实例
        void (*blk)(void) = ((void (*)())&__main_block_impl_0((void *)__main_block_func_0, &__main_block_desc_0_DATA, fmt, (__Block_byref_val_0 *)&val, 570425344));
        
        // 如前所示 (*blk).impl->FuncPtr 函数执行
        ((void (*)(__block_impl *))((__block_impl *)blk)->FuncPtr)((__block_impl *)blk);
    }

    return 0;
}
```
`__block int val = 0;`  
```
__attribute__((__blocks__(byref))) __Block_byref_val_0 val =
{
    (void*)0,
    (__Block_byref_val_0 *)&val,
    0,
    sizeof(__Block_byref_val_0),
    10
};
```
发现竟然变为了结构体实例。`__block 变量`也同 `Block` 一样变成 `__Block_byref_val_0` 结构体类型的自动变量，即栈上生成的 `__Block_byref_val_0` 结构体实例。该变量初始化为 10，这个值也出现在结构体实例的初始化中，**这意味着该结构体持有相当于原自动变量的成员变量。**
```
struct __Block_byref_val_0 {
    void* isa;
    __Block_byref_val_0* __forwarding;
    int __flags;
    int __size;
    int val;
};
```
**如同初始化时的源代码，该结构体中最后的成员变量 val 是相当于原自动变量的成员变量。**
赋值的情况:
```
static void __main_block_func_0(struct __main_block_impl_0 *__cself) {
    __Block_byref_val_0 *val = __cself->val; // bound by ref
    const char *fmt = __cself->fmt; // bound by copy

    (val->__forwarding->val) = 20;
    printf(fmt, (val->__forwarding->val));
}
```
刚刚在 Block 中向静态变量赋值时只是使用了指向该静态变量的指针。而向 __block 变量赋值更复杂。`__main_block_impl_0` 结构体实例持有指向 __block 变量的 `__Block_byref_val_0` 结构体实例的指针。
`__Block_byref_val_0` 结构体实例的成员变量 `__forwarding` 持有指向该实例自身的指针。通过成员变量 `__forwarding` 访问成员变量 `val`。( 成员变量 val 是该实例自身持有的变量，它相当于原自动变量。)

且 `__Block_byref_val_0` 单独拿出来的定义，这样是为了在多个 Block 中重用。
```
// 示例 1:
        const char* fmt = "val = %d\n";
        __block int val = 10;
        void (^blk)(void) = ^{
//            val = 20;
            printf(fmt, val);
        };
        
        void (^blk2)(void) = ^{
            val = 50;
            printf(fmt, val);
        };
        
        blk2();
        blk();
        // 执行结果:
        val = 50
        val = 50
        
        // blk 和 blk2 定义时截获 __block val 变量，val 只有一份，
        // 不管是被谁修改以后，
        // 当 blk 和 blk2 执行时，取到的都是内存内当前保存的值
// 示例 2:
        const char* fmt = "val = %d\n";
        __block int val = 10;
        void (^blk)(void) = ^{
//            val = 20;
            printf(fmt, val);
        };
        
        void (^blk2)(void) = ^{
            val = 50;
            printf(fmt, val);
        };
        
        blk2();
        val = 60;
        blk();
        // 执行结果:
        val = 50
        val = 60
        
        const char* fmt = "val = %d\n";
        __attribute__((__blocks__(byref))) __Block_byref_val_0 val = {(void*)0,(__Block_byref_val_0 *)&val, 0, sizeof(__Block_byref_val_0), 10};
        
        void (*blk)(void) = ((void (*)())&__main_block_impl_0((void *)__main_block_func_0, &__main_block_desc_0_DATA, fmt, (__Block_byref_val_0 *)&val, 570425344));

        void (*blk2)(void) = ((void (*)())&__main_block_impl_1((void *)__main_block_func_1, &__main_block_desc_1_DATA, fmt, (__Block_byref_val_0 *)&val, 570425344));
```
当 Block 内部使用多个 __block 变量时:
```
// char*
struct __Block_byref_fmt_0 {
  void *__isa;
__Block_byref_fmt_0 *__forwarding;
 int __flags;
 int __size;
 char *fmt;
};

// int 
struct __Block_byref_val_1 {
  void *__isa;
__Block_byref_val_1 *__forwarding;
 int __flags;
 int __size;
 int val;
};

// int 
struct __Block_byref_temp_2 {
  void *__isa;
__Block_byref_temp_2 *__forwarding;
 int __flags;
 int __size;
 int temp;
};

// NSMutableArray *
struct __Block_byref_array_3 {
  void *__isa;
__Block_byref_array_3 *__forwarding;
 int __flags;
 int __size;
 
 // 看到对象类型的多了两个成员变量

 // 该结构体使用的 copy 和 dispose 函数指针
 void (*__Block_byref_id_object_copy)(void*, void*);
 void (*__Block_byref_id_object_dispose)(void*);
 
 NSMutableArray *array;
};

// NSObject *
struct __Block_byref_object_4 {
  void *__isa;
__Block_byref_object_4 *__forwarding;
 int __flags;
 int __size;
 
 // 看到对象类型的多了两个成员变量
 
