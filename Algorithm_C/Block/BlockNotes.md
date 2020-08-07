# + (void)initialize
临时插入一个 initialize 函数的话题：
`+(void)initialize;` 类方法，在该类收到第一条消息前初始化该类。
1. 一般情况下，每个类这一方法自己只主动调用一次，如果程序从没使用过该类，则该方法不会执行。
2. 若分类重写 initialize 方法后，则只会调用分类的 initialize 实现，而原类的该方法不会被调用，即分类里面的实现会覆盖原类。
3. Father 和 Son 同时实现该方法时，Father 会在 Son 之前收到该消息，且双方各执行一次。**父类一定早已子类执行**
4. 若只在 Father 里面实现该方法，则 Father 里面会执行两次，self 一次是 Father 一次是 son，其实是 Father 和 Son 类分别进行了初始化。

**特殊情况：**
+ 只是在 Father 里面实现 initialize 方法，在 Son 里面不实现且 Son 里面不使用 `[super initialize];` 主动调用时，Father 里面会执行两次，两次其实是 Father 和 Son 分别进行了一次初始化：
```
// 第一次的 self 是 Father
Father --> self == Father, functionString == +[Father initialize]
// 第二次的 self 是 Son
Father --> self == Son, functionString == +[Father initialize]
```
+ 在 Son 里面主动使用一次 `[super initialize];`，Father 里面会执行三次，第三次的调用是 super 调用那次产生的，因为看似是用的 super 实际传进去的是 Son。连续调用，则 Father 里面连续执行：
```
// 第一次的 self 是 Father
Father --> self == Father, functionString == +[Father initialize]
// 第二次的 self 是 Son
Father --> self == Son, functionString == +[Father initialize]

// 第三次的 self 是 Son，且这次执行是因为在 Son 里面主动调用了 initialize
Father --> self == Son, functionString == +[Father initialize]

// 连续调用 initialize，在 Father 里面会连续调用，且 self 都是 Son
[super initialize];
[super initialize];
[super initialize];

```
+  **特别注意：如果想使类本身 `+(void) initialize` 方法保护自己部分内容不被多次运行，可以如下书写，建议如果重写了 initialize 方法一定加下面的 if 判断，防止被子类重复执行：**
```
@implementation Father
+ (void)initialize {
    NSLog(@"会被调用几次～～ functionString == %s", __FUNCTION__);
    if (self == [Father class]) {
        // 这里只会执行一次，就是在 Father 类初始化的时候执行一次，
        // 其它再进入该方法时，self 都是 Father 的子类
        NSLog(@"Father --> self == %@, functionString == %s", [self class], __FUNCTION__);
    }
}
@end
```
+ 还有一个点，如果有 `Father <= Son <= GrandSon` 三层继承，每个类如果自身不实现 initialize 方法，则它初始化时或主动调用 `[super initialize]`方法时 ，会去找它最近的一层父类里面看有没有实现，如果实现了则会执行一次 initialize 方法，通过打印 self 可以看出规律:
```
// 1. 只在 Father 里面实现 initialize 方法，且在 GrandSon 里面主动调用了一次 [super initialize];
Father --> self == Father, functionString == +[Father initialize]
Father --> self == Son, functionString == +[Father initialize]
Father --> self == GrandSon, functionString == +[Father initialize]
Father --> self == GrandSon, functionString == +[Father initialize]

// 2. 都实现 initialize 方法，且在 GrandSon 里面主动调用了一次 [super initialize];
Father --> self == Father, functionString == +[Father initialize]
Son --> self == Son, functionString == +[Son initialize]
GrandSon --> self == GrandSon, functionString == +[GrandSon initialize]
// 在 GrandSon 里面主动调用 [super initialize]，实现是到了 Son 里面
Son --> self == GrandSon, functionString == +[Son initialize]

// 3. 在 Son 和 GrandSon 里面实现，且在 GrandSon 里面主动调用了一次 [super initialize];
Son --> self == Son, functionString == +[Son initialize]
GrandSon --> self == GrandSon, functionString == +[GrandSon initialize]
// 主动这次进入了 Son 里面
Son --> self == GrandSon, functionString == +[Son initialize]

// 4. 只在 Son 和 Father 里面实现，且在 GrandSon 里面主动调用了一次 [super initialize];
Father --> self == Father, functionString == +[Father initialize]
Son --> self == Son, functionString == +[Son initialize]
Son --> self == GrandSon, functionString == +[Son initialize]
Son --> self == GrandSon, functionString == +[Son initialize]

// 5. 只在 Son 和 Father 里面实现，可以看到三个类都执行初始化，且 GrandSon 的执行在 Son 里面
Father --> self == Father, functionString == +[Father initialize]
Son --> self == Son, functionString == +[Son initialize]
Son --> self == GrandSon, functionString == +[Son initialize]
```
+ 自己主动调用 `[self initialize];` 执行顺序和上面几乎一样，首先找自己的实现，如果自己没实现就去找父类。
```
// 1. 只有 Father 里面实现了 initialize 函数，其他位置都未实现，
// GrandSon 里面主动调用了一次 [self initialize];
Father --> self == Father, functionString == +[Father initialize]
Father --> self == Son, functionString == +[Father initialize]
Father --> self == GrandSon, functionString == +[Father initialize]
Father --> self == GrandSon, functionString == +[Father initialize]

// 2. Father 和 GrandSon 里面实现 initialize 函数
Father --> self == Father, functionString == +[Father initialize]
Father --> self == Son, functionString == +[Father initialize]
GrandSon --> self == GrandSon, functionString == +[GrandSon initialize]
GrandSon --> self == GrandSon, functionString == +[GrandSon initialize]

// 3. 在自己的 initialize 实现里面调用 [self initialize];
// 会停止运行，并打印如下：
// 看到 Father 里面的 initialize 执行了
Father --> self == Father, functionString == +[Father initialize]
Father --> self == Son, functionString == +[Father initialize]
warning: could not execute support code to read Objective-C class data in the process. This may reduce the quality of type information available.
```


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

