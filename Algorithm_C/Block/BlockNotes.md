#  Blocks
> 主要介绍 OS X Snow Leopard(10.6) 和 iOS 4 引入的 C 语言扩充功能 “Blocks” 。究竟是如何扩充 C 语言的，扩充之后又有哪些优点呢？下面通过其实现来一步一步探究。
## 什么是 Blocks 
Blocks 是 C 语言的扩充功能。可以用一句话来表示 Blocks 的扩充功能：带有自动变量（局部变量）的匿名函数。(对于程序员而言，命名就是工作的本质。)
### 首先何谓匿名，下面的示例代码可完美解释
```
typedef void(^Blk_T)(int);
// block 定义(匿名)
int i = 10;
Blk_T a = ^void (int event) {
    printf("buttonID: %d event = %d\n", i, event);
};
// 函数定义
void Func(int event) {
    printf("buttonID: %d event = %d\n", i, event);
};
void (*funcPtr)(int) = &Func;
```
匿名是针对有名而言的，如上代码 Blk_T a 等号后面的 block 定义是不需要取名的，而下面的 Func 函数定义必须给它一个函数名。
> Block 可以省略返回值类型的，省略返回值类型时，如果表达式中有 return 语句就使用该返回值的类型，如果表达式没有 return 语句就使用 void 类型。表达式中含有多个 return 语句时，所有的 return 的返回值类型必须相同。
（以前一直忽略了 Block 是可以省略返回值类型的，以为是 Swift 的闭包才支持省略返回值类型。😖）

完整形式的 Block 语法与一般的 C 语言函数定义相比，仅有两点不同：
1. 没有函数名。
2. 带有 “^”。
Block 定义范式如下:
^ 返回值类型 参数列表 表达式
“返回值类型” 同 C 语言函数的返回值类型，“参数列表” 同 C 语言函数的参数列表，“表达式”同 C 语言函数中允许使用的表达式。

在 Block 语法下，可将 Block 语法赋值给声明为 Block 类型的变量中。即源代码中一旦使用 Block 语法就相当于生成了可赋值给 Block 类型变量的 “值”。Blocks 中由 Block 语法生成的值也称为 “Block”。“Block”既指源代码中的 Block 语法，也指由 Block 语法所生成的值。
使用 Block  语法将 Block 赋值为 Block 类型变量。
```
int (^blk)(int) = ^(int count) { return count + 1; };
```
```
// 返回值是 Block 类型的函数
void (^ func() )(int) {
    return ^(int count) { return count + 1; }; 
}
```
Block 类型变量可完全像通常的 C 语言变量一样使用，因此也可以使用指向 Block 类型变量的指针，即 Block 的指针类型变量。
```
typedef int (^blk_t)(int);
blk_t blk = ^(int count) { return count + 1; };
blk_t* blkPtr = &blk;
(*blkPrt)(10);
```

Block 类型变量与一般的 C 语言变量完全相同，可作为以下用途使用：
+ 自动变量
+ 函数参数
+ 静态变量
+ 静态全局变量
+ 全局变量

通过 Block 类型变量调用 Block 与 C 语言通常的函数调用没有区别。

## 截获自动变量值

“带有自动变量值” 在 Blocks 中表现为 “截获自动变量值”。
“带有” => “截获”

```
// 示例 1：
int dmy = 256;
int val = 10;
const char* fmt = "val = %d\n";
void (^blk)(void) = ^{
    printf(fmt, val);
};

val = 2; // 虽然改写了 val，blk 只是截获了 val 的瞬间值去初始化 block 结构体，val 的值无论再怎么改写，都与 blk 内的值再无瓜葛
fmt = "These values were changed. val = %d\n";

blk();
// 运行结果：val = 10

// 示例 2：
int dmy = 256;
int temp = 10;
int* val = &temp;
const char* fmt = "val = %d\n";
void (^blk)(void) = ^{
    printf(fmt, *val);
};

*val = 20; // 直接改写 val 指向的地址内的值，blk 截获的 val 指向的地址
fmt = "These values were changed. val = %d\n";

blk();
// 运行结果：val = 20

// 示例 3：
int dmy = 256;
__block int val = 10;
const char* fmt = "val = %d\n";
void (^blk)(void) = ^{
    printf(fmt, val);
};

val = 2; // val 用 __block 修饰后，blk 内部持有的也是 val 的地址，这里也代表着修改了内存地址里面存放的值，所以 blk 执行时，读出来的也是这个 2
fmt = "These values were changed. val = %d\n";

blk();
// 运行结果：val = 2
```

> **思考：无论 Block 定义在哪，啥时候执行。当 Block 执行时，用的值都是它之前截获（可以理解为拿外部变量赋值给 Block 结构体的成员变量）的基本变量或者是截获的内存地址，如果是内存地址的话，从定义到执行这段时间，不管里面保存的值有没有被修改了， block 执行时，都是读出来的当时内存里保存的值。** 

Block 语法的表达式使用的是它之前声明的自动变量 fmt 和 val。Blocks 中，Block 表达式截获所使用的自动变量的值，即保存该自动变量的瞬间值。

### __block 说明符
自动变量值截获只能保存执行 Block 语法瞬间的值。保存后就不能改写该值。
这个不能改写该值是 Block 的语法规定。如果是截获的是指针变量的话，可以通过指针来修改内存里面的值。比如传入 `NSMutableArray` 变量，可以往里面添加对象，但是不能对该 `NSMutableArray` 变量进行赋值。传入 `int* val` 也可以直接用 `*val = 20;` 来修改 val 指针指向的内存里面保存的值，并且如果截获的是指针变量的话，在 Block 内部修改其指向内存里面的内容后，在 Block 外部读取该指针指向的值时也与 block 内部的修改都是同步的。**因为本身它们操作的都是同一块内存地址**。
这里之所以语法定为不能修改，是因为修改了值以后是无法传出去的，只是在 block 内部使用，是没有意义的。就比如 Block 定义里面截获了变量 val，你看着这时用的是 val 这个变量，其实只是把 val 变量的值赋值给了 block 结构体的 val 成员变量。这时在 Block 内部修改 val 的值，可以理解为只是修改 block 结构体 val 成员变量的值，与 Block 外部的 val 已经完全无瓜葛了。然后截获指针变量也是一样的，其实截获的只是指针变量所指向的地址，在 Block 内部修改的只是 Block 结构体成员变量的指向，这种修改针对外部变量而言都是毫无瓜葛的。
```
// 示例代码
int dmy = 256;
int temp = 10;
int* val = &temp;

printf("🎉🎉 val 初始值：= %d\n", *val);

const char* fmt = "🎉 Block 内部：val = %d\n";
void (^blk)(void) = ^{
    printf(fmt, *val);
    int temp2 = 30;
    val = &temp2; // !!!!!!!!! 这里报错 Variable is not assignable (missing __block type specifier)
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
