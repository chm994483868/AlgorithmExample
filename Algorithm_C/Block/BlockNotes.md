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

initialize 的源码在 objc4 中分析。

# +(void)load
临时插入一个 load 函数的话题：
load 方法使用频次不算太高。
load 方法是在加载类和分类时系统调用的，一般不手动调用，
如果想要在类或分类加载时做一些事情，可以重写类或者分类的 load 方法。
每个类、分类的 load 在程序运行过程中只调用一次。

## 使用场景：
1. hook 方法的时候
2. 涉及到组件化开发中不同组件间通信，在 load 中注册相关协议等。
## 调用时机
在 pre-main 阶段（即 main 函数之前），这点大家都知道。所以 load 应该尽可能的简单，不然影响 App 启动速度。
## load 执行顺序
父类、子类、category 都实现 load 方法，那么 load 的加载顺序是如何呢？
1. 首先是父类一定早于子类执行，父类和子类一定早于其分类执行。
2. 当父类的分类和子类的分类都实现时，此时根它们的编译顺序有关，可调整 Compile Sources 里面编译顺序自己验证（先编译，先调用)。
3. 类要优先于分类调用 +load 方法。
4. 子类调用 +load 方法时，要先要调用父类的 +load 方法；(父类优先与子类，与继承不同)。
5. 不同的类按照编译先后顺序调用 +load 方法（先编译，先调用)。
6. 分类的按照编译先后顺序调用 +load方法（先编译，先调用）。

## 特殊情况，如果自己主动调用 [super load];
1. 在 son 的 load 里面调用 [super load]时：
// 示例 1:
💭💭💭 Father --> self == Father, functionString == +[Father load]
// 此处多执行了一次 Father 里面的 load，且 self 传的是 Son
// 如果 Father 的分类也实现了 load，则会去分类里面执行
💭💭💭 Father --> self == Son, functionString == +[Father load]
💭💭💭 Son --> self == Son, functionString == +[Son load]
💭💭💭 GrandSon --> self == GrandSon, functionString == +[GrandSon load]

// 示例 2: 在 Son 的 load 里面调用 [super load]，且 Father 的分类里面实现了 load
💭💭💭 Father --> self == Father, functionString == +[Father load]
// 看到 这里插入了一条 Father 分类的执行，且 self == son
💭💭💭 Father (Test) --> self == Son, functionString == +[Father(Test) load]
💭💭💭 Son --> self == Son, functionString == +[Son load]
💭💭💭 GrandSon --> self == GrandSon, functionString == +[GrandSon load]

// 示例 3: 在 Son 的 load 里面调用 [self load]，且 Son 的分类也实现了 lod，则会执行分类里的
// 看到先去 Son 分类里面执行
💭💭💭 Son (Test) --> self == Son, functionString == +[Son(Test) load]
💭💭💭 Son --> self == Son, functionString == +[Son load]

// 示例 4: 当如果 Son 分类没有实现，且只有 Son 自己实现 load，且在 load 里面调 [self load] 会崩溃。
warning: could not execute support code to read Objective-C class data in the process. This may reduce the quality of type information available.

// 示例 5: 如果 Father 有两个分类都实现了 load，都会执行，且谁编译在前谁先执行
💭💭💭 Father (Test2) --> self == Son, functionString == +[Father(Test2) load]
💭💭💭 Father (Test) --> self == Father, functionString == +[Father(Test) load]

// 示例 6: 如果在 Son 里面执行 [super load]，且 Father 两个分类都实现了 load，则会去后编译的那个里面执行
💭💭💭 Father (Test2) --> self == Son, functionString == +[Father(Test2) load]
💭💭💭 Son --> self == Son, functionString == +[Son load]

上面这个顺序是能猜到了，这个涉及到同一个类的多个分类中添加了同样的方法，此时如果去执行，后编译的分类的方法会添加到方法列表的前面。让后方法执行时去方法列表里面找执行，当找到第一个后编译的方法后就直接返回执行了。

load 的源码在 objc4 中分析。

#  Blocks
**block 捕获的对象类型变量，在block 结构体中有个对应的对象类型指针，一直指向该对象类型的实例。**
**__block 结构体实例的对象类型的成员变量作为一个指针，一直指向该对象的实例。**

**堆区地址小于栈区地址，基本类型存在栈区**

// 这里

```
// 示例 1：
NSObject *obj = [[NSObject alloc] init];
__block NSObject *object = obj;
NSLog(@"⛈⛈⛈ obj retainCount = %lu", (unsigned long)[obj arcDebugRetainCount]);
// 打印：
⛈⛈⛈ obj retainCount = 2 // 被 obj 和 object 持有

// 示例 2:
__block NSObject *object = [[NSObject alloc] init];
NSLog(@"⛈⛈⛈ object retainCount = %lu", (unsigned long)[object arcDebugRetainCount]);
// 打印：
⛈⛈⛈ object retainCount = 1 // 只被 object 持有

// 示例 3:
__block NSObject *object = [[NSObject alloc] init];
^{
    NSLog(@"%@", object);
};

NSLog(@"⛈⛈⛈ object retainCount = %lu", (unsigned long)[object arcDebugRetainCount]);
// 打印：
⛈⛈⛈ object retainCount = 1 // 只被 object 持有，栈区的 block 持有 object 结构体 

// 示例 4：对比上面不用 __block 修饰，引用为 2，一次被变量 object 持有，一次被 block 持有
// 接下来为了区分变量在栈区还是堆区，打印它们的地址方便比较
NSObject *object = [[NSObject alloc] init];
NSLog(@"⛈⛈⛈ object retainCount = %lu ---%p---%p", (unsigned long)[object arcDebugRetainCount], object, &object);
^{
    NSLog(@"⛈⛈⛈ object retainCount = %lu ---%p---%p", (unsigned long)[object arcDebugRetainCount], object, &object);
}();

NSLog(@"⛈⛈⛈ object retainCount = %lu ---%p---%p", (unsigned long)[object arcDebugRetainCount], object, &object);
// 打印：这里有个细节，看三次 object 打印地址是相同的，都指向原始的 NSObject 对象，这没有问题，
// 第二行 block 内部的打印 &object 地址不同与上下两次，因为这个 object 是 block 结构体的 object 成员变量的地址
// 上下两次还是栈中的 object 变量
⛈⛈⛈ object retainCount = 1 ---0x102800750---0x7ffeefbff578
⛈⛈⛈ object retainCount = 2 ---0x102800750---0x7ffeefbff560
⛈⛈⛈ object retainCount = 2 ---0x102800750---0x7ffeefbff578

// 示例 5：
 __block NSObject *object = [[NSObject alloc] init];
 NSLog(@"⛈⛈⛈ object retainCount = %lu ---%p---%p", (unsigned long)[object arcDebugRetainCount], object, &object);
^{
    NSLog(@"⛈⛈⛈ object retainCount = %lu ---%p---%p", (unsigned long)[object arcDebugRetainCount], object, &object);
}();

NSLog(@"⛈⛈⛈ object retainCount = %lu ---%p---%p", (unsigned long)[object arcDebugRetainCount], object, &object);

// 打印：都是 1 ，只被 __block 变量 object 持有，在栈 Block中，自始只有强指针指向 object，就是__block生成的结构体。
// 三次地址完全相同，这里涉及到 __block 变量对应的结构体中的 __forwarding 指针
// 上下是：
// &(object.__forwarding->object)
// 中间 block 执行是：
&(object->__forwarding->object)
// 至此取的的 object 就是原始的 NSObject 对象
⛈⛈⛈ object retainCount = 1 ---0x102802820---0x7ffeefbff578
⛈⛈⛈ object retainCount = 1 ---0x102802820---0x7ffeefbff578
⛈⛈⛈ object retainCount = 1 ---0x102802820---0x7ffeefbff578

// 示例 6:
 __block NSObject *object = [[NSObject alloc] init];
 NSLog(@"⛈⛈⛈ object retainCount = %lu ---%p---%p", (unsigned long)[object arcDebugRetainCount], object, &object);
void (^aBlock)() = ^{
    NSLog(@"⛈⛈⛈ object retainCount = %lu ---%p---%p", (unsigned long)[object arcDebugRetainCount], object, &object);
};
aBlock();

NSLog(@"⛈⛈⛈ object retainCount = %lu ---%p---%p", (unsigned long)[object arcDebugRetainCount], object, &object);
// 打印：

// 极其重要的一句: "并断开栈中的obj结构体对obj对象的指向" "并断开栈中的obj结构体对obj对象的指向" "并断开栈中的obj结构体对obj对象的指向" 

可以看到，obj的内存地址一直在栈中，而执行了BlockCopy后，obj指针的地址从栈变到了堆中，而obj的引用计数一直是1。在copy操作之后，结构体obj也被复制到了堆中，并断开栈中的obj结构体对obj对象的指向。那如果这个时候取栈中的obj不就有问题了？__forwarding就派上用场了，上面编译的结果发现，结构体对象在使用obj的时候会使用obj->__forwarding->obj，如果所有__forwarding都指向自己，这一步还有什么意义？栈Block在执行copy操作后，栈obj结构体的__forwarding就会指向copy到堆中的obj结构体。此时再取值，操作的就是同一份指针了。证明如下:

// 示例 7：
__block NSObject *object = [[NSObject alloc] init];
NSLog(@"⛈⛈⛈ object retainCount = %lu ---%p---%p", (unsigned long)[object arcDebugRetainCount], object, &object);

void (^aBlock)() = ^{
    NSLog(@"⛈⛈⛈ object retainCount = %lu ---%p---%p", (unsigned long)[object arcDebugRetainCount], object, &object);
};

aBlock();
void (^bBlock)() = [aBlock copy];
bBlock();
aBlock();
NSLog(@"⛈⛈⛈ object retainCount = %lu ---%p---%p", (unsigned long)[object arcDebugRetainCount], object, &object);

// MRC 下打印：
⛈⛈⛈ object retainCount = 1 ---0x10065bd50---0x7ffeefbff570 // 原始状态: 堆区 栈区
⛈⛈⛈ object retainCount = 1 ---0x10065bd50---0x7ffeefbff570 // 堆区 栈区 // 这里虽然发生了赋值操作，但是并没有主动被复制到堆区

⛈⛈⛈ object retainCount = 1 ---0x10065bd50---0x1010083f8 // 堆区 堆区 // 这里开始主动调用了 copy 才被复制到堆区
⛈⛈⛈ object retainCount = 1 ---0x10065bd50---0x1010083f8 // 堆区 堆区
⛈⛈⛈ object retainCount = 1 ---0x10065bd50---0x1010083f8 // 堆区 堆区

// ARC 下打印：
⛈⛈⛈ object retainCount = 1 ---0x1006002e0---0x7ffeefbff578  // 原始状态: // 堆区 栈区
⛈⛈⛈ object retainCount = 1 ---0x1006002e0---0x1007396e8 // 堆区 堆区 // 这里发生了赋值操作，__block 变量被复制到堆区
⛈⛈⛈ object retainCount = 1 ---0x1006002e0---0x1007396e8 // 堆区 堆区
⛈⛈⛈ object retainCount = 1 ---0x1006002e0---0x1007396e8 // 堆区 堆区
⛈⛈⛈ object retainCount = 1 ---0x1006002e0---0x1007396e8 // 堆区 堆区

由上可知，在copy之前，aBlock的打印结果都是初始化生成的指针，而copy之后打印就和bBlock的打印结果相同了。总结一下就是，在栈中的obj结构体__forwarding指向的就是栈中的自己，执行copy之后，会在堆中生成一份obj结构体并断开栈中对obj的引用，此时堆中的obj结构体__forwarding就指向自己，而栈中的__forwarding就指向堆中的obj结构体。下面也会通过分析源码来具体解释。

__block NSObject *object = [[NSObject alloc] init];
NSLog(@"⛈⛈⛈ object retainCount = %lu ---%p---%p", (unsigned long)[object arcDebugRetainCount], object, &object); // 堆区 栈区

void (^aBlock)() = ^{
    NSLog(@"⛈⛈⛈ object retainCount = %lu ---%p---%p", (unsigned long)[object arcDebugRetainCount], object, &object);
};

NSObject *temp = object; // + 1
aBlock(); // 堆区 堆区
void (^bBlock)() = [aBlock copy];
bBlock(); // 堆区 堆区
aBlock(); // 堆区 堆区
NSLog(@"⛈⛈⛈ object retainCount = %lu ---%p---%p", (unsigned long)[object arcDebugRetainCount], object, &object); // 堆区 堆区
// 打印：
⛈⛈⛈ object retainCount = 1 ---0x10053e1b0---0x7ffeefbff578
⛈⛈⛈ object retainCount = 2 ---0x10053e1b0---0x10053e988
⛈⛈⛈ object retainCount = 2 ---0x10053e1b0---0x10053e988
⛈⛈⛈ object retainCount = 2 ---0x10053e1b0---0x10053e988
⛈⛈⛈ object retainCount = 2 ---0x10053e1b0---0x10053e988
```