// 虽然改写了 val，blk 只是截获了 val 的瞬间值去初始化 block 结构体，
// val 的值无论再怎么改写，都与 blk 内的值再无瓜葛
val = 2;
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
// 直接改写 val 指向的地址内的值，blk 截获的 val 指向的地址
*val = 20; 
fmt = "These values were changed. val = %d\n";

blk();
// 运行结果：val = 20

// 示例 3：
int dmy = 256;
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
// 运行结果：val = 22

// 示例 4：
int dmy = 256;
__block int val = 10;
const char* fmt = "val = %d\n";
void (^blk)(void) = ^{
    printf(fmt, val);
};

// val 用 __block 修饰后，blk 内部持有的也是 val 的地址，
// 这里也代表着修改了内存地址里面存放的值，
// 所以 blk 执行时，读出来的也是这个 2
val = 2;
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
以上不能修改（或者理解为为其赋值）时，可以用 __block 说明符来修饰该变量。该变量称为 __block 变量。
> 注意：
> 在 Block 内部不能使用 C 语言数组：
```
const char text[] = "Hello"; 
void (^blk)(void) = ^{ 
  // Cannot refer to declaration with an array type inside block 
  // 这是因为现在的 Blocks 截获自动变量的方法并没有实现对 C 语言数组的截获。
  printf("%c\n", text[0]);
}; 
```

> 向截获的 NSMutableArray 变量赋值会产生编译错误。源码中截获的变量值为 NSMutableArray 类的对象，如果用 C 语言来描述，即是截获 NSMutableArray 类对象用的结构体实例指针。

## Block 的实质
Block 是 “带有自动变量的匿名函数”，但 Block 究竟是什么呢？
语法看上去很特别，但它实际上是作为**极普通的 C 语言源码**来处理的。通过**支持 Block 的编译器**，含有 Block 语法的源代码转换为一般 C 语言编译器能够处理的源代码，并作为极为普通的 C 语言源代码被编译。
这不过是概念上的问题，在实际编译时无法转换成我们能够理解的源代码，但 clang(LLVM 编译器)具有转换为我们可读源代码的功能。通过 "-rewrite-objc" 选项就能将含有 Block 语法的源代码转换为 C++ 的源代码。说是 C++，其实也**仅仅是使用了 struct 结构，其本质是 C 语言源代码**。

> clang -rewrite-objc 源代码文件名

如下源代码通过 clang 可变换为: 

