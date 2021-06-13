# iOS APP 启动优化(五)：map_images、load_images、unmap_image 过程解析


&emsp;我们再梳理一下 dyld 的流程：

```c++
+ 在 recursiveInitialization 方法中调用 bool hasInitializers = this->doInitialization(context); 这个方法是来判断 image 是否已加载

+ doInitialization 这个方法会调用 doModInitFunctions(context) 这个方法就会进入 libSystem 框架里调用 libSystem_initializer 方法，最后就会调用 _objc_init 方法

+ _objc_init 会调用 _dyld_objc_notify_register 将 map_images、load_images、unmap_image 传入 dyld 方法 registerObjCNotifiers。

+ 在 registerObjCNotifiers 方法中，我们把 _dyld_objc_notify_register 传入的 map_images 赋值给 sNotifyObjCMapped，将 load_images 赋值给 sNotifyObjCInit，将 unmap_image 赋值给 sNotifyObjCUnmapped。

+ 在 registerObjCNotifiers 方法中，我们将传参复制后就开始调用 notifyBatchPartial()。

+ notifyBatchPartial 方法中会调用 (*sNotifyObjCMapped)(objcImageCount, paths, mhs)； 触发 map_images 方法。

+ dyld 的 recursiveInitialization 方法在调用完 bool hasInitializers = this->doInitialization(context) 方法后，会调用 notifySingle() 方法

+ 在 notifySingle() 中会调用 (*sNotifyObjCInit)(image->getRealPath(), image->machHeader());

+ 上面我们将 load_images 赋值给了 sNotifyObjCInit，所以此时就会触发 load_images 方法。

+ sNotifyObjCUnmapped 会在 removeImage 方法里触发，字面理解就是删除 Image（映射的镜像文件）。
```

## \_dyld_objc_notify_register

&emsp;开始之前我们再顺着前一篇的结尾处的 `_dyld_objc_notify_register(&map_images, load_images, unmap_image);` 函数调用往下看。

```c++
void _dyld_objc_notify_register(_dyld_objc_notify_mapped    mapped,
                                _dyld_objc_notify_init      init,
                                _dyld_objc_notify_unmapped  unmapped)
{
    dyld::registerObjCNotifiers(mapped, init, unmapped);
}
```

&emsp;所以上面传入的三个实参分别对应的三个形参：

+ `&map_images` 对应 `_dyld_objc_notify_mapped mapped` 参数
+ `load_images` 对应 `_dyld_objc_notify_init init` 参数
+ `unmap_image` 对应 `_dyld_objc_notify_unmapped unmapped` 参数

&emsp;下面我们看一下 `_dyld_objc_notify_register` 函数内部调用的 `dyld::registerObjCNotifiers` 函数的定义。

### dyld::registerObjCNotifiers

```c++
void registerObjCNotifiers(_dyld_objc_notify_mapped mapped, _dyld_objc_notify_init init, _dyld_objc_notify_unmapped unmapped)
{
    // record functions to call
    // 记录要调用的函数
    
    // ⬇️⬇️⬇️ 这里直接把 &map_images、load_images、unmap_image 三个参数直接赋值给如下三个静态全局变量（函数指针），以方便后续函数的调用！
    sNotifyObjCMapped    = mapped;
    sNotifyObjCInit        = init;
    sNotifyObjCUnmapped = unmapped;

    // call 'mapped' function with all images mapped so far
    // ⬇️⬇️⬇️ 调用 'mapped' 函数，其中包含迄今为止映射的所有 images
    try {
        notifyBatchPartial(dyld_image_state_bound, true, NULL, false, true);
    }
    catch (const char* msg) {
        // ignore request to abort during registration
    }

    // <rdar://problem/32209809> call 'init' function on all images already init'ed (below libSystem)
    // ⬇️⬇️⬇️ <rdar://problem/32209809> 在所有已经初始化的 images 上调用 'init' 函数（在 libSystem 下面）
    for (std::vector<ImageLoader*>::iterator it=sAllImages.begin(); it != sAllImages.end(); it++) {
        ImageLoader* image = *it;
        if ( (image->getState() == dyld_image_state_initialized) && image->notifyObjC() ) {
            dyld3::ScopedTimer timer(DBG_DYLD_TIMING_OBJC_INIT, (uint64_t)image->machHeader(), 0, 0);
            
            // ⬇️⬇️⬇️ 调用 sNotifyObjCInit 函数，即调用我们上面实参传入的 load_images 函数   
            (*sNotifyObjCInit)(image->getRealPath(), image->machHeader());
        }
    }
}
```