**堆 Block __NSMallocblock__ 内存由 ARC 控制，没有强指针指向时释放。而在 MRC 中，赋值不会执行 copy 操作，所以左侧 block 依然存在于栈中，所以在 MRC 中一般都需要执行 copy，否则很容易造成 crash.在 ARC 中，当 Block 作为属性被 strong、copy 修饰或被强指针引用或作为返回值时，都会默认执行 copy。而 MRC 中，只有被 copy 修饰时，block 才会执行 copy。所以 MRC 中 Block 都需要用 copy 修饰，而在 ARC 中用 copy 修饰只是沿用了 MRC 的习惯，此时用 copy 和 strong效果是相同的。**

**Block 在捕获外部变量的操作基本一致，都是在生成结构体的时候将所有 Block 里用到的外部变量作为属性保存起来。self.block 里面调用 self 会造成循环引用，因为 Block 捕获了 self 并把 self 当做一个值保存了起来。**

**Block里的a只是copy过去的a的值，在Block里改变a的值也不会影响外面，编译器避免这个错误就报错。**

**同样的，捕获对象时也是对指针的copy，生成一个指针指向obj对象，所以如果在Block中直接让obj指针指向另外一个对象也会报错。这点编译器已经加了注释 // bound by copy。**

**多了__main_block_copy_0和__main_block_dispose_0这两个函数，并在描述__main_block_desc_0结构体中保存了这两个函数指针。这就是上面所说的copy_dispose助手，C语言结构体中，编译器很难处理对象的初始化和销毁操作，所以使用runtime来管理相关内存。BLOCK_FIELD_IS_OBJECT是在捕获对象时添加的特别标识，此时是3，下面会细讲。**

**此Block是为栈Block_NSConcreteStackBlock，不过在ARC中，因为赋值给aBlock，会执行一次copy，将其中栈中copy到堆中，所以在MRC中aBlock为_NSConcreteStackBlock，在ARC中就是_NSConcreteMallocBlock。**


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

// 示例 5:
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
// 运行结果：
⛈⛈⛈ b = 10
⛈⛈⛈ block 内部 b 修改前: b = 10
⛈⛈⛈ b = 20
⛈⛈⛈ a = 30