```
int main() {
void (^blk)(void) = ^{ printf("Block\n"); };
blk();

return 0;
}
```

 **__block_impl**
 ```
 struct __block_impl {
   void *isa;
   int Flags;
   int Reserved;
   void *FuncPtr;
 };
 ```
 
 **__main_block_impl_0**
 ```
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
 
 **__main_block_func_0**
 ```
 static void __main_block_func_0(struct __main_block_impl_0 *__cself) {
     printf("Block\n");
 }
 ```
 
 **__main_block_desc_0**
 ```
 static struct __main_block_desc_0 {
   size_t reserved;
   size_t Block_size;
 } __main_block_desc_0_DATA = { 0, sizeof(struct __main_block_impl_0)};
 ```
 
**main 函数内部**
```
int main(int argc, const char * argv[]) {
    /* @autoreleasepool */ { __AtAutoreleasePool __autoreleasepool; 
    NSLog((NSString *)&__NSConstantStringImpl__var_folders_24_5w9yv8jx63bgfg69gvgclmm40000gn_T_main_948e6f_mi_0);
        
    // 首先是等号左边是一个返回值和参数都是 void 的函数指针: void (*blk)(void)
    // 等号右边去掉 &(取地址符) 前面的强制类型转换后，可看到后面是创建了一个
    // __main_block_impl_0 结构体实例，所以此处可以理解为在栈上创建了一个 Block 结构体实例
    // 并把它的地址转化为了一个函数指针
    void (*blk)(void) = ((void (*)())&__main_block_impl_0((void *)__main_block_func_0, &__main_block_desc_0_DATA));
    
    // 取出 __block_impl 里面的 FuncPtr 函数执行
    ((void (*)(__block_impl *))((__block_impl *)blk)->FuncPtr)((__block_impl *)blk);
    }

    return 0;
}

// 生成 Block 去掉转换部分可以理解为：
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
如变换后的源代码所示，通过 Blocks 使用的匿名函数实际上**被作为简单的 C 语言函数来处理**(__main_block_func_0 函数)。另外，**根据 Block 语法所属的函数名（此处为 main）和该 Block 语法在该函数出现的顺序值（此处为 0）来给经 clang 变换的函数命名**。
该函数的参数 __cself 相当于 C++ 实例方法中指向实例自身的变量 this，或是 OC 实例方法中指向对象自身的变量 self，即参数 __cself 为指向 Block 值的变量。（__main_block_impl_0）

> C++ 的 this，Objective-C 的 self
```
// C++ 中定义类的实例方法：
void MyClass::method(int arg) {
    printf("%p %d\n", this, arg);
}

// C++ 编译器将该方法作为 C 语言函数来处理
void __ZN7MyClass6methodEi(MyClass *this, int arg) {
    printfi("%p %d\n", this, arg);
}
// MyClass::method 方法的实质就是 __ZN7MyClass6methodEi 函数，
// "this" 作为第一个参数传递进去。
// 该方法调用如下:
MyClass cls;
cls.method(10);
// C++ 编译器转换成 C 语言函数调用形式:
struct MyClass cls;
__ZN7MyClass6methodEi(&cls, 10);
// 即 this 就是 MyClass 类（结构体）的实例。

// OC 实例方法：
- (void)method:(int)arg {
    NSLog(@"%p %d\n", self, arg);
}
// OC 编译器同 C++ 方法一样，也将该方法作为 C 语言函数处理
void _I_MyObject_method_(struct MyObject *self, SEL _cmd, int arg) {
    NSLog(@"%p %d\n", self, arg);
}
// "self" 作为第一个参数被传递过去，以下调用方代码:
MyObject *obj = [[MyObject alloc] init];
[obj method:10];
// 使用 clang 的 -rewrite-objc 选项，会转为如下:
MyObject *obj = objc_msgSend(objc_getClass("MyObject"), sel_registerName("alloc"));
obj = objc_msgSend(obj, sel_registerName("init"));

objc_msgSend(obj, sel_registerName("method:"), 10);

// ！！！此下段描述极为重要！！！
// objc_msgSend 函数根据指定的对象和函数名，从 对象持有类的结构体中 检索
// _I_MyObject_method_ 函数的指针并调用。
// 此时 objc_msgSend 函数的第一个参数 obj 作为 _I_MyObject_method_ 函数的
// 第一个参数 self 进行传递。
// 同 C++ 一样，self 就是 MyObject 类的对象。
```
`static void __main_block_func_0(struct __main_block_impl_0* __cself)` 与 C++ 的 this 和 OC 的 self 相同，参数 __cself 是 __main_block_impl_0 结构体的指针。

`isa = &_NSConcreteStackBlock;` 
将 Block 指针赋给 Block 的结构体成员变量 isa。为了理解它，首先要理解 OC 类和对象的实质。其实，所谓 Block 就是 OC 对象。