&emsp;`sNotifyObjCMapped`、`sNotifyObjCInit`、`sNotifyObjCUnmapped` 三个静态全局变量（函数指针）的声明和对应的类型如下:

```c++
typedef void (*_dyld_objc_notify_mapped)(unsigned count, const char* const paths[], const struct mach_header* const mh[]);
typedef void (*_dyld_objc_notify_init)(const char* path, const struct mach_header* mh);
typedef void (*_dyld_objc_notify_unmapped)(const char* path, const struct mach_header* mh);

static _dyld_objc_notify_mapped        sNotifyObjCMapped;
static _dyld_objc_notify_init        sNotifyObjCInit;
static _dyld_objc_notify_unmapped    sNotifyObjCUnmapped;
```

&emsp;在 `notifyBatchPartial(dyld_image_state_bound, true, NULL, false, true);` 函数调用中会调用 `(*sNotifyObjCMapped)(objcImageCount, paths, mhs);`，即我们的 `map_images` 函数。

```c++
static void notifyBatchPartial(dyld_image_states state, bool orLater, dyld_image_state_change_handler onlyHandler, bool preflightOnly, bool onlyObjCMappedNotification)
{
...
if ( objcImageCount != 0 ) {
    dyld3::ScopedTimer timer(DBG_DYLD_TIMING_OBJC_MAP, 0, 0, 0);
    uint64_t t0 = mach_absolute_time();
    (*sNotifyObjCMapped)(objcImageCount, paths, mhs); // ⬅️ 调用 map_images 函数
    uint64_t t1 = mach_absolute_time();
    ImageLoader::fgTotalObjCSetupTime += (t1-t0);
}
...
}
```

## map_images

&emsp;下面我们就开始看下在 objc/Source/objc-runtime-new.m 中声明的极其重要的 `map_images` 函数。

&emsp;处理由 dyld 映射的给定的 images。在获取特定于 ABI 的锁后调用与 ABI 无关的代码。

```c++
/***********************************************************************
* map_images
* Process the given images which are being mapped in by dyld.
* Calls ABI-agnostic code after taking ABI-specific locks.
*
* Locking: write-locks runtimeLock
**********************************************************************/
void
map_images(unsigned count, const char * const paths[],
           const struct mach_header * const mhdrs[])
{
    // 加锁
    mutex_locker_t lock(runtimeLock);
    // 调用 map_images_nolock
    return map_images_nolock(count, paths, mhdrs);
}
```

&emsp;注释告诉我们 `map_images` 是用来处理 dyld 映射的 images，可看到加锁（runtimeLock）后，直接调用 `map_images_nolock`，下面我们看一下它的定义。

### map_images_nolock 

&emsp;处理由 dyld 映射的给定 images。执行所有类注册和 fixups（or deferred pending discovery of missing superclasses etc），并调用 +load 方法。info[] 是自下而上的顺序，即 libobjc 在数组中将比链接到 libobjc 的任何库更早。 

&emsp;开启 `OBJC_PRINT_IMAGES` 环境变量时，启动时则打印 images 数量以及具体的 image。如 objc-781 下有此打印: `objc[10503]: IMAGES: processing 296 newly-mapped images...`.