// 示例 6:
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
// 运行结果:
⛈⛈⛈ b = 10
⛈⛈⛈ b = 30
⛈⛈⛈ block 内部 b 修改前: b = 30
⛈⛈⛈ b = 20
⛈⛈⛈ a = 20
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
  // 实质是因为 C 语言规定，数组不能直接赋值，可用 char* 代替
  printf("%c\n", text[0]);
}; 
```

> 向截获的 NSMutableArray 变量赋值会产生编译错误。源码中截获的变量值为 NSMutableArray 类的对象，如果用 C 语言来描述，即是截获 NSMutableArray 类对象用的结构体实例指针。

## Block 的实质
Block 是 “带有自动变量的匿名函数”，但 Block 究竟是什么呢？
语法看上去很特别，但它实际上是作为**极普通的 C 语言源码**来处理的。通过**支持 Block 的编译器**，含有 Block 语法的源代码转换为一般 C 语言编译器能够处理的源代码，并作为极为普通的 C 语言源代码被编译。
这不过是概念上的问题，在实际编译时无法转换成我们能够理解的源代码，但 clang(LLVM 编译器)具有转换为我们可读源代码的功能。通过 "-rewrite-objc" 选项就能将含有 Block 语法的源代码转换为 C++ 的源代码。说是 C++，其实也**仅仅是使用了 struct 结构，其本质是 C 语言源代码**。

+ 名字中的impl即 implementation 的缩写，换句话说这一部分是 block 的实现部分结构体
+ void *isa：
+ + C语言中void * 为 “不确定类型指针”，void *可以用来声明指针。
+ + 看到isa就会联想到之前在objc_class结构体，因此我们的block本质上也是一个对象【而且是个类对象】
+ + 我们知道实例对象->类对象->元类构成了isa链中的一条，而这个__block_impl结构体占据的是中间类对象的位置
+ + 实例对象应该是生成的 block 变量，个人认为
+ + 因此这里的 isa 指针会指向元类，这里的元类主要是为了说明这个块的存储区域【详见：Block 存储域 &&Block 元类】
+ + int Flags：
+ + 标识符，在实现block的内部操作时会用到
+ + int Reserved：
+ + 注明今后版本升级所需区域大小 Reserved
+ + 一般就是填个0
+ + void *FuncPtr：
+ + 函数指针
+ **实际执行的函数，也就是 block 中花括号里面的代码内容，最后是转化成一个 C 语言函数执行的**
+ 具体内容就是对 impl 中相应的内容进行赋值，要说明的是 impl.isa = &_NSConcreteStackBlock 这个参看 Block 存储域 && Block 元类

+ 我们的 fmt，val 这两个被 block 截获的自动变量被放入到该结构体当中，同时构造函数也发生了变化，构造时要给fmt，val赋值
+ 这里我们就能大概猜出截获自动变量的原理了，自动变量会被存入 block 结构体
+ 在这里也要注意我们等于是使用了一个长得一模一样，保存在结构体里的数来进行的赋值操作，所以我们不能对它进行赋值操作，因为我们操作的只能是我们自己建的数据，而不会是我们真正的变量

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
 
 虽然 Block 语法的匿名函数部分简单转换为了 C  语言函数，但从这个变换的函数中访问 **静态全局变量/全局变量** 并没有任何改变，可直接使用。
 &ensp;**但是静态变量的情况下，转换后的函数原本就设置在含有 Block 语法的函数外**，所以无法从变量作用域访问。
 
 **这里的静态变量的访问，作用域之外，应该深入思考下，虽然代码写在了一起，但是转换后并不在同一个作用域，能跨域请求只能靠指针。！！！！**
 
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

         NSLog((NSString *)&__NSConstantStringImpl__var_folders_24_5w9yv8jx63bgfg69gvgclmm40000gn_T_main_54420a_mi_0);
         
         // static_val 初始化
         static int static_val = 3;
         
         void (*blk)(void) = ((void (*)())&__main_block_impl_0((void *)__main_block_func_0, &__main_block_desc_0_DATA, &static_val));
         
         // 这里的赋值只是赋值，可以和 __block 的 forwarding 指针方式寻值进行比较思考
         static_val = 12;
         
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

**原因在于，我们的静态变量是存在数据区的，在程序结束前它其实一直都会存在，之所以会被称为局部，只是说出了作用域无法调用到它了，并不是说这块数据不存在了。因此我们只要自己准备好一个指针，保证出了作用域依然能调用到他就行；而对于自动变量，它们真正的问题在于一但出了作用域，直接被释放了，所以要在结构体里开辟空间重新存放，进行值传递**

2. 第二种是使用 "__block 说明符"。更准确的表达方式为 "__block 存储域说明符"（__block storage-class-specifier）。
&ensp;C 语言中有以下存储域类说明符:
+ typedef
+ extern
+ static
+ auto
+ register
__block 说明符类似于 static、auto 和 register 说明符，他们用于指定将变量设置到哪个存储域中。例如: `auto` 表示作为自动变量存储在栈中，`static` 表示作为静态变量存储在数据区中。

**对于使用__block修饰的变量，不管在块里有没有使用，都会相应的给他生成一个结构体**

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
如果 __block 修饰的是对象类型时，会多两个函数指针类型的成员变量: `__Block_byref_id_object_copy`  `__Block_byref_id_object_dispose` 。
```
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
`__block_impl`，作为一个被复用结构体，保持不变
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
  // 但是在初始化列表中用的是 val(_val->forwarding 指针)
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
**这里看到我们用 val 截获下来的就是一个 __Block_byref_val_0 结构体了，对它进行赋值的时候需要通过 forwarding 指针进行**

**1. 已知：当 Block 截获对象类型变量时（如 NSObject NSMutableArray）会有如下的 copy 和 dispose 函数**
**2. 当使用 __block 变量时会有如下的 copy 和 dispose 函数**
**3. 当函数返回值和参数类型都是 Block 类型时也会有如下的 copy 和 dispose 函数**

第一次出现的： `__main_block_copy_0`, //  `BLOCK_FIELD_IS_BYREF` 后面研究 
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
 
 // 看到对象类型的多了两个成员变量
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
            
            // 这里函数执行有一手，用的 ->__forwarding->fmt 去找值
            // 保证不管是堆区还是栈区执行函数，找到的一个值都是堆上的或者栈上的，
            // 就是大家操作的都是同一个地址的数据
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
    
    // 需要到 libclosure74 里面看源码
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
OC 对象类型用 __block 修饰时的情况单独拿出来说一下:
```
// object
__attribute__((__blocks__(byref))) __Block_byref_object_4 object = {(void*)0,(__Block_byref_object_4 *)&object, 33554432, sizeof(__Block_byref_object_4), __Block_byref_id_object_copy_131, __Block_byref_id_object_dispose_131, ((NSObject *(*)(id, SEL))(void *)objc_msgSend)((id)((NSObject *(*)(id, SEL))(void *)objc_msgSend)((id)objc_getClass("NSObject"), sel_registerName("alloc")), sel_registerName("init"))};

// 简化后
__Block_byref_object_4 object = {
(void*)0, // isa
(__Block_byref_object_4 *)&object, // __forwarding
33554432, // __flags
sizeof(__Block_byref_object_4), // __size
__Block_byref_id_object_copy_131, // __Block_byref_id_object_copy
__Block_byref_id_object_dispose_131, // __Block_byref_id_object_dispose
((NSObject *(*)(id, SEL))(void *)objc_msgSend)((id)((NSObject *(*)(id, SEL))(void *)objc_msgSend)((id)objc_getClass("NSObject"), sel_registerName("alloc")), sel_registerName("init")) // obj
}
```
+ __flags = 33554432 即二进制的 10000000000000000000000000 即 1 << 25，BLOCK_HAS_COPY_DISPOSE = (1 << 25), // compiler 译：compiler 含有 copy_dispose 助手【即拥有 copy 和 dispose 函数】


## Block 存储域
&ensp;通过前面的研究可知，Block 转换为 Block 的结构体类型的自动变量，__block 变量转换为 __block 变量的结构体类型的自动变量。所谓结构体类型的自动变量，即**栈上生成的该结构体的实例**。如表:
|  名称  |  实质  |
|  -----  |  -----  |
| Block |  栈上 Block 的结构体实例  |
| __block 变量  | 栈上 __block 变量的结构体实例 |

通过之前的说明可知 **Block 也是 OC 对象**。将 Block 当作 OC 对象来看时，该 Block 的类为 `_NSConcreteStackBlock`。同时还有 `_NSConcreteGlobalBlock`、`_NSConcreteMallocBlock`。 由名称中含有 `stack` 可知，该类的对象 Block 设置在栈上。同样由 `global` 可知，与全局变量一样，设置在程序的数据区域（.data 区）中。`malloc` 设置在由 `malloc` 函数分配的内存块（即堆）中。
|类|设置对象的存储域|
|---|---|
|_NSConcreteStackBlock|栈|
|_NSConcreteGlobalBlock|程序的数据区域(.data 区)|
|_NSConcreteMallocBlock|堆|

应用程序的内存分配
1. 程序区域 .text 区
2. 数据区域 .data 区
3. 堆
4. 栈

**在记述全局变量的地方使用 Block 语法**时，生成的 Block 为 _NSConcreteGlobalBlock 类对象。

**isa 是靠 runtime 动态确定的，不能通过转换代码看出**

如下:
```
void (^blk)(void) = ^{ printf("全局区的 _NSConcreteGlobalBlock Block！\n"); };

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // insert code here...
        NSLog(@"🎉🎉🎉 Hello, World!");
        
        blk();
        
        NSLog(@"❄️❄️❄️ block isa: %@", blk);
    }
}

// 打印:
全局区的 _NSConcreteGlobalBlock Block！
❄️❄️❄️ block isa: <__NSGlobalBlock__: 0x100002068>

// 转换后:

// 命名都是以 __blk 开始的，对应该全局 Block 的名字 blk
// __blk_block_impl_0
struct __blk_block_impl_0 {
  struct __block_impl impl;
  struct __blk_block_desc_0* Desc;
  
  // 构造函数
  __blk_block_impl_0(void *fp, struct __blk_block_desc_0 *desc, int flags=0) {
    // isa 指向了 _NSConcreteGlobalBlock 
    impl.isa = &_NSConcreteGlobalBlock;
    impl.Flags = flags;
    impl.FuncPtr = fp;
    Desc = desc;
  }
};

// __blk_block_func_0
static void __blk_block_func_0(struct __blk_block_impl_0 *__cself) {
 printf("全局区的 _NSConcreteGlobalBlock Block！\n"); 
}

// __blk_block_desc_0
static struct __blk_block_desc_0 {
  size_t reserved;
  size_t Block_size;
} __blk_block_desc_0_DATA = { 0, sizeof(struct __blk_block_impl_0)};

// 创建 __blk_block_impl_0 实例
static __blk_block_impl_0 __global_blk_block_impl_0((void *)__blk_block_func_0, &__blk_block_desc_0_DATA);

// blk 
void (*blk)(void) = ((void (*)())&__global_blk_block_impl_0);

int main(int argc, const char * argv[]) {
    /* @autoreleasepool */ { __AtAutoreleasePool __autoreleasepool; 

        NSLog((NSString *)&__NSConstantStringImpl__var_folders_24_5w9yv8jx63bgfg69gvgclmm40000gn_T_main_b56a4a_mi_0);

        // 调用 blk
        ((void (*)(__block_impl *))((__block_impl *)blk)->FuncPtr)((__block_impl *)blk);
    }

    return 0;
}
```
此 Block 即该 Block 用结构体实例设置在程序的数据区域中。因为在使用全局变量的地方不能使用自动变量，所以不存在对自动变量进行截获。由此 Block 用结构体实例的内容不依赖于执行时的状态，所以整个程序中只需要一个实例。因此将 Block 用结构体实例设置在与全局变量相同的数据区域中即可。