```
// 把 __main_block_impl_0 展开的话
// 已经几乎和 OC 对象
struct __main_block_impl_0 {
void* isa;
int Flags;
int Reserved;
void* FuncPtr;

struct __main_block_desc_0* Desc;
};
```

```
typedef struct objc_object* id;
typedef struct objc_class* Class;
// objc_object 结构体和 objc_class 
// 结构体归根结底是在各个对象和类的实现中使用的最基本的结构体
```

## 截获自动变量值

通过 clang 进行转换：
```
int dmy = 256;
int val = 10;
int* valPtr = &val;
const char* fmt = "val = %d\n";

void (^blk)(void) = ^{
    printf(fmt, val);
    printf("valPtr = %d\n", *valPtr);
};

// val 修改为 2，valPtr 指针也跟着指为 2 
// blk 内部调用是 *valPtr 也是 2
// Block 结构体实例 valPtr 成员变量，初始值传入的就是外部的 valPtr
// 它们两者指向的内存地址是一样的

val = 2;
fmt = "These values were changed. val = %d\n";

blk();

// 打印结果:
val = 10
valPtr = 2

```
转换后的代码:

__block_impl  不变：
```
struct __block_impl {
  void *isa;
  int Flags;
  int Reserved;
  void *FuncPtr;
};
```
__main_block_impl_0 成员变量增加了，Block 语法表达式中使用的自动变量被作为成员变量追加到了 __main_block_impl_0 结构体中，且类型完全相同：
```
struct __main_block_impl_0 {
  struct __block_impl impl;
  struct __main_block_desc_0* Desc;
  
  // Block 截获三个外部的自动变量，然后增加了自己对应的成员变量
  // 且和外部的自动变量的类型都是完全一致的
  //（这里加深记忆，后面学习 __block 变量的时候可与其进行比较）
  const char *fmt;
  int val;
  int *valPtr;
  
  // 初始化列表里面 : fmt(_fmt), val(_val), valPtr(_valPtr)
  // 构造结构体实例时用截获的外部自动变量进行初始化
  __main_block_impl_0(void *fp, struct __main_block_desc_0 *desc, const char *_fmt, int _val, int *_valPtr, int flags=0) : fmt(_fmt), val(_val), valPtr(_valPtr) {
    impl.isa = &_NSConcreteStackBlock;
    impl.Flags = flags;
    impl.FuncPtr = fp;
    Desc = desc;
  }
  
};
```
__main_block_func_0，函数内部终于使用到了 __cself：
```
static void __main_block_func_0(struct __main_block_impl_0 *__cself) {

    // 可以看到通过函数传入 __main_block_impl_0 实例读取对应的截获的外部自动变量 
    const char *fmt = __cself->fmt; // bound by copy
    int val = __cself->val; // bound by copy
    int *valPtr = __cself->valPtr; // bound by copy

    printf(fmt, val);
    printf("valPtr = %d\n", *valPtr);
}
```
__main_block_desc_0 保持不变：
```
static struct __main_block_desc_0 {
  size_t reserved;
  size_t Block_size;
} __main_block_desc_0_DATA = { 0, sizeof(struct __main_block_impl_0)};
```
main 函数里面，__main_block_impl_0 结构体构建和 __main_block_func_0 函数执行保持不变：
```
int main(int argc, const char * argv[]) {
    /* @autoreleasepool */ { __AtAutoreleasePool __autoreleasepool; 

        NSLog((NSString *)&__NSConstantStringImpl__var_folders_24_5w9yv8jx63bgfg69gvgclmm40000gn_T_main_4ea116_mi_0);


        int dmy = 256;
        int val = 10;
        int* valPtr = &val;
        const char* fmt = "val = %d\n";
        
        // 根据传递给构造函数的参数对由自动变量追加的成员变量进行初始化
        void (*blk)(void) = ((void (*)())&__main_block_impl_0((void *)__main_block_func_0, &__main_block_desc_0_DATA, fmt, val, valPtr));

        val = 2;
        fmt = "These values were changed. val = %d\n";

        ((void (*)(__block_impl *))((__block_impl *)blk)->FuncPtr)((__block_impl *)blk);
    }

    return 0;
}
```
**总的来说，所谓 “截获自动变量值” 意味着在执行Block 语法时，Block 语法表达式所使用的自动变量值被保存到 Block 的结构体实例（即 Block 自身）中。** 
这里前面提到的 Block 不能直接使用 C  语言数组类型的自动变量。如前所述，截获自动变量时，将值传递给结构体的构造函数进行保存，如果传入的是 C 数组，假设是 a[10]，那构造函数内部发生的赋值是 `int b[10] = a;` 这是 C 语言规范所不允许的。Block 是完全遵循 C 语言规范的。