```c++
/***********************************************************************
* map_images_nolock
* Process the given images which are being mapped in by dyld.
* All class registration and fixups are performed (or deferred pending discovery of missing superclasses etc), and +load methods are called.
*
* info[] is in bottom-up order i.e. libobjc will be earlier in the array than any library that links to libobjc.
*
* Locking: loadMethodLock(old) or runtimeLock(new) acquired by map_images.
**********************************************************************/
#if __OBJC2__
#include "objc-file.h"
#else
#include "objc-file-old.h"
#endif

void 
map_images_nolock(unsigned mhCount, const char * const mhPaths[],
                  const struct mach_header * const mhdrs[])
{
    // 局部静态变量，表示第一次调用
    static bool firstTime = YES;
    
    // hList 是统计 mhdrs 中的每个 mach_header 对应的 header_info
    header_info *hList[mhCount];
    
    uint32_t hCount;
    size_t selrefCount = 0;

    // Perform first-time initialization if necessary. 如有必要，执行首次初始化。
    // This function is called before ordinary library initializers. 此函数在 ordinary library 初始化程序之前调用。
    // fixme defer initialization until an objc-using image is found? 推迟初始化，直到找到一个 objc-using image？
    
    // 如果是第一次加载，则准备初始化环境
    if (firstTime) {
        preopt_init();
    }

    // 开启 OBJC_PRINT_IMAGES 环境变量时，启动时则打印 images 数量。
    // 如：objc[10503]: IMAGES: processing 296 newly-mapped images... 
    if (PrintImages) {
        _objc_inform("IMAGES: processing %u newly-mapped images...\n", mhCount);
    }

    // Find all images with Objective-C metadata.
    hCount = 0;

    // Count classes. Size various table based on the total.
    // 计算 class 的数量。根据总数调整各种表格的大小。
    
    int totalClasses = 0;
    int unoptimizedTotalClasses = 0;
    {
        uint32_t i = mhCount;
        while (i--) {
        
            // typedef struct mach_header_64 headerType;
            // 取得指定 image 的 header 指针
            const headerType *mhdr = (const headerType *)mhdrs[i];
            
            // 以 mdr 构建其 header_info，并添加到全局的 header 列表中（是一个链表，大概看源码到现在还是第一次看到链表的使用）。
            // 且通过 GETSECT(_getObjc2ClassList, classref_t const, "__objc_classlist"); 读取 __objc_classlist 区中的 class 数量添加到 totalClasses 中，
            // 以及从 dyld shared cache 中未找到 mhdr 的 header_info 时，添加 class 的数量到 unoptimizedTotalClasses 中。
            auto hi = addHeader(mhdr, mhPaths[i], totalClasses, unoptimizedTotalClasses);
            
            // 这里有两种情况下 hi 为空：
            // 1. mhdr 的 magic 不是既定的 MH_MAGIC、MH_MAGIC_64、MH_CIGAM、MH_CIGAM_64 中的任何一个
            // 2. 从 dyld shared cache 中找到了 mhdr 的 header_info，并且 isLoaded 为 true（）
            if (!hi) {
                // no objc data in this entry
                continue;
            }
            
            // #define MH_EXECUTE 0x2 /* demand paged executable file demand 分页可执行文件 */ 
            if (mhdr->filetype == MH_EXECUTE) {
                // Size some data structures based on main executable's size
                // 根据主要可执行文件的大小调整一些数据结构的大小

                size_t count;
                
                // ⬇️ GETSECT(_getObjc2SelectorRefs, SEL, "__objc_selrefs");
                // 获取 __objc_selrefs 区中的 SEL 的数量
                _getObjc2SelectorRefs(hi, &count);
                selrefCount += count;
                
                // GETSECT(_getObjc2MessageRefs, message_ref_t, "__objc_msgrefs"); 
                // struct message_ref_t {
                //     IMP imp;
                //     SEL sel;
                // };
                // ⬇️ 获取 __objc_msgrefs 区中的 message 数量
                _getObjc2MessageRefs(hi, &count);
                selrefCount += count;
...
            }
            
            hList[hCount++] = hi;
            
            if (PrintImages) {
                // 打印 image 信息
                // 如：objc[10565]: IMAGES: loading image for /usr/lib/system/libsystem_blocks.dylib (has class properties) (preoptimized)
                _objc_inform("IMAGES: loading image for %s%s%s%s%s\n", 
                             hi->fname(),
                             mhdr->filetype == MH_BUNDLE ? " (bundle)" : "",
                             hi->info()->isReplacement() ? " (replacement)" : "",
                             hi->info()->hasCategoryClassProperties() ? " (has class properties)" : "",
                             hi->info()->optimizedByDyld()?" (preoptimized)":"");
            }
        }
    }

    // ⬇️⬇️⬇️
    // Perform one-time runtime initialization that must be deferred until 
    // the executable itself is found. This needs to be done before 
    // further initialization.
    // (The executable may not be present in this infoList if the 
    // executable does not contain Objective-C code but Objective-C 
    // is dynamically loaded later.
    if (firstTime) {
        sel_init(selrefCount);
        arr_init();

#if SUPPORT_GC_COMPAT
        // Reject any GC images linked to the main executable.
        // We already rejected the app itself above.
        // Images loaded after launch will be rejected by dyld.

        for (uint32_t i = 0; i < hCount; i++) {
            auto hi = hList[i];
            auto mh = hi->mhdr();
            if (mh->filetype != MH_EXECUTE  &&  shouldRejectGCImage(mh)) {
                _objc_fatal_with_reason
                    (OBJC_EXIT_REASON_GC_NOT_SUPPORTED, 
                     OS_REASON_FLAG_CONSISTENT_FAILURE, 
                     "%s requires Objective-C garbage collection "
                     "which is no longer supported.", hi->fname());
            }
        }
#endif

#if TARGET_OS_OSX
        // Disable +initialize fork safety if the app is too old (< 10.13).
        // Disable +initialize fork safety if the app has a
        //   __DATA,__objc_fork_ok section.

        if (dyld_get_program_sdk_version() < DYLD_MACOSX_VERSION_10_13) {
            DisableInitializeForkSafety = true;
            if (PrintInitializing) {
                _objc_inform("INITIALIZE: disabling +initialize fork "
                             "safety enforcement because the app is "
                             "too old (SDK version " SDK_FORMAT ")",
                             FORMAT_SDK(dyld_get_program_sdk_version()));
            }
        }

        for (uint32_t i = 0; i < hCount; i++) {
            auto hi = hList[i];
            auto mh = hi->mhdr();
            if (mh->filetype != MH_EXECUTE) continue;
            unsigned long size;
            if (getsectiondata(hi->mhdr(), "__DATA", "__objc_fork_ok", &size)) {
                DisableInitializeForkSafety = true;
                if (PrintInitializing) {
                    _objc_inform("INITIALIZE: disabling +initialize fork "
                                 "safety enforcement because the app has "
                                 "a __DATA,__objc_fork_ok section");
                }
            }
            break;  // assume only one MH_EXECUTE image
        }
#endif

    }

    if (hCount > 0) {
        _read_images(hList, hCount, totalClasses, unoptimizedTotalClasses);
    }

    firstTime = NO;
    
    // Call image load funcs after everything is set up.
    for (auto func : loadImageFuncs) {
        for (uint32_t i = 0; i < mhCount; i++) {
            func(mhdrs[i]);
        }
    }
}
```






































## 参考链接
**参考链接:🔗**
+ [线程本地存储TLS(Thread Local Storage)的原理和实现——分类和原理](https://www.cnblogs.com/zhoug2020/p/6497709.html)
+ [imp_implementationWithBlock()的实现机制](https://www.jianshu.com/p/c52bc284e9c7)
+ [iOS 底层原理之—dyld 与 objc 的关联](https://www.yuque.com/ioser/spb08a/alu6tz)
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