&ensp;只在截获自动变量时，Block 用结构体实例截获的值才会根据执行时的状态变化。

**也就是说，即使在函数内而不在记述广域变量的地方使用 Block 语法时，只要 Block 不截获自动变量，就可以将 Block 用结构体实例设置在程序的数据区域。**
```
// 当前在 main 函数内:
// 不捕获外部自动变量
void (^globalBlock)(void) = ^{
    NSLog(@"❄️❄️❄️ 测试 block isa");
};

globalBlock();
NSLog(@"❄️❄️❄️ block isa: %@", globalBlock);

// 打印:
❄️❄️❄️ 测试 block isa
❄️❄️❄️ block isa: <__NSGlobalBlock__: 0x100002088>
```

**对于没有要截获自动变量的 block，我们不需要依赖于其运行时的状态【捕获的变量】，这样我们就不涉及到 block 的 copy 情况，因此是放在数据区。**

**此外要注意的是，通过 clang 编译出来的 isa 在第二种情况下会显示成 stackblock，这是因为 OC 是一门动态语言，真正的元类还是在运行的情况下确定的，这种情况下可以使用 lldb 调试器查看。**

**虽然通过 clang 转换的源代码通常是 _NSConcreteStackBlock 类对象，但实现上却有不同。总结如下:**

+ 记述全局变量的地方有 Block 语法时
+ Block 语法的表达式中不使用截获的自动变量时

以上情况下，Block 为 `_NSConcreteGlobalBlock` 类对象，即 Block 配置在程序的数据区域中。除此之外 Block 语法生成的 Block 为 _NSConcreteStackBlock 类对象，且设置在栈上。

```
// 不捕获外部自动变量是 global
void (^globalBlock)(void) = ^{
    NSLog(@"❄️❄️❄️ 测试 block isa");
};

int a = 2;
// 右边栈区 block 赋值给左侧 block 时，会被复制到堆区
void (^mallocBlock)(void) = ^{
    NSLog(@"❄️❄️❄️ 测试 block isa a = %d", a);
};

globalBlock();
mallocBlock();

NSLog(@"❄️❄️❄️ globalBlock isa: %@", globalBlock);
NSLog(@"❄️❄️❄️ mallocBlock isa: %@", mallocBlock);
// 栈区 block
NSLog(@"❄️❄️❄️ stackBlock isa: %@", ^{ NSLog(@"❄️❄️❄️ a = %d", a); });

// 打印：
❄️❄️❄️ 测试 block isa
❄️❄️❄️ 测试 block isa a = 2
❄️❄️❄️ globalBlock isa: <__NSGlobalBlock__: 0x100002088>
❄️❄️❄️ mallocBlock isa: <__NSMallocBlock__: 0x100540fa0>
❄️❄️❄️ stackBlock isa: <__NSStackBlock__: 0x7ffeefbff4e0>
```
 配置在全局变量上的 Block ，从变量作用域外也可以通过指针安全的使用，但设置在栈上的 Block，如果其所属的变量作用域结束，该 Block 就被废弃。由于 __Block 变量也配置在栈上，同样的，如果其所属的变量作用域结束，则该 __block 变量也会被废弃。
 
 示例代码:
 ```
 // block 不持有 object2
 void (^blk)(void);
 {
     NSObject *object = [[NSObject alloc] init];
     NSObject * __weak object2 = object;
     // 右边栈区 block 被复制到堆区
     // 弱引用 object2, 出了下面花括号，object 被释放废弃，object2 被置为 nil 
     blk = ^{
         NSLog(@"object2 = %@", object2);
     };
 }
 blk();
 //打印：
 object2 = (null)
 
 // block 持有 object
 void (^blk)(void);
 {
     NSObject *object = [[NSObject alloc] init];
     // NSObject * __weak object2 = object;
     // 出了花括号 object 依然存在，因为它被 blk 强引用
     blk = ^{
         NSLog(@"object = %@", object);
     };
 }
 blk();
 // 打印：
 object = <NSObject: 0x10059cee0>
 ```
 
 &ensp;Blocks 提供了将 Block 和 __block 变量从栈上复制到堆上的方法来解决这个问题。将配置在栈上的 Block 复制到堆上，这样即使 Block 语法记述的变量作用域结束，堆上的 Block 还可以继续存在。

+ 不会有任何一个块一上来就被存在堆区，请牢记这一点！
+ `_NSConcreteMallocBlock` 存在的意义和 `autorelease` 一样，就是为了能延长 block 的作用域
+ 我们将 block 对象和 __blcok 对象从栈区复制到堆区，这样就算栈上的 block 被废弃了，还是可以使用堆上那一个
+ 可以联想我们在 ARC 是如何处理返回值中的 __strong 的，大概同理

**在这里要思考一个问题：在栈上和堆上同时有一个 block 的情况下，我们的赋值，修改，废弃操作应该怎样管理？**

复制到堆上的 Block isa 会指向 _NSConcreteMallocBlock，即 impl.isa = &_NSConcreteMallocBlock;

**__block 变量用结构体成员变量 __forwarding 可以实现无论 __block 变量配置在栈上还是堆上时都能够正确地访问 __block 变量。**

**有时在 __block 变量配置在堆上的状态下，也可以访问栈上的 __block 变量。只要栈上的结构体实例成员变量 __forwarding 指向堆上的结构体实例，那么不管是从栈上的 __block 变量还是从堆上的 __block 变量都能够正确访问。**

// 代码示例：
```
// 这个 a 是位于栈区 __Block_byref_a_0 结构体实例，已经不是 int 类型
__block int a = 2;

// 下面 block 被复制到堆区，a 也同时被复制到 堆区
void (^mallocBlock)(void) = ^{
    // a->__forwarding->a 自增
    // 堆上 a 的 __forwarding 指向自己
    ++a;
    NSLog(@"❄️❄️❄️ 测试 block isa a = %d", a);
};

// 下面的 a 还是在栈区的 __Block_byref_a_0 结构体实例，
// 但是它的 __forwrding 指针是指向上面被复制堆区的 a 的，
// 这样不管是栈区 a 还是 堆区 a，当操作 int a = 2，这个数值 a 时都是同一个。
++a;
```

Blocks 提供的复制方法究竟是什么呢？实际上 ARC  有效时，大多数情形下编译器会恰当的进行判断，自动生成将 Block 从栈复制到堆上的代码。