### __block 说明符
> 回顾前面截获自动变量值的例子：
```
^{ printf(fmt, val); };

// 转换后是:
static void __main_block_func_0(struct __main_block_impl_0* __cself) {
    const char* fmt = __cself->fmt;
    int val = __cself->val;

    printf(fmt, val);
}
```
> 看完源代码，Block 中所使用的被截获自动变量就如 “带有自动变量值的匿名函数” 所说，仅截获自动变量的值。Block 中使用自动变量后，在 Block 的结构体实例中重写该自动变量也不会改变原先截获的自动变量。当试图改变自动变量值时，会发生编译错误。因为在实现上不能改写被截获自动变量的值，所以当编译器在编译过程中检出给被截获自动变量赋值的操作时，便产生编译错误。理论上 block 内部的成员变量已经和外部的自动变量完全无瓜葛了，理论上 block 结构体的成员变量是能修改的，但是这里修改是结构体自己的成员变量，且又和外部完全同名，如果修改了内部成员变量开发者会误以为连带外部自动变量一起修改了，索性直接发生编译错误更好！（当然 __block 变量就是为修改而生的）。

解决这个问题有两种方法：
1. C 语言中有一个变量允许 Block 改写值:
 + 静态变量
 + 静态全局变量
 + 全局变量
 虽然 Block 语法的匿名函数部分简单转换为了 C  语言函数，但从这个变换的函数中访问 **静态全局变量/全局变量**并没有任何改变，可直接使用。
 &ensp;但是静态变量的情况下，转换后的函数原本就设置在含有 Block 语法的函数外，所以无法从变量作用域访问。
 
 代码验证:
 ```
 int global_val = 1;
 static int static_global_val = 2;
 
 int main(int argc, const char * argv[]) {
 @autoreleasepool {
     // insert code here...
     static int static_val = 3;
     void (^blk)(void) = ^{
        global_val *= 2;
        static_global_val *= 2;
        static_val *= 3;
     };
     static_val = 12;
     blk();
                
     // static_val = 111;
     printf("static_val = %d, global_val = %d, static_global_val = %d\n", static_val, global_val, static_global_val);
     }
}
// 执行结果:
// 看到 static_val 是 36， 即 blk 执行前 static_val 修改为了 12
// 然后 blk 执行时 static_val = 12 * 3 => static_val = 36
// 即 block 内部可以修改 static_val 且 static_val 外部的修改也会
// 传递到 blk 内部
// static_val = 36, global_val = 2, static_global_val = 4
 ```
 clang 转换后的源代码:
 __main_block_impl_0 追加了 static_val 指针为成员变量
 ```
 struct __main_block_impl_0 {
   struct __block_impl impl;
   struct __main_block_desc_0* Desc;
   
   // 记得是 int *，传递进来的是 static_val 的指针 
   int *static_val;
   
   __main_block_impl_0(void *fp, struct __main_block_desc_0 *desc, int *_static_val, int flags=0) : static_val(_static_val) {
     impl.isa = &_NSConcreteStackBlock;
     impl.Flags = flags;
     impl.FuncPtr = fp;
     Desc = desc;
   }
 };
 ```
 __main_block_func_0 
 ```
 static void __main_block_func_0(struct __main_block_impl_0 *__cself) {
 
    // 从 block 结构体中取出 static_val 指针
    int *static_val = __cself->static_val; // bound by copy
    
    global_val *= 2;
    static_global_val *= 2;
    (*static_val) *= 3;
}
 ```
 main 函数
 ```
 int main(int argc, const char * argv[]) {
     /* @autoreleasepool */ { __AtAutoreleasePool __autoreleasepool; 

         NSLog((NSString *)&__NSConstantStringImpl__var_folders_24_5w9yv8jx63bgfg69gvgclmm40000gn_T_main_f4dae6_mi_0);
         
         static int static_val = 3;
         
         // 入参是 &static_val，（是地址 是指针）
         void (*blk)(void) = ((void (*)())&__main_block_impl_0((void *)__main_block_func_0, &__main_block_desc_0_DATA, &static_val));

         ((void (*)(__block_impl *))((__block_impl *)blk)->FuncPtr)((__block_impl *)blk);

         printf("static_val = %d, global_val = %d, static_global_val = %d\n", static_val, global_val, static_global_val);
     }

     return 0;
 }
 ```
 
