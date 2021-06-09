# iOS APP 启动优化(四)：_objc_init 过程解析

&emsp;上一篇学习 dyld 涉及到 objc 中的 \_objc_init 函数，但是我们没有深入学习其涉及到的流程，那么就由本篇开始吧。

## \_objc_init
&emsp;在 objc/Source/objc-os.mm 中可找到 `void _objc_init(void)` 的定义。 

```c++
/***********************************************************************
* _objc_init
* Bootstrap initialization. Registers our image notifier with dyld.
* Called by libSystem BEFORE library initialization time
**********************************************************************/

void _objc_init(void)
{
    // initialized 局部静态变量，保证只初始化一次，下次再调用 _objc_init 则直接 return
    static bool initialized = false;
    if (initialized) return;
    initialized = true;
    
    // fixme defer initialization until an objc-using image is found?
    environ_init(); // 1⃣️ 环境变量初始化 
    
    tls_init(); // 2⃣️ 本地线程池
    static_init(); // 3⃣️ 系统级别的 C++ 构造函数调用
    runtime_init(); // 4⃣️ runtime 初始化
    exception_init(); // 5⃣️ 注册监听异常的回调
    cache_init(); // 6⃣️ cache 的初始化
    _imp_implementationWithBlock_init(); // 7⃣️ 对 imp 的 Block 标记进行初始化

    // 8⃣️ 注册回调通知，& 是引用类型的参数
    _dyld_objc_notify_register(&map_images, load_images, unmap_image);

#if __OBJC2__
    // 9⃣️ dyld 通知注册标记
    didCallDyldNotifyRegister = true;
#endif
}
```

&emsp;下面我们就详细看看 1⃣️ 到 9⃣️ 的具体实现。

## environ_init

&emsp;`environ_init` 方法就是进行环境变量的初始化。在项目运行之前我们可以在 Edit Scheme... -> Run -> Arguments -> Environment Variables 中添加环境变量以及对应的值，它们默认都是 NO，我们可以根据我们的需要来进行添加并把 value 设置为 YES。在 objc-env.h 文件中列出了所有的环境变量，其中它们分别以 OBJC_PRINT_、OBJC_DEBUG_、OBJC_DISABLE_ 为开头分了 3 块，分别针对 PRINT（打印）、DEBUG（调试）、DISABLE（禁用）的情况。环境变量的设置，可以帮助我们更快速的处理一些问题。例如添加 OBJC_PRINT_LOAD_METHODS 环境变量，控制台就会打印项目中所有的系统类、我们自己的类以及分类中的 `+load` 方法。