> 碰到两个问题，都是用中间变量接一下就正常了：
```
// 问题一：
// 用 clang -rewrite-objc 能转换成功
typedef int(^BLK)(int);

BLK func(int rate) {
    // 右边栈区 block 复制到堆区，并被 temp 持有
    BLK temp = ^(int count){ return rate * count; };
    return temp;
}

// 用 clang -rewrite-objc 转换失败，改成上面就会成功，（用中间变量接收一下）
typedef int(^BLK)(int);

BLK func(int rate) {
    // 此时直接返回栈区 block 不行 
    return ^(int count){ return rate * count; };
}

// 失败描述，转换失败，但是执行该函数是正常的
returning block that lives on the local stack
return ^(int count){ return rate * count; };
           ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
64 warnings and 1 error generated.

// 问题二:
BLK __weak blk;
{
    NSObject *object = [[NSObject alloc] init];
    
    // NSObject * __weak object2 = object;
    
    void (^strongBlk)(void) = ^{
        NSLog(@"object = %@", object);
    };
    
    // blk 是一个弱引用变量，用一个 strong 赋值给他，
    // 它不持有该 strong 变量
    blk = strongBlk;
}

// blk();
printf("blk = %p\n", blk);
// 打印正常，出了花括号，block 结构体实例释放了:
blk = 0x0

BLK __weak blk;
{
    NSObject *object = [[NSObject alloc] init];
    // NSObject * __weak object2 = object;
    // void (^strongBlk)(void) = ^{
    // NSLog(@"object = %@", object);
    // };

    // 这里给了警告: 
    // Assigning block literal to a weak variable; object will be released after assignment
    blk = ^{
        NSLog(@"object = %@", object);
    };
    printf("内部 blk = %p\n", blk);
}

// blk();
printf("blk = %p\n", blk);
// 打印：出了花括号，还是打印了 blk 不为 0x0
内部 blk = 0x7ffeefbff538
blk = 0x7ffeefbff538
```
看一下下面这个返回 Block 的函数:
```
typedef int (^blk_t)(int);
blk_t func(int rate) {
    return ^(int count) {
        return rate * count;
    };
}
```
源代码为返回配置在栈上的 Block 的函数。即程序执行中从 **该函数** 返回 **函数调用方** 时变量作用域结束，因此栈上的 Block 也被废弃。虽然有这样的问题，但是该源代码通过对应 ARC 的编译器可转换为如下:
```
blk_t func(int rate) {
blk_t tmp = &__func_block_impl_0(__func_block_func_0, &__func_block_desc_0_DATA, rate);

tmp = objc_retainBlock(tmp);

return objc_autoreleaseReturnValue(tmp);
}
```
另外，因为 ARC 处于有效状态，所以 `blk_t tmp` 实际上与附有 `__strong` 修饰符的 `blk_t __strong tmp` 相同。
在 objc4 找到  `objc_retainBlock` 函数实际上就是 `Block_copy` 函数:
```
// 在 NSObject.mm 文件 31 行
//
// The -fobjc-arc flag causes the compiler to issue calls to objc_{retain/release/autorelease/retain_block}
//

id objc_retainBlock(id x) {
    return (id)_Block_copy(x);
}

// usr/include/Block.h 中找到
// Create a heap based copy of a Block or simply add a reference to an existing one.
// 创建基于堆的 Block 副本，或仅添加对现有 Block 的引用。（已经在堆上的 block 调用 copy 函数，引用计数增加）
// This must be paired with Block_release to recover memory, even when running under Objective-C Garbage Collection.
// 如果在 OC 的垃圾回收机制下使用时必须与 "Block_release" 配对使用。

BLOCK_EXPORT void *_Block_copy(const void *aBlock)
    __OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);

```
即:
```
tmp = _Block_copy(tmp);
return objc_autoreleaseReturnValue(tmp);
```
分析:
```
// 第一步，__func_block_impl_0 结构体实例生成
// 将通过 Block 语法生成的 Block， 
// 即配置在栈上的 Block 用结构体实例赋值给相当于 Block 类型的变量 tmp 中
blk_t tmp = &__func_block_impl_0(__func_block_func_0, &__func_block_desc_0_DATA, rate);

// 第二步，_Block_copy 函数执行
// _Block_copy 函数，将栈上的 Block 复制到堆上。
// 复制后，将堆上的地址作为指针赋值给变量 tmp。
tmp = _Block_copy(tmp);

// 第三步，将堆上的 Block 作为 OC 对象，
// 注册到 autoreleasepool 中，然后返回该对象
return objc_autoreleaseReturnValue(tmp);
```
**将 Block 作为函数返回值返回时，编译器会自动生成复制到堆上的代码。**

前面说大部分情况下编译器会适当的进行判断，不过在此之外的情况下需要**手动**（自己调用 copy 函数）生成代码，将 Block 从栈上复制到 **堆** （_Block_copy 函数的注释已经说了，它是创建基于堆的 block 副本）上，即我们自己主动调用 “copy” 实例方法。

**编译器不能进行判断究竟是什么样的状况呢？**

+ 向方法或函数的参数中传递 Block 时。
但是如果在方法或函数 **中** 适当的复制了传递过来的参数，那么就不必在调用该方法或函数前手动复制了。

以下方法或函数不用手动复制，编译器会给我们自动复制:

+ Cocoa 框架的方法且方法名中含有 usingBlock 等时
+ Grand Central Dispatch 的 API
+ 将 Block 赋值给类的附有 __strong 修饰符的 id 类型或 Block 类型成员变量时【当然这种情况就是最多的，只要赋值一个block变量就会进行复制】

NSArray 的 enumerateObjectsUsingBlock 以及 dispatch_async 函数就不用手动复制。
NSArray 的 initWithObjects 上传递 Block 时需要手动复制。

下面是个🌰：
```
id obj = [Son getBlockArray];
void (^blk)(void) = [obj objectAtIndex:0];
blk();

// 对 block 主动调用 copy 函数，能正常运行 
+ (id)getBlockArray {
    int val = 10;
    return [[NSArray alloc] initWithObjects:[^{NSLog(@"blk0: %d", val);} copy], [^{NSLog(@"blk1: %d", val);} copy], nil];
}

// 如下如果不加 copy 函数，则运行崩溃
+ (id)getBlockArray {
    int val = 10;
    return [[NSArray alloc] initWithObjects:^{NSLog(@"blk0: %d", val);}, ^{NSLog(@"blk1: %d", val);}, nil];
}

// 崩溃原因: 不主动调用 copy，getBlockArray 函数执行结束，栈上的 block 被废弃了
// 编译器对此种情况不能判断是否需要复制。
// 也可以不判断全部情况都复制，但是将 Block 从栈复制到堆是相当消耗 CPU 的。
// 当 Block 在栈上也能使用时，从栈上复制到堆上，就只是浪费 CPU 资源了。
// 此时需要我们判断，自行手动复制。
```
|Block 的类|副本源的配置存储域|复制效果|
|---|---|---|
|_NSConcreteStackBlock|栈|从栈复制到堆|
|_NSConcreteGlobalBlock|程序的数据区域|什么也不做|
|_NSConcreteMallocBlock|堆|引用计数增加|
不管 Block 配置在何处，用 copy 方法复制都不会引起任何问题，在不确定时调用 copy 方法即可。

## __block 变量存储域
使用 __block 变量的 Block 从栈复制到堆上时，__block 变量也会受到影响。
|__block 变量的配置存储域|Block 从栈复制到堆时的影响|
|---|---|
|栈|从栈复制到堆并被 Block 持有|
|堆|被 Block 持有|

若在一个 Block 中使用 __block 变量，当该 Block 从栈复制到堆时，使用的所有 __block 变量也必定配置在栈上，这些 __block 变量也全部被从栈复制到堆。此时，Block 持有 __block 变量，即使在该 Block 已复制到堆的情形下，复制 Block 也对所使用的 __block 变量没有任何影响。

**使用 __block 变量的 Block 持有 __block 变量。如果 Block被废弃，它所持有的 __block 变量也就被释放。**

回顾 __block 变量用结构体成员变量 __forwarding 的原因：**不管 __block 变量配置在栈上还是在堆上，都能够正确的访问该变量。**
通过 Block 的复制，__block 变量也从栈上复制到堆上。此时可同时访问栈上的 __block 变量和堆上的 __block 变量。

源代码如下:
```
__block int val = 0;

// 使用 copy 方法复制了使用 __block 变量的 Block 语法
// Block 和 __block 变量两者均从栈复制到堆 
// 在 Block 语法的表达式中使用初始化后的 __block 变量，做了自增运算
void (^blk)(void) = [^{++val;} copy];

// 在 Block 语法之后使用与 Block 无关的变量，
// 此时的 val 是第一行生成的 __block 变量，
// Block 语法表达式中使用的 val 是 Block 结构体自己的成员变量 val
// 两者之间毫无瓜葛，硬要说有关系的话，大概就是 Block 表达式里面的 val（指针）成员变量
// 在 Block 结构体初始化时初始化列表里面 val 初始化是用的:val(_val->__forwarding) { }

++val;

// 通过 clang 转换，看到两次自增运算均转换为如下形式:

// Block 表达式内部：
// 首先找到 Block 结构体实例的成员变量 val 
__Block_byref_val_0 *val = __cself->val; // bound by ref
// val 是结构体 __Block_byref_val_0 指针
++(val->__forwarding->val);

// 外部：
++(val.__forwarding->val);

blk();
// 且此行打印语句也是用的 val.__forwarding->val
NSLog(@"val = %d", val);
```

在变换 Block 语法的函数中，该变量 val 为复制到堆上的 __block 变量用结构体实例，而使用的与 Block 无关的变量 val，为复制前栈上的 __block 变量用结构体实例。

**超级重要的一句：**
**但是栈上的 __block 变量用结构体实例在 __block 变量从栈复制到堆上时，会将成员变量 __forwarding 的值替换为复制目标堆上的 __block 变量用结构体实例的地址**。

至此，无论是在 Block 语法中、Block 语法外使用 __block 变量，还是 __block 变量配置在栈上或堆上，都可以顺利的访问到同一个 __block 变量。

**所有使用 val 的地方实际都转化为了: val->__forwarding->val（block 内部）或者 val.__forwarding->val（外部，是结构体实例可以直接使用 .）。**

## 截获对象
示例代码:
```
void (^blk)(id);
{
    // id array = [[NSMutableArray alloc] init];
    id array = [NSMutableArray array];
    
    // 注意最后 block 调用了 copy 函数
    // 即使不调用也能正常执行
    // 原书是调用了 copy
    // 实际当右边的栈区赋值给左侧 blk 变量时，
    // 已经发生了 block 复制到堆区
    blk = [^(id obj){
        [array addObject:obj];
        
        NSLog(@"array count = %ld", [array count]);
    } copy];
}

blk([[NSObject alloc] init]);
blk([[NSObject alloc] init]);
blk([[NSObject alloc] init]);
// 打印：
array count = 1
array count = 2
array count = 3
```
意味着赋值给变量 array 的 NSMutableArray 类的对象在该源代码最后 Block 的执行部分超出其变量作用域而存在。