可看到 `global_val` 和 `static_global_val` 的访问和转换前完全相同。
使用静态变量 `static_val` 的指针对其进行访问。将 static_val 的指针传递给 `__main_block_impl_0` 结构体的构造函数并保存。这是超出作用域使用变量的最简单方法。
静态变量的这种方法似乎也适用于自动变量的访问，但是为什么没有这么做呢？
实际上，在由 Block 语法生成的值 Block 上，可以存有超过其变量作用域的被截获对象的自动变量。变量作用域结束的同时，原来的自动变量被废弃，因此 Block 中超过变量作用域而存在的变量同静态变量一样，将不能通过指针访问原来的自动变量。
2. 第二种是使用 "__block 说明符"。更准确的表达方式为 "__block 存储域说明符"（__block storage-class-specifier）。
&ensp;C 语言中有以下存储域类说明符:
+ typedef
+ extern
+ static
+ auto
+ register
__block 说明符类似于 static、auto 和 register 说明符，他们用于指定将变量设置到哪个存储域中。例如: `auto` 表示作为自动变量存储在栈中，`statci` 表示作为静态变量存储在数据区中。

在前面编译错误的源代码的自动变量声明上追加 __block 说明符：
```
int main(int argc, const char* argv[]) {
const char* fmt = "val = %d\n";
__block int val = 10;
void (^blk)(void) = ^{
    val = 20;
    printf(fmt, val);
};

blk();
return 0;
}
```
转换如下:
根据 `main` 函数里面的实现发现，直接根据 `val` 定义了一个结构体`__block_byref_val_0`
```
struct __Block_byref_val_0 {
  void *__isa;
__Block_byref_val_0 *__forwarding;
 int __flags;
 int __size;
 int val;
};
```
`__block_impl`，作为一个复用结构体，保持不变
```
struct __block_impl {
  void *isa;
  int Flags;
  int Reserved;
  void *FuncPtr;
};
```
`__main_block_impl_0`
```
struct __main_block_impl_0 {
  struct __block_impl impl;
  struct __main_block_desc_0* Desc;
  
  // 看到新增了两个成员变量
  // 已知在 block 定义中截获了 fmt 和 val 两个外部自动变量
  // fmt 和前面的转码没有区别
  const char *fmt;
  // val 是一个 __Block_byref_val_0 结构体指针
  __Block_byref_val_0 *val; // by ref
  
  // 首先看到的是 __Block_byref_val_0 * _val 参数
  // 但是在初始化列表中用的是 val(_val->forwarding)
  __main_block_impl_0(void *fp, struct __main_block_desc_0 *desc, const char *_fmt, __Block_byref_val_0 *_val, int flags=0) : fmt(_fmt), val(_val->__forwarding) {
    impl.isa = &_NSConcreteStackBlock;
    impl.Flags = flags;
    impl.FuncPtr = fp;
    Desc = desc;
  }
};
```
`__main_block_func_0 `
```
static void __main_block_func_0(struct __main_block_impl_0 *__cself) {

// 首先从 __main_block_impl_0 结构体实例中取出 val 和 fmt
__Block_byref_val_0 *val = __cself->val; // bound by ref
const char *fmt = __cself->fmt; // bound by copy

// 又看到 val->forwarding-val 
// 先找到 forwarding 然后又取 val 然后给它赋值 20
(val->__forwarding->val) = 20;
printf(fmt, (val->__forwarding->val));

}
```
**已知：当Block 截获对象类型变量时（如 NSObject NSMutableArray）会有如下的 copy 和 dispose 函数**
**当使用 __block 变量时会有如下的 copy 和 dispose 函数**

第一次出现的： `__main_block_copy_0`, // `BLOCK_FIELD_IS_BYREF` 后面研究 
```
// _Block_object_assign 用的第一个参数: (void*)&dst->val
// 第二个参数: (void*)src->val
static void __main_block_copy_0(struct __main_block_impl_0*dst, struct __main_block_impl_0*src) {_Block_object_assign((void*)&dst->val, (void*)src->val, 8/*BLOCK_FIELD_IS_BYREF*/);}
```
第二次出现：`__main_block_dispose_0`
```
// 入参: (void*)src->val
static void __main_block_dispose_0(struct __main_block_impl_0*src) {_Block_object_dispose((void*)src->val, 8/*BLOCK_FIELD_IS_BYREF*/);}
```
`__main_block_desc_0` 新增了成员变量
```
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
`main` 函数内部:
```
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
 void (*__Block_byref_id_object_copy)(void*, void*);
 void (*__Block_byref_id_object_dispose)(void*);
 NSObject *object;
};

