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

&emsp;`environ_init` 方法就是进行环境变量的设置，

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
    
    // 
    bool PrintHelp = false;
    bool PrintOptions = false;
    bool maybeMallocDebugging = false;

    // Scan environ[] directly instead of calling getenv() a lot.
    // This optimizes the case where none are set.
    for (char **p = *_NSGetEnviron(); *p != nil; p++) {
        if (0 == strncmp(*p, "Malloc", 6)  ||  0 == strncmp(*p, "DYLD", 4)  ||  
            0 == strncmp(*p, "NSZombiesEnabled", 16))
        {
            maybeMallocDebugging = true;
        }

        if (0 != strncmp(*p, "OBJC_", 5)) continue;
        
        if (0 == strncmp(*p, "OBJC_HELP=", 10)) {
            PrintHelp = true;
            continue;
        }
        if (0 == strncmp(*p, "OBJC_PRINT_OPTIONS=", 19)) {
            PrintOptions = true;
            continue;
        }
        
        const char *value = strchr(*p, '=');
        if (!*value) continue;
        value++;
        
        for (size_t i = 0; i < sizeof(Settings)/sizeof(Settings[0]); i++) {
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
    // when some malloc debugging is enabled 
    // and OBJC_DEBUG_POOL_ALLOCATION is not set to something other than NO.
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
    if (PrintHelp  ||  PrintOptions) {
        if (PrintHelp) {
            _objc_inform("Objective-C runtime debugging. Set variable=YES to enable.");
            _objc_inform("OBJC_HELP: describe available environment variables");
            if (PrintOptions) {
                _objc_inform("OBJC_HELP is set");
            }
            _objc_inform("OBJC_PRINT_OPTIONS: list which options are set");
        }
        if (PrintOptions) {
            _objc_inform("OBJC_PRINT_OPTIONS is set");
        }

        for (size_t i = 0; i < sizeof(Settings)/sizeof(Settings[0]); i++) {
            const option_t *opt = &Settings[i];            
            if (PrintHelp) _objc_inform("%s: %s", opt->env, opt->help);
            if (PrintOptions && *opt->var) _objc_inform("%s is set", opt->env);
        }
    }
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