clang 转换后:
`_main_block_impl_0`
```
struct __main_block_impl_0 {
  struct __block_impl impl;
  struct __main_block_desc_0* Desc;
  // id 类型的 array 成员变量，截获外部 array 自动变量
  id array;
  
  // 构造函数 初始化列表 : array(_array)
  __main_block_impl_0(void *fp, struct __main_block_desc_0 *desc, id _array, int flags=0) : array(_array) {
    impl.isa = &_NSConcreteStackBlock;
    impl.Flags = flags;
    impl.FuncPtr = fp;
    Desc = desc;
  }
};
```
`__main_block_func_0`
```
static void __main_block_func_0(struct __main_block_impl_0 *__cself, id obj) {
// 取到 block 结构体实例的成员变量 array
id array = __cself->array; // bound by copy

// 调用 addObject 函数，参数是 id obj 
((void (*)(id, SEL, ObjectType _Nonnull))(void *)objc_msgSend)((id)array, sel_registerName("addObject:"), (id)obj);

// 打印 count
NSLog((NSString *)&__NSConstantStringImpl__var_folders_24_5w9yv8jx63bgfg69gvgclmm40000gn_T_main_b7ba78_mi_1, ((NSUInteger (*)(id, SEL))(void *)objc_msgSend)((id)array, sel_registerName("count")));
}
```

```
// __main_block_copy_0
// 3/*BLOCK_FIELD_IS_OBJECT*/ 这里表示捕获的是对象
// 如果是 block 截获 __block 变量时，
// 会是 8/*BLOCK_FIELD_IS_BYREF*/ 这里表示捕获的是引用

static void __main_block_copy_0(struct __main_block_impl_0*dst, struct __main_block_impl_0*src) {_Block_object_assign((void*)&dst->array, (void*)src->array, 3/*BLOCK_FIELD_IS_OBJECT*/);}

// __main_block_dispose_0
// 3/*BLOCK_FIELD_IS_OBJECT*/ 这里表示捕获的是对象
// 如果是 block 截获 __block 变量时，
// 会是 8/*BLOCK_FIELD_IS_BYREF*/ 这里表示捕获的是引用

static void __main_block_dispose_0(struct __main_block_impl_0*src) {_Block_object_dispose((void*)src->array, 3/*BLOCK_FIELD_IS_OBJECT*/);}

static struct __main_block_desc_0 {
  size_t reserved;
  size_t Block_size;
  void (*copy)(struct __main_block_impl_0*, struct __main_block_impl_0*);
  void (*dispose)(struct __main_block_impl_0*);
} __main_block_desc_0_DATA = { 0, sizeof(struct __main_block_impl_0), __main_block_copy_0, __main_block_dispose_0};
```
main 函数内:
```
void (*blk)(id);
{
    // 创建 array 对象
    id array = ((NSMutableArray * _Nonnull (*)(id, SEL))(void *)objc_msgSend)((id)objc_getClass("NSMutableArray"), sel_registerName("array"));
    
    // block 定义，创建 block 结构体实例时，传入 array
    blk = (void (*)(id))((id (*)(id, SEL))(void *)objc_msgSend)((id)((void (*)(id))&__main_block_impl_0((void *)__main_block_func_0, &__main_block_desc_0_DATA, array, 570425344)), sel_registerName("copy"));
}

// 下面是三次 block 执行，（创建 object 对象）
((void (*)(__block_impl *, id))((__block_impl *)blk)->FuncPtr)((__block_impl *)blk, ((NSObject *(*)(id, SEL))(void *)objc_msgSend)((id)((NSObject *(*)(id, SEL))(void *)objc_msgSend)((id)objc_getClass("NSObject"), sel_registerName("alloc")), sel_registerName("init")));

((void (*)(__block_impl *, id))((__block_impl *)blk)->FuncPtr)((__block_impl *)blk, ((NSObject *(*)(id, SEL))(void *)objc_msgSend)((id)((NSObject *(*)(id, SEL))(void *)objc_msgSend)((id)objc_getClass("NSObject"), sel_registerName("alloc")), sel_registerName("init")));

((void (*)(__block_impl *, id))((__block_impl *)blk)->FuncPtr)((__block_impl *)blk, ((NSObject *(*)(id, SEL))(void *)objc_msgSend)((id)((NSObject *(*)(id, SEL))(void *)objc_msgSend)((id)objc_getClass("NSObject"), sel_registerName("alloc")), sel_registerName("init")));
```
1.3.4 节中，在 OC 中，C 语言结构体不能含有附有 __strong 修饰符的变量。因为编译器不知道应何时进行 C 语言结构体的初始化和废弃操作，不能很好地管理内存。
但是 OC  运行时库能准确的把握 Block 从栈复制到堆以及堆上的 Block 被废弃的时机，因此 Block 用结构体中即使含有附有 __strong 修饰符或者 __weak 修饰符的变量，也可以恰当的进行初始化和废弃。为此需要 __main_block_copy_0 和 __main_block_dispose_0 函数，并把他们放在了 __main_block_desc_0 结构体的成员变量 copy 和 dispose 中。
__main_block_copy_0 函数使用 `_Block_object_assign` 函数将对象类型对象赋值给 Block 用结构体的成员变量 array 中并持有该对象。
```
static void __main_block_copy_0(struct __main_block_impl_0*dst, struct __main_block_impl_0*src) {
_Block_object_assign(
(void*)&dst->array, // 参数 1
(void*)src->array, // 参数 2
3/*BLOCK_FIELD_IS_OBJECT*/); // 参数 3
}

static void __main_block_dispose_0(struct __main_block_impl_0*src) {
_Block_object_dispose(
(void*)src->array, // 参数 1
3/*BLOCK_FIELD_IS_OBJECT*/); // 参数 2
}

// usr/include/Block.h 中
// Used by the compiler. Do not call this function yourself.
BLOCK_EXPORT void _Block_object_assign(void *, const void *, const int)
    __OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);
    
    
    // Create a heap based copy of a Block or simply add a reference to an existing one.
    // This must be paired with Block_release to recover memory, even when running
    // under Objective-C Garbage Collection.
    BLOCK_EXPORT void *_Block_copy(const void *aBlock)
        __OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);

    // Lose the reference, and if heap based and last reference, recover the memory
    BLOCK_EXPORT void _Block_release(const void *aBlock)
        __OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);


    // Used by the compiler. Do not call this function yourself.
    BLOCK_EXPORT void _Block_object_assign(void *, const void *, const int)
        __OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);

    // Used by the compiler. Do not call this function yourself.
    BLOCK_EXPORT void _Block_object_dispose(const void *, const int)
        __OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);

    // Used by the compiler. Do not use these variables yourself.
    BLOCK_EXPORT void * _NSConcreteGlobalBlock[32]
        __OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);
    BLOCK_EXPORT void * _NSConcreteStackBlock[32]
        __OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);
        
        
        #define Block_copy(...) ((__typeof(__VA_ARGS__))_Block_copy((const void *)(__VA_ARGS__)))
        #define Block_release(...) _Block_release((const void *)(__VA_ARGS__))
```
_Block_object_assign 函数调用相当于 retain 实例方法的函数，将对象赋值在对象类型的结构体成员变量中。
__main_block_dispose_0 调用 _Block_object_dispose，释放赋值在 Block 用结构体成员变量 array 中的对象。
_Block_object_dispose 函数调用相当于 release 实例方法的函数，释放赋值在对象类型的结构体成员变量中的对象。

转换代码中 __main_block_desc_0 中的 copy 和 dispose 从没使用过，那什么时候会使用呢？
（这些方法都是编译器自己去调用的，我们不会主动调用它们。）
|函数|调用时机|
|---|---|
|copy 函数|栈上的 Block 复制到堆时|
|dispose 函数|堆上的 Block 被废弃时|

栈上 Block 复制到堆上时的情况:

+ 调用 Block 的 copy 实例方法时
+ Block 作为函数返回值返回时
+ 将 Block 赋值给附有 __strong 修饰符 id 类型的类或 Block 类型成员变量时
+ 在方法名中含有 usingBlock 的 Cocoa 框架方法或 Grand Central Dispatch 的 API 中传递 Block 时

这些情况下，编译器自动的将对象的 Block 作为参数并调用 _Block_copy 函数，这与调用 Block 的 copy 实例方法的效果相同。usingBlock 和 GCD 中传递 Block 时，在该方法或函数内部对传递过来的 Block 调用 Block 的 copy 实例方法或者 _Block_copy 函数。
看似从栈复制到堆上，其实可归结为 _Block_copy 函数被调用时 Block 从栈复制到堆。
相对，释放复制到堆上的 Block 后，谁都不持有 Block 而使其被废弃时调用 dispose 函数，这相当于对象的 dealloc 实例方法。