struct __main_block_impl_0 {
  struct __block_impl impl;
  struct __main_block_desc_0* Desc;
  
  // 对应 5 个 __block 变量的结构体类型的成员变量
  __Block_byref_fmt_0 *fmt; // by ref
  __Block_byref_val_1 *val; // by ref
  __Block_byref_temp_2 *temp; // by ref
  __Block_byref_array_3 *array; // by ref
  __Block_byref_object_4 *object; // by ref
  
  // 初始化列表：
  // fmt(_fmt->__forwarding), 
  // val(_val->__forwarding), 
  // temp(_temp->__forwarding),
  // array(_array->__forwarding),
  // object(_object->__forwarding)
  
  __main_block_impl_0(void *fp, struct __main_block_desc_0 *desc, __Block_byref_fmt_0 *_fmt, __Block_byref_val_1 *_val, __Block_byref_temp_2 *_temp, __Block_byref_array_3 *_array, __Block_byref_object_4 *_object, int flags=0) : fmt(_fmt->__forwarding), val(_val->__forwarding), temp(_temp->__forwarding), array(_array->__forwarding), object(_object->__forwarding) {
    impl.isa = &_NSConcreteStackBlock;
    impl.Flags = flags;
    impl.FuncPtr = fp;
    Desc = desc;
  }
  
  static void __main_block_func_0(struct __main_block_impl_0 *__cself) {
  // 取出成员变量
  __Block_byref_fmt_0 *fmt = __cself->fmt; // bound by ref
  __Block_byref_val_1 *val = __cself->val; // bound by ref
  __Block_byref_temp_2 *temp = __cself->temp; // bound by ref
  __Block_byref_array_3 *array = __cself->array; // bound by ref
  __Block_byref_object_4 *object = __cself->object; // bound by ref

            (fmt->__forwarding->fmt) = "FMT val = %d\n";
            printf((fmt->__forwarding->fmt), (val->__forwarding->val));
            (temp->__forwarding->temp) = 30;
            
            ((void (*)(id, SEL, ObjectType _Nonnull))(void *)objc_msgSend)((id)(array->__forwarding->array), sel_registerName("addObject:"), (id _Nonnull)(object->__forwarding->object));
    }
    
    // Block 用的 copy，只有 Block 中使用 __block 变量时才会出现 
    static void __main_block_copy_0(struct __main_block_impl_0*dst, struct __main_block_impl_0*src) {_Block_object_assign((void*)&dst->fmt, (void*)src->fmt, 8/*BLOCK_FIELD_IS_BYREF*/);_Block_object_assign((void*)&dst->val, (void*)src->val, 8/*BLOCK_FIELD_IS_BYREF*/);_Block_object_assign((void*)&dst->temp, (void*)src->temp, 8/*BLOCK_FIELD_IS_BYREF*/);_Block_object_assign((void*)&dst->array, (void*)src->array, 8/*BLOCK_FIELD_IS_BYREF*/);_Block_object_assign((void*)&dst->object, (void*)src->object, 8/*BLOCK_FIELD_IS_BYREF*/);}

    // Block 用的 dispose，只有 Block 中使用 __block 变量时才会出现
    static void __main_block_dispose_0(struct __main_block_impl_0*src) {_Block_object_dispose((void*)src->fmt, 8/*BLOCK_FIELD_IS_BYREF*/);_Block_object_dispose((void*)src->val, 8/*BLOCK_FIELD_IS_BYREF*/);_Block_object_dispose((void*)src->temp, 8/*BLOCK_FIELD_IS_BYREF*/);_Block_object_dispose((void*)src->array, 8/*BLOCK_FIELD_IS_BYREF*/);_Block_object_dispose((void*)src->object, 8/*BLOCK_FIELD_IS_BYREF*/);}
    