```c++
/***********************************************************************
* environ_init
* Read environment variables that affect the runtime. 读取影响 runtime 的环境变量。
* Also print environment variable help, if requested. 如果有需要也会打印对我们有帮助的环境变量。
**********************************************************************/
void environ_init(void) 
{
    if (issetugid()) {
        // All environment variables are silently ignored when setuid or setgid.
        // 当 setuid 或 setgid 时，所有的环境变量是被静默忽略的。
        
        // This includes OBJC_HELP and OBJC_PRINT_OPTIONS themselves.
        // 这也包括 OBJC_HELP 和 OBJC_PRINT_OPTIONS。
        
        return; // ⬅️ 直接 return
    } 
    
    // 三个局部变量，默认是 false，然后在下面第一个 for 循环中判断是否把它们置为 true。
    bool PrintHelp = false;
    bool PrintOptions = false;
    bool maybeMallocDebugging = false;

    // Scan environ[] directly instead of calling getenv() a lot.
    // This optimizes the case where none are set.
    
    // 直接遍历扫描 environ[] 
    for (char **p = *_NSGetEnviron(); *p != nil; p++) {
        // 如果扫描到 "Malloc"、"DYLD"、"NSZombiesEnabled" 则把 maybeMallocDebugging 置为 true
        if (0 == strncmp(*p, "Malloc", 6)  ||  0 == strncmp(*p, "DYLD", 4)  ||  
            0 == strncmp(*p, "NSZombiesEnabled", 16))
        {
            maybeMallocDebugging = true;
        }
        
        // 如果是 "OBJC_" 打头的则直接跳过
        if (0 != strncmp(*p, "OBJC_", 5)) continue;
        
        // 如果扫描到 "OBJC_HELP=" 则把 PrintHelp 置为 true
        if (0 == strncmp(*p, "OBJC_HELP=", 10)) {
            PrintHelp = true;
            continue;
        }
        
        // 如果扫描到 "OBJC_PRINT_OPTIONS=" 则把 PrintOptions 置为 true
        if (0 == strncmp(*p, "OBJC_PRINT_OPTIONS=", 19)) {
            PrintOptions = true;
            continue;
        }
        
        // strchr 函数功能为在一个串中查找给定字符的第一个匹配之处。
        // 函数原型为：char *strchr(const char *str, int c)，
        // 即在参数 str 所指向的字符串中搜索第一次出现字符 c（一个无符号字符）的位置。
        // strchr 函数包含在 C 标准库 <string.h> 中。
        
        // 查找 p 中第一个 = 的位置 
        const char *value = strchr(*p, '=');
        if (!*value) continue;
        value++; // 然后这里 value 做了一次自增运算（因为 value 是一个 char 指针，所以 value 前进一个字节）
        
        // 这里是遍历 Settings 这个元素类型是 option_t 的全局不可变数组。
        // 在 objc-env.h 文件中列出了所有的 option_t 项。
        for (size_t i = 0; i < sizeof(Settings)/sizeof(Settings[0]); i++) {
            // ⚠️ 实话实说，下面这一段判断是否要赋值为 YES，没有完全看懂，
            // 大概是从 _NSGetEnviron 拿到环境变量，
            // 然后遍历来判断是否需要把 Settings 中存放的对应的 option_t 实例的 var 成员变量置为 YES。
            const option_t *opt = &Settings[i];
            if ((size_t)(value - *p) == 1+opt->envlen  &&  
                0 == strncmp(*p, opt->env, opt->envlen))
            {
                *opt->var = (0 == strcmp(value, "YES"));
                break;
            }
        }            
    }

    // Special case: enable some autorelease pool debugging
    // 特例：启用一些自动释放池调试
    // when some malloc debugging is enabled and OBJC_DEBUG_POOL_ALLOCATION is not set to something other than NO.
    // 当启用了某些 malloc 调试并且 OBJC_DEBUG_POOL_ALLOCATION 未设置为 NO 以外的值时。
    
    // 两个 if 判断是否把 DebugPoolAllocation 置为 true
    if (maybeMallocDebugging) {
        const char *insert = getenv("DYLD_INSERT_LIBRARIES");
        const char *zombie = getenv("NSZombiesEnabled");
        const char *pooldebug = getenv("OBJC_DEBUG_POOL_ALLOCATION");
        if ((getenv("MallocStackLogging")
             || getenv("MallocStackLoggingNoCompact")
             || (zombie && (*zombie == 'Y' || *zombie == 'y'))
             || (insert && strstr(insert, "libgmalloc")))
            &&
            (!pooldebug || 0 == strcmp(pooldebug, "YES")))
        {
            DebugPoolAllocation = true;
        }
    }

    // Print OBJC_HELP and OBJC_PRINT_OPTIONS output.
    // 打印 OBJC_HELP 和 OBJC_PRINT_OPTIONS 输出。
    if (PrintHelp  ||  PrintOptions) {
        // 下面两个 if 是输出一些提示信息
        if (PrintHelp) {
            // Objective-C 运行时调试。设置 variable=YES 以启用。
            _objc_inform("Objective-C runtime debugging. Set variable=YES to enable.");
            // OBJC_HELP：描述可用的环境变量
            _objc_inform("OBJC_HELP: describe available environment variables");
            if (PrintOptions) {
                // OBJC_HELP 已设置
                _objc_inform("OBJC_HELP is set");
            }
            // OBJC_PRINT_OPTIONS：列出设置了哪些选项
            _objc_inform("OBJC_PRINT_OPTIONS: list which options are set");
        }
        if (PrintOptions) {
            // OBJC_PRINT_OPTIONS 已设置
            _objc_inform("OBJC_PRINT_OPTIONS is set");
        }
        
        // 下面的 for 循环便是遍历打印 Settings 中的所有的环境变量的 env 以及对应的描述，
        // 然后是打印我们设置为 YES 的环境变量，表示我们对该环境变量进行了设置。
        for (size_t i = 0; i < sizeof(Settings)/sizeof(Settings[0]); i++) {
            const option_t *opt = &Settings[i];
            // 打印现有的环境变量，以及其对应的描述
            if (PrintHelp) _objc_inform("%s: %s", opt->env, opt->help);
            
            // 打印 var 的值设置为 YES 的环境变量，即告诉我们当前对哪些环境变量进行了设置
            if (PrintOptions && *opt->var) _objc_inform("%s is set", opt->env);
        }
    }
}
```

### struct option_t 和 Settings 数组