有了这些构造，通过使用附有 __strong 修饰符的自动变量，Block 中截获的对象就能够超出其作用域而存在。

在使用 __block 变量时，已经用到 copy 和 dispose 函数：
```
static void __main_block_copy_0(struct __main_block_impl_0*dst, struct __main_block_impl_0*src) {
_Block_object_assign(

(void*)&dst->val,
(void*)src->val,
8/*BLOCK_FIELD_IS_BYREF*/);

}

static void __main_block_dispose_0(struct __main_block_impl_0*src) {
_Block_object_dispose(

(void*)src->val,
8/*BLOCK_FIELD_IS_BYREF*/);

}
```
发现最后的参数有所不同:

截获对象时和使用 __block 变量时的不同：

| 对象 | BLOCK_FIELD_IS_OBJECT |
| __block 对象 | BLOCK_FIELD_IS_BYREF |

通过 BLOCK_FIELD_IS_OBJECT  和 BLOCK_FIELD_IS_BYREF 区分 copy 函数和 dispose 函数的对象类型是对象还是 __block 变量。

copy 函数持有截获的对象、dispose 函数释放截获的对象
copy 函数持有所使用的 __block 变量、dispose 函数释放所使用的 __block 变量

**Block中使用的赋值给附有__strong修饰符的自动变量的对象和复制到堆上的__block变量由于被堆上的Block所持有，因而可超出其变量作用域而存在。**

```
// 不调用 copy 也能正常执行，因为右边 block 赋值给右边时，已经赋值到了堆上
        void (^blk)(id);
        {
             id array = [[NSMutableArray alloc] init];
//            id array = [NSMutableArray array];
//            id __weak array2 = array;
            
            blk = ^(id obj){
                [array addObject:obj];
                
                NSLog(@"array count = %ld", [array count]);
            };
        }
        
        blk([[NSObject alloc] init]);
        blk([[NSObject alloc] init]);
        blk([[NSObject alloc] init]);
        
// 示例 2：
blk_t blk;
{
    id array = [[NSMutableArray alloc] init];
    // array = [NSMutableArray array];
    // __unsafe_unretained id array2 = array;
    NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);
    blk = ^(id obj){
        // id __strong array3 = array2;
        [array addObject:obj];
        NSLog(@"array count = %ld", [array count]);
        // NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);
    };

    NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);
}

blk([[NSObject alloc] init]);
blk([[NSObject alloc] init]);
blk([[NSObject alloc] init]);

// 打印：
⛈⛈⛈ array retainCount = 1
⛈⛈⛈ array retainCount = 3
array count = 1
array count = 2
array count = 3
```

## __block 变量和对象
__block 说明符可指定任何类型的自动变量。
```
// 
__block id obj = [[NSObject alloc] init];

// __Block_byref_obj_0
struct __Block_byref_obj_0 {
  void *__isa;
__Block_byref_obj_0 *__forwarding;
 int __flags;
 int __size;
 void (*__Block_byref_id_object_copy)(void*, void*);
 void (*__Block_byref_id_object_dispose)(void*);
 id obj;
};

// _Block_object_assign
static void __Block_byref_id_object_copy_131(void *dst, void *src) {
 _Block_object_assign(
 
 (char*)dst + 40,
 *(void * *) ((char*)src + 40),
 131
 
 );
}

// _Block_object_dispose
static void __Block_byref_id_object_dispose_131(void *src) {
 _Block_object_dispose(
 
 *(void * *) ((char*)src + 40),
 131
 
 );
}

// 
__attribute__((__blocks__(byref))) __Block_byref_obj_0 obj = {

(void*)0,
(__Block_byref_obj_0 *)&obj,
33554432,
sizeof(__Block_byref_obj_0),

__Block_byref_id_object_copy_131,
__Block_byref_id_object_dispose_131,

((NSObject *(*)(id, SEL))(void *)objc_msgSend)((id)((NSObject *(*)(id, SEL))(void *)objc_msgSend)((id)objc_getClass("NSObject"), sel_registerName("alloc")), sel_registerName("init"))

};
```
_Block_object_assign 和 _Block_object_dispose

```
// 当 array 用不同的创建方式时，
//（block 是否调用 copy 都不影响结果）：
// (array2 是否用 __block 不影响结果)
// 1. id array = [[NSMutableArray alloc] init];
blk_t blk;
{
     id array = [[NSMutableArray alloc] init];
    // id array = [NSMutableArray array];
    id __weak array2 = array;

    blk = [^(id obj){
        [array2 addObject:obj];

        NSLog(@"array count = %ld", [array2 count]);
    } copy];
}

blk([[NSObject alloc] init]);
blk([[NSObject alloc] init]);
blk([[NSObject alloc] init]);
// 打印:
array count = 0
array count = 0
array count = 0

// 2. id array = [NSMutableArray array];
blk_t blk;
{
    // id array = [[NSMutableArray alloc] init];
    id array = [NSMutableArray array];
    id __weak array2 = array;

    blk = [^(id obj){
        [array2 addObject:obj];

        NSLog(@"array count = %ld", [array2 count]);
    } copy];
}

blk([[NSObject alloc] init]);
blk([[NSObject alloc] init]);
blk([[NSObject alloc] init]);
// 打印:
array count = 1
array count = 2
array count = 3

// 3. 如果再加一层 @autoreleasepool {}
blk_t blk;
{
    // id array = [[NSMutableArray alloc] init];
    @autoreleasepool {
        
        id array = [NSMutableArray array];
        id __weak array2 = array;
        
        blk = [^(id obj){
            [array2 addObject:obj];
            
            NSLog(@"❄️❄️❄️ array count = %ld", [array2 count]);
        } copy];
        
    } // 出了这个花括号 array 就释放废弃了，同时 array2 也被置为 nil 了，所以打印 0 
    
}

blk([[NSObject alloc] init]);
blk([[NSObject alloc] init]);
blk([[NSObject alloc] init]);
// 打印:
❄️❄️❄️ array count = 0
❄️❄️❄️ array count = 0
❄️❄️❄️ array count = 0

// 4. 用 __unsafe_unretained 修饰
blk_t blk;
{
     id array = [[NSMutableArray alloc] init];
    // id array = [NSMutableArray array];
    id __unsafe_unretained array2 = array;

    blk = ^(id obj){
        [array2 addObject:obj];
        
        NSLog(@"array count = %ld", [array2 count]);
    };
}

blk([[NSObject alloc] init]);
blk([[NSObject alloc] init]);
blk([[NSObject alloc] init]);
// 崩溃，访问悬垂指针

// 5. array 用 [NSMutableArray array] 创建能正常运行
blk_t blk;
{
    // id array = [[NSMutableArray alloc] init];
    id array = [NSMutableArray array];
    id __unsafe_unretained array2 = array;

    blk = ^(id obj){
        [array2 addObject:obj];
        
        NSLog(@"array count = %ld", [array2 count]);
    };
}
// 即使出了花括号， array 还处于自动释放池，并没有被释放

blk([[NSObject alloc] init]);
blk([[NSObject alloc] init]);
blk([[NSObject alloc] init]);
// 打印:
array count = 1
array count = 2
array count = 3

// 6. 报错 Cannot capture __autoreleasing variable in a block
id __autoreleasing array2 = array;

// 7. 把 array 赋值给 __block id array2 则较复杂
 id array = [[NSMutableArray alloc] init];
 // array = [NSMutableArray array];
__block id array2 = array;

// 转换后：
 id array = ((NSMutableArray *(*)(id, SEL))(void *)objc_msgSend)((id)((NSMutableArray *(*)(id, SEL))(void *)objc_msgSend)((id)objc_getClass("NSMutableArray"), sel_registerName("alloc")), sel_registerName("init"));

// 看到 __Block_byref_array2_0 结构体初始化传入了 array 赋值给它的 array2 成员变量
__attribute__((__blocks__(byref))) __Block_byref_array2_0 array2 = 
{
(void*)0,
(__Block_byref_array2_0 *)&array2,
33554432,
sizeof(__Block_byref_array2_0),
__Block_byref_id_object_copy_131,
__Block_byref_id_object_dispose_131,
array
};

// 这个 Block 调用 copy 函数也有必要认真看一下
blk = 
(blk_t)((id (*)(id, SEL))(void *)objc_msgSend)((id)((void (*)(id))&__main_block_impl_0((void *)__main_block_func_0, &__main_block_desc_0_DATA, (__Block_byref_array2_0 *)&array2, 570425344)), sel_registerName("copy"));
```