    // 不变
    static struct __main_block_desc_0 {
      size_t reserved;
      size_t Block_size;
      void (*copy)(struct __main_block_impl_0*, struct __main_block_impl_0*);
      void (*dispose)(struct __main_block_impl_0*);
    } __main_block_desc_0_DATA = { 0, sizeof(struct __main_block_impl_0), __main_block_copy_0, __main_block_dispose_0};
    
    // 5 个 __block 变量的初始化：
    // fmt
    __attribute__((__blocks__(byref))) __Block_byref_fmt_0 fmt = {(void*)0,(__Block_byref_fmt_0 *)&fmt, 0, sizeof(__Block_byref_fmt_0), "val = %d\n"};
    
    // val
    __attribute__((__blocks__(byref))) __Block_byref_val_1 val = {(void*)0,(__Block_byref_val_1 *)&val, 0, sizeof(__Block_byref_val_1), 10};
    
    // temp
    __attribute__((__blocks__(byref))) __Block_byref_temp_2 temp = {(void*)0,(__Block_byref_temp_2 *)&temp, 0, sizeof(__Block_byref_temp_2), 20};
    
    // array
    __attribute__((__blocks__(byref))) __Block_byref_array_3 array = {(void*)0,(__Block_byref_array_3 *)&array, 33554432, sizeof(__Block_byref_array_3), __Block_byref_id_object_copy_131, __Block_byref_id_object_dispose_131, ((NSMutableArray * _Nonnull (*)(id, SEL))(void *)objc_msgSend)((id)objc_getClass("NSMutableArray"), sel_registerName("array"))};
    
    // object
    __attribute__((__blocks__(byref))) __Block_byref_object_4 object = {(void*)0,(__Block_byref_object_4 *)&object, 33554432, sizeof(__Block_byref_object_4), __Block_byref_id_object_copy_131, __Block_byref_id_object_dispose_131, ((NSObject *(*)(id, SEL))(void *)objc_msgSend)((id)((NSObject *(*)(id, SEL))(void *)objc_msgSend)((id)objc_getClass("NSObject"), sel_registerName("alloc")), sel_registerName("init"))};

    // array 和 object 里面有新东西
    // __Block_byref_id_object_copy_131
    // __Block_byref_id_object_dispose_131
    // 结构体里面定义是两个函数指针:
    // void (*__Block_byref_id_object_copy)(void*, void*);
    // void (*__Block_byref_id_object_dispose)(void*);
    
    // 在 110 行和 113 行找到了定义
    static void __Block_byref_id_object_copy_131(void *dst, void *src) {
     _Block_object_assign((char*)dst + 40, *(void * *) ((char*)src + 40), 131);
    }
    
    static void __Block_byref_id_object_dispose_131(void *src) {
     _Block_object_dispose(*(void * *) ((char*)src + 40), 131);
    }
    
    // 看到里面调用了 _Block_object_assign 和 _Block_object_dispose
    // 这和上面的 __main_block_copy_0 和 __main_block_dispose_0 里面调用是是一样的函数
    // _Block_object_dispose 和 _Block_object_assign 函数。
    // 已知 Block 截获对象类型和使用 __block 变量时
    // 会添加 __main_block_copy_0 和 __main_block_dispose_0 函数。 
};

// 看到 68 行的定义:
// Runtime copy/destroy helper functions (from Block_private.h)
#ifdef __OBJC_EXPORT_BLOCKS
extern "C" __declspec(dllexport) void _Block_object_assign(void *, const void *, const int);
extern "C" __declspec(dllexport) void _Block_object_dispose(const void *, const int);
extern "C" __declspec(dllexport) void *_NSConcreteGlobalBlock[32];
extern "C" __declspec(dllexport) void *_NSConcreteStackBlock[32];
#else
__OBJC_RW_DLLIMPORT void _Block_object_assign(void *, const void *, const int);
__OBJC_RW_DLLIMPORT void _Block_object_dispose(const void *, const int);
__OBJC_RW_DLLIMPORT void *_NSConcreteGlobalBlock[32];
__OBJC_RW_DLLIMPORT void *_NSConcreteStackBlock[32];
#endif

```

## Block 存储域
&ensp;通过前面的研究可知，Block 转换为 Block 的结构体类型的自动变量，__block 变量转换为 __block 变量的结构体类型的自动变量。所谓结构体类型的自动变量，即栈上生成的该结构体的实例。如表:
|  名称  |  实质  |
|  -----  |  -----  |
| Block |  栈上 Block 的结构体实例  |
| __block 变量  | 栈上 __block 变量的结构体实例 |