&emsp;`objc-env.h` 中就完全是一大组 `OPTION` 宏的使用，定义了一组 `option_t` 结构体实例，每一个 `option_t` 实例就用来表示一个环境变量。

&emsp;这里我们首先要明白 `#include "objc-env.h"` 的作用，在编译时编译器会把  `#include "objc-env.h"` 直接替换为 `objc-env.h` 文件中的一大组  `option_t` 实例，即这里的 `const option_t Settings[]` 数组便包含了 `objc-env.h` 中的所有的 `option_t` 结构体实例。

&emsp;这里我们摘出其中几个比较重要或者我们比较熟悉的环境变量，如：`OBJC_DISABLE_TAGGED_POINTERS` 表示是否禁用 NSNumber、NSString 等的 tagged pointer 指针优化、`OBJC_DISABLE_TAG_OBFUSCATION` 表示是否禁用 tagged pointer 的混淆、`OBJC_DISABLE_NONPOINTER_ISA` 表示是否禁用 non-pointer 的 isa 字段。

```c++
struct option_t {
    bool* var;
    const char *env;
    const char *help;
    size_t envlen;
};

const option_t Settings[] = {
#define OPTION(var, env, help) option_t{&var, #env, help, strlen(#env)}, 
#include "objc-env.h"
#undef OPTION
};
```

```c++
// OPTION(var, env, help)

OPTION( PrintImages,              OBJC_PRINT_IMAGES,               "log image and library names as they are loaded")
OPTION( PrintImageTimes,          OBJC_PRINT_IMAGE_TIMES,          "measure duration of image loading steps")
...
OPTION( DisableTaggedPointers,    OBJC_DISABLE_TAGGED_POINTERS,    "disable tagged pointer optimization of NSNumber et al.") 
OPTION( DisableTaggedPointerObfuscation, OBJC_DISABLE_TAG_OBFUSCATION,    "disable obfuscation of tagged pointers")
OPTION( DisableNonpointerIsa,     OBJC_DISABLE_NONPOINTER_ISA,     "disable non-pointer isa fields")
...
```

### 设置 OBJC_DISABLE_NONPOINTER_ISA

&emsp;下面我们演示一下 OBJC_DISABLE_NONPOINTER_ISA 的使用，我们在 Environment Variables 中添加 OBJC_DISABLE_NONPOINTER_ISA 并设置为 YES。（我们应该还记得如何判断实例对象的 isa 是 non-pointer 还是 pointer，即 uintptr_t nonpointer : 1，如果 isa 的第一位是 1 则表示它是 non-pointer 否则就是 pointer。）

&emsp;我们创建一个实例对象，然后分别在 OBJC_DISABLE_NONPOINTER_ISA 设置为 YES/NS 的情况下打印它的 isa 中的内容。

```c++
// ⬇️⬇️ OBJC_DISABLE_NONPOINTER_ISA 未设置或者设置为 NO
(lldb) x/4gx person
0x108baa1a0: 0x011d8001000080e9 0x0000000000000000
0x108baa1b0: 0x6b636950534e5b2d 0x426863756f547265
(lldb) p/t 0x011d8001000080e9
// ⬇️ isa 第一位是 1
(long) $1 = 0b0000000100011101100000000000000100000000000000001000000011101001

// ⬇️⬇️ OBJC_DISABLE_NONPOINTER_ISA 设置为 YES
(lldb) x/4gx person
0x108d04080: 0x00000001000080e8 0x0000000000000000
0x108d04090: 0x00000001a0080001 0x0000000100008028
(lldb) p/t 0x00000001000080e8
// ⬇️ isa 第一位是 0
(long) $1 = 0b0000000000000000000000000000000100000000000000001000000011101000
```

### 设置 OBJC_PRINT_LOAD_METHODS

&emsp;下面我们演示一下 OBJC_PRINT_LOAD_METHODS 的使用，我们在 Environment Variables 中添加 OBJC_PRINT_LOAD_METHODS 并设置为 YES。运行项目，可看到如下打印项目中所有的 load 方法。