## Block 循环引用
如果在 Block 中使用附有 strong 修饰符的对象类型自动变量，那么当 block 从栈复制到堆时，该对象为 block 所持有。不复制也会持有的，block 结构体初始化的时候已经将其捕获。
示例：
```
// 1. 
id array = [[NSMutableArray alloc] init];
{
    NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);
    
    ^(id obj) {
        [array addObject:obj];
        NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);
    };
    
    NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);
}
NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);

// 打印：
⛈⛈⛈ array retainCount = 1 // array 持有
⛈⛈⛈ array retainCount = 2 // array 和 栈上 block 同时持有
⛈⛈⛈ array retainCount = 1 // 出了花括号，栈上 block 释放，只剩下 array 持有

// 2.
id array = [[NSMutableArray alloc] init];
{
    NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);
    
    blk = ^(id obj) {
        [array addObject:obj];
        NSLog(@"⛈⛈⛈  Block array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);
    };
    
    NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);
}
NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);

if (blk != nil) {
    blk([[NSObject alloc] init]);
    blk([[NSObject alloc] init]);
    blk([[NSObject alloc] init]);
}
// 打印：
⛈⛈⛈ array retainCount = 1 // array 持有
⛈⛈⛈ array retainCount = 3 // 花括号内，栈上 block 持有、复制到堆的 block 持有、array 持有，总共是 3
⛈⛈⛈ array retainCount = 2 // 这里减 1 是栈上 block 出了花括号释放，同时也释放了 array，所以这里减 1
⛈⛈⛈  Block array retainCount = 2 // 这里 block 执行 3 次打印都是 2，此时 array 持有和堆上的 block blk 持有
⛈⛈⛈  Block array retainCount = 2
⛈⛈⛈  Block array retainCount = 2

// 3.
id array = [[NSMutableArray alloc] init];
{
    NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);
    
    blk = ^(id obj) {
        [array addObject:obj];
        NSLog(@"⛈⛈⛈  Block array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);
    };
    
    NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);
}

NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);

if (blk != nil) {
    blk([[NSObject alloc] init]);
    blk([[NSObject alloc] init]);
    blk([[NSObject alloc] init]);
}

blk = nil;
NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);
// 打印：
⛈⛈⛈ array retainCount = 1
⛈⛈⛈ array retainCount = 3
⛈⛈⛈ array retainCount = 2
⛈⛈⛈  Block array retainCount = 2
⛈⛈⛈  Block array retainCount = 2
⛈⛈⛈  Block array retainCount = 2
⛈⛈⛈ array retainCount = 1 // 只有这里，blk 三次执行完毕后，blk 赋值 空，blk 释放，同时释放 array，所以还剩下 array 持有，retainCount 为 1

// 4.
{
    id array = [[NSMutableArray alloc] init];
    NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);
    
    blk = ^(id obj) {
        [array addObject:obj];
        NSLog(@"⛈⛈⛈  Block array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);
    };
    
    NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);
}

// NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);

if (blk != nil) {
    blk([[NSObject alloc] init]);
    blk([[NSObject alloc] init]);
    blk([[NSObject alloc] init]);
}
// 打印：
⛈⛈⛈ array retainCount = 1 // 对象创建时为 1
⛈⛈⛈ array retainCount = 3 // 栈上 block 持有和复制到堆时堆上 block 持有 
                             // 出了花括号以后，栈上 block 释放，array 局部变量释放
                             // 剩下的 1 是堆上的 block 持有的
                             // 所以下面 block 指向时，打印都是 1
⛈⛈⛈  Block array retainCount = 1 // 出了花括号以后变量 array 释放，还剩下 block blk 自己持有，所以打印 1
⛈⛈⛈  Block array retainCount = 1
⛈⛈⛈  Block array retainCount = 1
```
```
- (id)init {
    self = [super init];
    blk_ = ^{ NSLog(@"self = %@", self);};
    return self;
}

// 依然会捕获 self,对编译器而言，obj_ 只不过是对象用结构体的成员变量。
// blk_ = ^{ NSLog(@"obj_ = %@", self->obj_); };

- (id)init {
    self = [super init];
    blk_ = ^{
        NSLog(@"obj_ = %@", obj_);
        };
        
    return self;    
}

// 除了 __weak self 也可用:
id __weak obj = obj_;
blk_ = ^{ NSLog(@"obj_ = %@", obj); };
```
该源代码中，由于 Block 存在时，持有该 Block 的 Object 对象即赋值在变量 tmp 中的self 必定存在，因此不需要判断变量 tmp 的值是否为 nil。
在 iOS 4 和 OS X 10.6 中，可以用 _unsafe_unretained 代替 __weak 修饰符，此处即可代替，且不必担心悬垂指针。

**由于Block语法赋值在了成员变量 blk_ 中，因此通过 Block 语法生成在栈上的 Block 此时由栈复制到堆上，并持有所使用的 self.**

在为避免循环引用而使用 __weak 修饰符时，虽说可以确认使用附有 __weak 修饰符的变量时，是否为 nil，但更有必要使之生存，以使用赋值给附有 __weak 修饰符变量的对象。（意思就比如上面，block 表达式开始执行时，首先判断 self 是否是 nil，如果不是 nil 才有必要继续往下执行，在往下执行的过程中并且希望 self 一直存在，不要正在使用时，竟被释放了，如果是单线程则无需考虑，但是在多线程开发时一定要考虑到这一点。）

**在 Block 里面加 __strong 修饰 weakSelf 取得 strongSelf，防止 block 结构体实例的 self 成员变量过早释放。Block 从外界所捕获的对象和在 Block 内部使用 __strong 强引用的对象，差别就在于一个是在 定义的时候 就会影响对象的引用计数, 一个是在 Block 运行的时候才强引用对象，且 block 表达式执行完毕还是会 -1**

**__weak 修饰的对象被 Block 引用，不会影响对象的释放，而 __strong 在 Block 内部修饰的对象，会保证，在使用这个对象在 scope 内，这个对象都不会被释放，出了 scope，引用计数就会 -1，且 __strong 主要是用在多线程运用中，如果只使用单线程，则只需要使用 __weak 即可。**

用 __block 变量来避免循环引用，原理是在 Block 内部对捕获的变量赋值为 nil，硬性破除引用环。
```
- (id)init {
    self = [super init];
    __block id tmp = self;
    blk_ = ^{
        NSLog(@"self = %@", tmp);
        tmp = nil;
    };
}
```

**对使用 __block 变量避免循环引用的方法和使用 __weak 修饰符及 __unsafe_unretained 修饰符避免循环引用的方法做比较:**

__block 优点：
+ 通过 __block 变量可控制对象的持有期间。
+ 在不能使用 __weak 修饰符的环境中不使用 __unsafe_unretained 修饰符即可（不必担心访问悬垂指针）
+ 在执行 Block 时可动态决定是否将 nil 或其他对象赋值在 __block  变量中。

__block 缺点:
+ 为避免循环引用必须执行 Block。

## copy/release
ARC  无效时，一般需要手动将 Block 从栈复制到堆，另外，由于 ARC 无效，所以肯定要手动释放复制的 Block。此时可用 copy 实例方法来复制，用 release 实例方法来释放。

```
void (^blk_on_heap)(void) = [blk_on_stack copy];
[blk_on_heap release];
```

只要 Block 有一次 **复制并配置在堆上**，就可通过 **retain 实例方法** 持有。

```
[blk_on_heap retain];
```
但是对于 **配置在栈上的 Block  调用 retain 实例方法则不起作用**。
```
[blk_on_stack retain];
```
该源代码中，虽然对赋值给 blk_on_stack 的栈上的 Block 调用了 retain 实例方法，**但实际上对此源代码不起任何作用**。因此**推荐使用 copy 实例方法来持有 Block**。

另外，由于 Blocks 是 C 语言的扩展，所以在 C 语言中也可以使用 Block 语法。此时使用 “Block_copy 函数” 和 “Block_release 函数” 代替 copy/release 实例方法。使用方法以及引用计数的思考方式与 OC 中的 copy/release 实例方法相同。
```
// 把栈上的 block 复制到堆上
void (^blk_on_heap)(void) = Block_copy(blk_on_stack);
// 释放堆上的 block
Block_release(blk_on_heap);
```
Block_copy 函数就是之前出现过的 _Block_copy 函数，即 OC  运行时库所使用的为 C 语言而准备的函数。释放堆上的 Block 时也同样调用 OC 运行时库的 Block_release 函数。

另外极其重要的一个知识点:
另外极其重要的一个知识点:
另外极其重要的一个知识点:

**ARC 无效时，__Block 说明符被用来避免 Block 中的循环引用，这是由于当 Block 从栈复制到堆时，若 Block 使用的变量为附有 __block 说明符的 id 类型或对象类型的自动变量，不会被 retain；若 Block 使用变量为没有 __block 说明符的 id 类型或对象类型的自动变量，则被 retain。**

由于 ARC 有效时和无效时 __block 说明符的用途有很大区别，因此编写源代码时，必须知道源代码是在 ARC 有效情况下编译还是无效情况下编译。

# Block 部分 完结撒花 🎉🎉🎉

