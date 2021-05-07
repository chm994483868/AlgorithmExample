# iOS APP 启动优化(三)：dyld（the dynamic link editor）是动态链接器


## 静态库与动态库

&emsp;TARGETS -> Build Phases -> Link Binary With Libraries 中我们可以添加多个系统库或我们自己的库，其中包含静态库和动态库。静态库通常以 .a .lib 或者 .framework 结尾，动态库以 .tbd .so .framework 结尾。链接时，静态库会被完整的复制到可执行文件中，被多次使用就会有多份冗余拷贝，动态库链接时不复制，程序运行时由系统动态加载到内存中，供程序调用，系统只加载一次，多个程序共用，节省内存。

## LLDB 常用命令

1. p po p/x p/o p/t p/d p/c
2. expression 修改参数
3. call 
4. x x/4gx x/4xg
5. image list
6. image lookup --address+地址
7. thread list
8. thread backtrace（bt）bt all
9. thread return frame variable
10. register read register read/x

## clang 

&emsp;clang:Clang 是一个 C++ 编写、基于 LLVM、发布于 LLVM BSD 许可证下的 C/C++/Objective-C/ Objective-C++ 编译器。它与 GNU C 语言规范几乎完全兼容(当然，也有部分不兼容的内容， 包括编译命令选项也会有点差异)，并在此基础上增加了额外的语法特性，比如 C 函数重载 (通过 \_ attribute_((overloadable)) 来修饰函数)，其目标(之一)就是超越 GCC。

```c++
// main.m 代码如下：

__attribute__((constructor)) void main_front() {
    printf("🦁🦁🦁 %s 执行 \n", __func__);
}

__attribute__((destructor)) void main_back() {
    printf("🦁🦁🦁 %s 执行 \n", __func__);
}

int main(int argc, char * argv[]) {
    NSLog(@"🦁🦁🦁 %s 执行", __func__);
    
//    NSString * appDelegateClassName;
//    @autoreleasepool {
//        // Setup code that might create autoreleased objects goes here.
//        appDelegateClassName = NSStringFromClass([AppDelegate class]);
//    }
//    return UIApplicationMain(argc, argv, nil, appDelegateClassName);
    
    return 0;
}

// ViewController.m 代码如下：

@implementation ViewController

+ (void)load {
    NSLog(@"🦁🦁🦁 %s 执行", __func__);
}

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
}

@end

// 运行后控制台打印如下：

2021-05-07 14:46:45.238651+0800 Test_ipa_Simple[43277:456220] 🦁🦁🦁 +[ViewController load] 执行
🦁🦁🦁 main_front 执行 
2021-05-07 14:46:45.242218+0800 Test_ipa_Simple[43277:456220] 🦁🦁🦁 main 执行
🦁🦁🦁 main_back 执行 
```
&emsp;\_\_attribute__ 可以设置函数属性(Function Attribute)、变量属性(Variable Attribute)和类型属性(Type Attribute)。\_\_attribute__ 前后都有两个下划线，并且后面会紧跟一对原括弧，括弧里面是相应的 \_\_attribute__ 参数，\_\_attribute__ 语法格式为：`__attribute__ ( ( attribute-list ) )`。

&emsp;若函数被设定为 constructor 属性，则该函数会在 main 函数执行之前被自动的执行。类似的，若函数被设定为 destructor 属性，则该函数会在 main 函数执行之后或者 exit 被调用后被自动的执行。










## 参考链接
**参考链接:🔗**
+ [OC底层原理之-App启动过程（dyld加载流程）](https://juejin.cn/post/6876773824491159565)
+ [iOS里的动态库和静态库](https://www.jianshu.com/p/42891fb90304)