```c++
objc[37659]: LOAD: category 'NSObject(NSObject)' scheduled for +load
objc[37659]: LOAD: +[NSObject(NSObject) load]

objc[37659]: LOAD: category 'NSObject(NSObject)' scheduled for +load
objc[37659]: LOAD: +[NSObject(NSObject) load]

objc[37659]: LOAD: class 'NSColor' scheduled for +load
objc[37659]: LOAD: class 'NSApplication' scheduled for +load
objc[37659]: LOAD: class 'NSBinder' scheduled for +load
objc[37659]: LOAD: class 'NSColorSpaceColor' scheduled for +load
objc[37659]: LOAD: class 'NSNextStepFrame' scheduled for +load
objc[37659]: LOAD: +[NSColor load]

objc[37659]: LOAD: +[NSApplication load]

objc[37659]: LOAD: +[NSBinder load]

objc[37659]: LOAD: +[NSColorSpaceColor load]

objc[37659]: LOAD: +[NSNextStepFrame load]

objc[37659]: LOAD: category 'NSError(FPAdditions)' scheduled for +load
objc[37659]: LOAD: +[NSError(FPAdditions) load]

objc[37659]: LOAD: class '_DKEventQuery' scheduled for +load
objc[37659]: LOAD: +[_DKEventQuery load]

objc[37659]: LOAD: class 'LGPerson' scheduled for +load
objc[37659]: LOAD: +[LGPerson load]
```

&emsp;环境变量的学习我们就到这里了，objc-env.h 中还有其他的一些环境变量的设置，这里就不一一演示了。

## tls_init

&emsp;

```c++
void tls_init(void)
{
#if SUPPORT_DIRECT_THREAD_KEYS
    pthread_key_init_np(TLS_DIRECT_KEY, &_objc_pthread_destroyspecific);
#else
    _objc_pthread_key = tls_create(&_objc_pthread_destroyspecific);
#endif
}
```










## 参考链接
**参考链接:🔗**
+ [dyld-832.7.3](https://opensource.apple.com/tarballs/dyld/)
+ [OC底层原理之-App启动过程（dyld加载流程）](https://juejin.cn/post/6876773824491159565)
+ [iOS中的dyld缓存是什么？](https://blog.csdn.net/gaoyuqiang30/article/details/52536168)
+ [iOS进阶之底层原理-应用程序加载（dyld加载流程、类与分类的加载）](https://blog.csdn.net/hengsf123456/article/details/116205004?utm_medium=distribute.pc_relevant.none-task-blog-baidujs_title-4&spm=1001.2101.3001.4242)
+ [iOS应用程序在进入main函数前做了什么？](https://www.jianshu.com/p/73d63220d4f1)
+ [dyld加载应用启动原理详解](https://www.jianshu.com/p/1b9ca38b8b9f)
+ [iOS里的动态库和静态库](https://www.jianshu.com/p/42891fb90304)
+ [Xcode 中的链接路径问题](https://www.jianshu.com/p/cd614e080078)
+ [iOS 利用 Framework 进行动态更新](https://nixwang.com/2015/11/09/ios-dynamic-update/)
+ [命名空间namespace ，以及重复定义的问题解析](https://blog.csdn.net/u014357799/article/details/79121340)
+ [C++ 命名空间namespace](https://www.jianshu.com/p/30e960717ef1)
+ [一文了解 Xcode 生成「静态库」和「动态库」 的流程](https://mp.weixin.qq.com/s/WH8emrMpLeVW-LfGwN09cw)
+ [Hook static initializers](https://blog.csdn.net/majiakun1/article/details/99413403)



```c++
if ( sEnv.DYLD_PRINT_OPTS )
    printOptions(argv);
if ( sEnv.DYLD_PRINT_ENV ) 
    printEnvironmentVariables(envp);
```

&emsp;此处是判断是否设置了环境变量，如果设置了，那么 xcode 就会在控制台打印相关的详细信息。（在 Edit Scheme... -> Run -> Arguments -> Environment Variables 进行添加） 

&emsp;当添加了 DYLD_PRINT_OPTS 时，会在控制台输出可执行文件的位置。
```c++
opt[0] = "/Users/hmc/Library/Developer/CoreSimulator/Devices/4E072E27-E586-4E81-A693-A02A3ED83DEC/data/Containers/Bundle/Application/ECDA091A-1610-49D2-8BC0-B41A58BC76EC/Test_ipa_Simple.app/Test_ipa_Simple"
```

&emsp;当添加了 DYLD_PRINT_ENV 时，会在控制台输出用户级别、插入的动态库、动态库的路径、模拟器的信息等等一系列的信息，由于内容过多这里就粘贴出来了。

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

&emsp;clang:Clang 是一个 C++ 编写、基于 LLVM、发布于 LLVM BSD 许可证下的 C/C++/Objective-C/Objective-C++ 编译器。它与 GNU C 语言规范几乎完全兼容（当然，也有部分不兼容的内容， 包括编译命令选项也会有点差异），并在此基础上增加了额外的语法特性，比如 C 函数重载（通过 \_ attribute_((overloadable)) 来修饰函数)，其目标(之一)就是超越 GCC。
