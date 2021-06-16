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
            // 以及未从 dyld shared cache 中找到 mhdr 的 header_info 时，添加 class 的数量到 unoptimizedTotalClasses 中。
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
    // Perform one-time runtime initialization that must be deferred until the executable itself is found. 
    // 执行 one-time runtime initialization，必须推迟到找到可执行文件本身。
    // This needs to be done before further initialization.
    // 这需要在进一步初始化之前完成。
    
    // The executable may not be present in this infoList if the executable does not contain
    // Objective-C code but Objective-C is dynamically loaded later.
    // 如果可执行文件不包含 Objective-C 代码但稍后动态加载 Objective-C，则该可执行文件可能不会出现在此 infoList 中。
    
    if (firstTime) {
        // 初始化 selector 表并注册内部使用的 selectors。
        sel_init(selrefCount);
        
        // ⬇️⬇️⬇️ 这里的 arr_init 函数超重要，可看到它内部做了三件事：
        // 1. 自动释放池的初始化（实际是在 TLS 中以 AUTORELEASE_POOL_KEY 为 KEY 写入 tls_dealloc 函数（自动释放池的销毁函数：内部所有 pages pop 并 free））
        // 2. SideTablesMap 初始化，也可理解为 SideTables 的初始化（为 SideTables 这个静态全局变量开辟空间）
        // 3. AssociationsManager 的初始化，即为全局使用的关联对象表开辟空间
        // void arr_init(void) 
        // {
        //     AutoreleasePoolPage::init();
        //     SideTablesMap.init();
        //     _objc_associations_init();
        // }
        
        arr_init();
        
...

// 这一段是在较低版本下 DYLD_MACOSX_VERSION_10_13 之前的版本中禁用 +initialize fork safety，大致看看即可
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
    
    // ⬇️⬇️⬇️⬇️⬇️⬇️⬇️⬇️⬇️ 下面就来到了最核心的地方
    // 以 header_info *hList[mhCount] 数组中收集到的 images 的 header_info 为参，直接进行 image 的读取
    if (hCount > 0) {
        _read_images(hList, hCount, totalClasses, unoptimizedTotalClasses);
    }
    
    // 把开始时初始化的静态局部变量 firstTime 置为 NO
    firstTime = NO;
    
    // ⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️
    // _read_images 看完再看下面的 loadImageFuncs 函数  
    
    // Call image load funcs after everything is set up.
    // 一切设置完毕后调用 image 加载函数。
    for (auto func : loadImageFuncs) {
        for (uint32_t i = 0; i < mhCount; i++) {
            func(mhdrs[i]);
        }
    }
}
```

&emsp;从上到下我们的每一行注释都超清晰了，其中最重要的就是 `_read_images` 函数的调用，`map_images_nolock` 上半部分就是对 `const struct mach_header * const mhdrs[]` 参数的处理，把数组中的 `mach_header` 转换为 `header_info` 并存在 `header_info *hList[mhCount]` 数组中，并统计 `totalClasses` 和 `unoptimizedTotalClasses` 的数量，然后调用 `_read_images(hList, hCount, totalClasses, unoptimizedTotalClasses)` 函数，下面我们对 `_read_images` 进行学习。

&emsp;`_read_images` 超长，但是真的超级重要、超级重要、超级重要：

```c++
/***********************************************************************
* _read_images
* Perform initial processing of the headers in the linked list beginning with headerList.
* 对以 headerList 开头的链表中的 headers 进行 initial processing。
*
* Called by: map_images_nolock
*
* Locking: runtimeLock acquired by map_images
**********************************************************************/
void _read_images(header_info **hList, uint32_t hCount, int totalClasses, int unoptimizedTotalClasses)
{
    header_info *hi;
    uint32_t hIndex;
    size_t count;
    size_t i;
    
    Class *resolvedFutureClasses = nil;
    size_t resolvedFutureClassCount = 0;
    
    // 静态局部变量，如果是第一次调用 _read_images 则 doneOnce 值为 NO
    static bool doneOnce;
    
    bool launchTime = NO;
    
    // 测量 image 加载步骤的持续时间
    // 对应 objc-env.h 中的 OPTION( PrintImageTimes, OBJC_PRINT_IMAGE_TIMES, "measure duration of image loading steps")
    TimeLogger ts(PrintImageTimes);

    // 加锁
    runtimeLock.assertLocked();

    // EACH_HEADER 是给下面的 for 循环使用的宏，遍历 hList 数组中的 header_info
#define EACH_HEADER \
    hIndex = 0;         \
    hIndex < hCount && (hi = hList[hIndex]); \
    hIndex++

    // 1⃣️
    // 第一次调用 _read_images 时，doneOnce 值为 NO，会进入 if 执行里面的代码 
    if (!doneOnce) {
        // 把静态局部变量 doneOnce 置为 YES，之后调用 _read_images 都不会再进来
        // 第一次调用 _read_images 的时候，class、protocol、selector、category 都没有，
        // 需要创建容器来保存这些东西，此 if 内部，最后是创建一张存 class 的表。
        doneOnce = YES;
        
        launchTime = YES;

    // 这一段是在低版本（swifit3 之前、OS X 10.11 之前）下禁用 non-pointer isa 时的一些打印信息，
    // 为了减少我们的理解负担，这里直接进行了删除，想要学习的同学可以去看一下源码
    ...
        
        // OPTION( DisableTaggedPointers, OBJC_DISABLE_TAGGED_POINTERS, "disable tagged pointer optimization of NSNumber et al.")
        // 禁用 NSNumber 等的 Tagged Pointers 优化时
        if (DisableTaggedPointers) {
            // 内部直接把 Tagged Pointers 用到的 mask 全部置为 0
            disableTaggedPointers();
        }
        
        // OPTION( DisableTaggedPointerObfuscation, OBJC_DISABLE_TAG_OBFUSCATION, "disable obfuscation of tagged pointers")
        // 可开启 OBJC_DISABLE_TAG_OBFUSCATION，禁用 Tagged Pointer 的混淆。
        
        // 随机初始化 objc_debug_taggedpointer_obfuscator。
        // tagged pointer obfuscator 旨在使攻击者在存在缓冲区溢出或其他对某些内存的写控制的情况下更难将特定对象构造为标记指针。
        // 在设置或检索有效载荷值（payload values）时， obfuscator 与 tagged pointers 进行异或。
        // 它们在第一次使用时充满了随机性。
        initializeTaggedPointerObfuscator();

        // OPTION( PrintConnecting, OBJC_PRINT_CLASS_SETUP, "log progress of class and category setup")
        // objc[26520]: CLASS: found 25031 classes during launch 在 objc-781 下在启动时有 25031 个类（包含所有的系统类和自定义类）
        
        if (PrintConnecting) {
            _objc_inform("CLASS: found %d classes during launch", totalClasses);
        }

        // namedClasses
        // Preoptimized classes don't go in this table.
        // 4/3 is NXMapTable's load factor
        
        // isPreoptimized 如果我们有一个有效的优化共享缓存（valid optimized shared cache），则返回 YES。
        // 然后是不管三目运算符返回的是 unoptimizedTotalClasses 还是 totalClasses，它都会和后面的 4 / 3 相乘，
        // 注意是 4 / 3
        int namedClassesSize = (isPreoptimized() ? unoptimizedTotalClasses : totalClasses) * 4 / 3;
        
        // gdb_objc_realized_classes 是一张全局的哈希表，虽然名字中有 realized，但是它的名字其实是一个误称，
        // 实际上它存放的是不在 dyld shared cache 中的 class，无论该 class 是否 realized。
        gdb_objc_realized_classes = NXCreateMapTable(NXStrValueMapPrototype, namedClassesSize);
        
        // 在 objc-781 下执行到这里时，会有如下打印:
        // objc[19881]: 0.04 ms: IMAGE TIMES: first time tasks
        // 这个过程花了 0.04 毫秒
        ts.log("IMAGE TIMES: first time tasks");
    }

    // 2⃣️
    // Fix up @selector references
    // 注册并修正 selector references
    //（其实就是把 image 的 __objc_selrefs 区中的 selector 放进全局的 selector 集合中，
    // 把其中）
    static size_t UnfixedSelectors;
    {
        // 加锁 selLock
        mutex_locker_t lock(selLock);
        
        // 遍历 header_info **hList 中的 header_info
        for (EACH_HEADER) {
        
            // 如果指定的 hi 不需要预优化则跳过
            if (hi->hasPreoptimizedSelectors()) continue;
            
            // 根据 mhdr()->filetype 判断 image 是否是 MH_BUNDLE 类型
            bool isBundle = hi->isBundle();
            
            // GETSECT(_getObjc2SelectorRefs, SEL, "__objc_selrefs");
            // 获取 __objc_selrefs 区中的 SEL
            SEL *sels = _getObjc2SelectorRefs(hi, &count);
            
            // 记录数量
            UnfixedSelectors += count;
            
            // static objc::ExplicitInitDenseSet<const char *> namedSelectors;
            // 是一个静态全局 set，用来存放 Selector（名字，Selector 本身就是字符串）
            
            // 遍历把 sels 中的所有 selector 放进全局的 selector 集合中   
            for (i = 0; i < count; i++) {
            
                // sel_cname 函数内部实现是返回：(const char *)(void *)sel; 即把 SEL 强转为 char 类型
                const char *name = sel_cname(sels[i]);
                
                // 注册 SEL，并返回其地址
                SEL sel = sel_registerNameNoLock(name, isBundle);
                
                // 如果 SEL 地址发生变化，则把它设置为相同
                if (sels[i] != sel) {
                    sels[i] = sel;
                }
            }
            
        }
    }
    
    // 这里打印注册并修正 selector references 用的时间
    // 在 objc-781 下打印：objc[27056]: 0.44 ms: IMAGE TIMES: fix up selector references
    // 耗时 0.44 毫秒
    ts.log("IMAGE TIMES: fix up selector references");

    // 3⃣️
    // Discover classes. Fix up unresolved future classes. Mark bundle classes.
    // 发现 classes。修复 unresolved future classes。标记 bundle classes。
    
    // Returns if any OS dylib has overridden its copy in the shared cache
    //
    // Exists in iPhoneOS 3.1 and later 
    // Exists in Mac OS X 10.10 and later
    bool hasDyldRoots = dyld_shared_cache_some_image_overridden();

    for (EACH_HEADER) {
        if (! mustReadClasses(hi, hasDyldRoots)) {
            // Image is sufficiently optimized that we need not call readClass()
            // Image 已充分优化，我们无需调用 readClass()
            continue;
        }

        // GETSECT(_getObjc2ClassList, classref_t const, "__objc_classlist");
        // 获取 __objc_classlist 区中的 classref_t
        
        // 从编译后的类列表中取出所有类，获取到的是一个 classref_t 类型的指针 
        // classref_t is unremapped class_t* ➡️ classref_t 是未重映射的 class_t 指针
        // typedef struct classref * classref_t; // classref_t 是 classref 结构体指针
        classref_t const *classlist = _getObjc2ClassList(hi, &count);

        bool headerIsBundle = hi->isBundle();
        bool headerIsPreoptimized = hi->hasPreoptimizedClasses();

        for (i = 0; i < count; i++) {
            Class cls = (Class)classlist[i];
            
            // 重点 ⚠️⚠️⚠️⚠️ 在这里：readClass。
            // 我们留在下面单独分析。
            Class newCls = readClass(cls, headerIsBundle, headerIsPreoptimized);

            if (newCls != cls  &&  newCls) {
                // Class was moved but not deleted. Currently this occurs only when the new class resolved a future class.
                // Non-lazily realize the class below.
                
                // realloc 原型是 extern void *realloc(void *mem_address, unsigned int newsize);
                // 先判断当前的指针是否有足够的连续空间，如果有，扩大 mem_address 指向的地址，并且将 mem_address 返回，
                // 如果空间不够，先按照 newsize 指定的大小分配空间，将原有数据从头到尾拷贝到新分配的内存区域，
                // 而后释放原来 mem_address 所指内存区域（注意：原来指针是自动释放，不需要使用free），
                // 同时返回新分配的内存区域的首地址，即重新分配存储器块的地址。
                resolvedFutureClasses = (Class *)realloc(resolvedFutureClasses, (resolvedFutureClassCount+1) * sizeof(Class));
                resolvedFutureClasses[resolvedFutureClassCount++] = newCls;
            }
        }
    }

    // 这里打印发现 classes 用的时间
    // 在 objc-781 下打印：objc[56474]: 3.17 ms: IMAGE TIMES: discover classes
    // 耗时 3.17 毫秒（和前面的 0.44 毫秒比，多出不少）
    ts.log("IMAGE TIMES: discover classes");
    
    // 4⃣️
    // Fix up remapped classes
    // Class list and nonlazy class list remain unremapped.
    // Class list 和 nonlazy class list 仍未映射。
    // Class refs and super refs are remapped for message dispatching.
    // Class refs 和 super refs 被重新映射为消息调度。
    
    // 主要是修复重映射 classes，!noClassesRemapped() 在这里为 false，所以一般走不进来，
    // 将未映射 class 和 super class 重映射，被 remap 的类都是非懒加载的类
    if (!noClassesRemapped()) {
        for (EACH_HEADER) {
            // GETSECT(_getObjc2ClassRefs, Class, "__objc_classrefs");
            // 获取 __objc_classrefs 区中的类引用
            Class *classrefs = _getObjc2ClassRefs(hi, &count);
            
            // 遍历 classrefs 中的类引用，如果类引用已被重新分配或者是被忽略的弱链接类，
            // 就将该类引用重新赋值为从重映射类表中取出新类
            for (i = 0; i < count; i++) {
                // Fix up a class ref, in case the class referenced has been reallocated or is an ignored weak-linked class.
                // 修复 class ref，以防所引用的类已 reallocated 或 is an ignored weak-linked class。
                remapClassRef(&classrefs[i]);
            }
            
            // fixme why doesn't test future1 catch the absence of this?
            // GETSECT(_getObjc2SuperRefs, Class, "__objc_superrefs");
            // 获取 __objc_superrefs 区中的父类引用
            classrefs = _getObjc2SuperRefs(hi, &count);
            
            for (i = 0; i < count; i++) {
                remapClassRef(&classrefs[i]);
            }
        }
    }

    // 这里打印修复重映射 classes 用的时间
    // 在 objc-781 下打印：objc[56474]: 0.00 ms: IMAGE TIMES: remap classes
    // 耗时 0 毫秒，即 Fix up remapped classes 并没有执行 
    ts.log("IMAGE TIMES: remap classes");

#if SUPPORT_FIXUP
...
#endif

    bool cacheSupportsProtocolRoots = sharedCacheSupportsProtocolRoots();
    
    // 5⃣️ 
    // Discover protocols. Fix up protocol refs.
    // 发现 protocols，修正 protocol refs。
    for (EACH_HEADER) {
        extern objc_class OBJC_CLASS_$_Protocol;
        Class cls = (Class)&OBJC_CLASS_$_Protocol;
        ASSERT(cls);
        
        // 创建一个长度是 16 的 NXMapTable
        NXMapTable *protocol_map = protocols();
        bool isPreoptimized = hi->hasPreoptimizedProtocols();

        // Skip reading protocols if this is an image from the shared cache and we support roots
        // 如果这是来自 shared cache 的 image 并且我们 support roots，则跳过 reading protocols
        
        // Note, after launch we do need to walk the protocol as the protocol in the shared cache is marked with isCanonical()
        // and that may not be true if some non-shared cache binary was chosen as the canonical definition
        // 启动后，我们确实需要遍历协议，因为 shared cache 中的协议用 isCanonical() 标记，如果选择某些非共享缓存二进制文件作为规范定义，则可能不是这样
        
        if (launchTime && isPreoptimized && cacheSupportsProtocolRoots) {
            if (PrintProtocols) {
                _objc_inform("PROTOCOLS: Skipping reading protocols in image: %s", hi->fname());
            }
            continue;
        }

        bool isBundle = hi->isBundle();
        
        // GETSECT(_getObjc2ProtocolList, protocol_t * const, "__objc_protolist");
        // 获取 hi 的 __objc_protolist 区下的 protocol_t
        protocol_t * const *protolist = _getObjc2ProtocolList(hi, &count);
        
        for (i = 0; i < count; i++) {
            // Read a protocol as written by a compiler.
            readProtocol(protolist[i], cls, protocol_map, 
                         isPreoptimized, isBundle);
        }
    }
    
    // 这里打印发现并修正 protocols 用的时间
    // 在 objc-781 下打印：objc[56474]: 5.45 ms: IMAGE TIMES: discover protocols
    // 耗时 05.45 毫秒
    ts.log("IMAGE TIMES: discover protocols");

    // 6⃣️
    // Fix up @protocol references
    // Preoptimized images may have the right answer already but we don't know for sure.
    for (EACH_HEADER) {
        // At launch time, we know preoptimized image refs are pointing at the
        // shared cache definition of a protocol.  We can skip the check on
        // launch, but have to visit @protocol refs for shared cache images
        // loaded later.
        if (launchTime && cacheSupportsProtocolRoots && hi->isPreoptimized())
            continue;
        protocol_t **protolist = _getObjc2ProtocolRefs(hi, &count);
        for (i = 0; i < count; i++) {
            remapProtocolRef(&protolist[i]);
        }
    }

    ts.log("IMAGE TIMES: fix up @protocol references");

    // Discover categories. Only do this after the initial category
    // attachment has been done. For categories present at startup,
    // discovery is deferred until the first load_images call after
    // the call to _dyld_objc_notify_register completes. rdar://problem/53119145
    if (didInitialAttachCategories) {
        for (EACH_HEADER) {
            load_categories_nolock(hi);
        }
    }

    ts.log("IMAGE TIMES: discover categories");
    
    // 
    // Category discovery MUST BE Late to avoid potential races
    // when other threads call the new category code before
    // this thread finishes its fixups.

    // +load handled by prepare_load_methods()

    // Realize non-lazy classes (for +load methods and static instances)
    for (EACH_HEADER) {
        classref_t const *classlist = 
            _getObjc2NonlazyClassList(hi, &count);
        for (i = 0; i < count; i++) {
            Class cls = remapClass(classlist[i]);
            
            printf("non-lazy classes: %s\n", cls->mangledName());
            
            if (!cls) continue;

            addClassTableEntry(cls);

            if (cls->isSwiftStable()) {
                if (cls->swiftMetadataInitializer()) {
                    _objc_fatal("Swift class %s with a metadata initializer "
                                "is not allowed to be non-lazy",
                                cls->nameForLogging());
                }
                // fixme also disallow relocatable classes
                // We can't disallow all Swift classes because of
                // classes like Swift.__EmptyArrayStorage
            }
            realizeClassWithoutSwift(cls, nil);
        }
    }

    ts.log("IMAGE TIMES: realize non-lazy classes");

    // Realize newly-resolved future classes, in case CF manipulates them
    if (resolvedFutureClasses) {
        for (i = 0; i < resolvedFutureClassCount; i++) {
            Class cls = resolvedFutureClasses[i];
            if (cls->isSwiftStable()) {
                _objc_fatal("Swift class is not allowed to be future");
            }
            realizeClassWithoutSwift(cls, nil);
            cls->setInstancesRequireRawIsaRecursively(false/*inherited*/);
        }
        free(resolvedFutureClasses);
    }

    ts.log("IMAGE TIMES: realize future classes");

    if (DebugNonFragileIvars) {
        realizeAllClasses();
    }


    // Print preoptimization statistics
    if (PrintPreopt) {
        static unsigned int PreoptTotalMethodLists;
        static unsigned int PreoptOptimizedMethodLists;
        static unsigned int PreoptTotalClasses;
        static unsigned int PreoptOptimizedClasses;

        for (EACH_HEADER) {
            if (hi->hasPreoptimizedSelectors()) {
                _objc_inform("PREOPTIMIZATION: honoring preoptimized selectors "
                             "in %s", hi->fname());
            }
            else if (hi->info()->optimizedByDyld()) {
                _objc_inform("PREOPTIMIZATION: IGNORING preoptimized selectors "
                             "in %s", hi->fname());
            }

            classref_t const *classlist = _getObjc2ClassList(hi, &count);
            for (i = 0; i < count; i++) {
                Class cls = remapClass(classlist[i]);
                if (!cls) continue;

                PreoptTotalClasses++;
                if (hi->hasPreoptimizedClasses()) {
                    PreoptOptimizedClasses++;
                }
                
                const method_list_t *mlist;
                if ((mlist = ((class_ro_t *)cls->data())->baseMethods())) {
                    PreoptTotalMethodLists++;
                    if (mlist->isFixedUp()) {
                        PreoptOptimizedMethodLists++;
                    }
                }
                if ((mlist=((class_ro_t *)cls->ISA()->data())->baseMethods())) {
                    PreoptTotalMethodLists++;
                    if (mlist->isFixedUp()) {
                        PreoptOptimizedMethodLists++;
                    }
                }
            }
        }

        _objc_inform("PREOPTIMIZATION: %zu selector references not "
                     "pre-optimized", UnfixedSelectors);
        _objc_inform("PREOPTIMIZATION: %u/%u (%.3g%%) method lists pre-sorted",
                     PreoptOptimizedMethodLists, PreoptTotalMethodLists, 
                     PreoptTotalMethodLists
                     ? 100.0*PreoptOptimizedMethodLists/PreoptTotalMethodLists 
                     : 0.0);
        _objc_inform("PREOPTIMIZATION: %u/%u (%.3g%%) classes pre-registered",
                     PreoptOptimizedClasses, PreoptTotalClasses, 
                     PreoptTotalClasses 
                     ? 100.0*PreoptOptimizedClasses/PreoptTotalClasses
                     : 0.0);
        _objc_inform("PREOPTIMIZATION: %zu protocol references not "
                     "pre-optimized", UnfixedProtocolReferences);
    }

#undef EACH_HEADER
}
```

&emsp;第 1⃣️ 部分完成后在 objc-781 下的打印是：`objc[19881]: 0.04 ms: IMAGE TIMES: first time tasks` （机器是 m1 的 macMini），第 1⃣️ 部分的内容只有在第一次调用 `_read_images` 的时候才会执行，它主要做了两件事情：

1. 根据环境变量（`OBJC_DISABLE_TAGGED_POINTERS`）判断是否禁用 Tagged Pointer，禁用 Tagged Pointer 时所涉及到的 mask 都被设置为 0，然后根据环境变量（`OBJC_DISABLE_TAG_OBFUSCATION`）以及是否是低版本系统来判断是否禁用 Tagged Pointer 的混淆器（obfuscation），禁用混淆器时 `objc_debug_taggedpointer_obfuscator` 的值 被设置为 0，否则为其设置一个随机值。
2. 通过 `NXCreateMapTable` 根据类的数量（* 4/3，根据当前类的数量做动态扩容）创建一张哈希表（是 `NXMapTable` 结构体实例，`NXMapTable` 结构体是被作为哈希表来使用的，可通过类名（const char *）来获取 Class 对象）并赋值给 `gdb_objc_realized_classes` 这个全局的哈希表，用来通过类名来存放类对象（以及读取类对象），即这个 `gdb_objc_realized_classes` 便是一个全局的类表，只要 class 没有在共享缓存中，那么不管其实现或者未实现都会存在这个类表里面。

&emsp;第 2⃣️ 部分完成后在 objc-781 下的打印是：`objc[27056]: 0.44 ms: IMAGE TIMES: fix up selector references`（机器是 m1 的 macMini），它主要做了一件事情，注册并修正 selector。也就是当 `SEL *sels = _getObjc2SelectorRefs(hi, &count);` 中的 SEL 和通过 `SEL sel = sel_registerNameNoLock(name, isBundle);` 注册返回的 SEL 不同时，就把 sels 中的 SEL 修正为 `sel_registerNameNoLock` 中返回的地址。

&emsp;第 3⃣️ 部分完成后在 objc-781 下的打印是：`objc[11344]: 5.05 ms: IMAGE TIMES: discover classes`（机器是 m1 的 macMini），它主要做了一件事情，发现并读取 classes。

&emsp;第 4⃣️ 部分 Fix up remapped classes 打印时间是 0 毫秒，一般情况下它不会执行！

&emsp;第 5⃣️ 部分，discover protocols！



















```c++
// M1 macMini 下
objc[56474]: 0.05 ms: IMAGE TIMES: first time tasks
objc[56474]: 2.45 ms: IMAGE TIMES: fix up selector references
objc[56474]: 3.17 ms: IMAGE TIMES: discover classes
objc[56474]: 0.00 ms: IMAGE TIMES: remap classes
objc[56474]: 0.15 ms: IMAGE TIMES: fix up objc_msgSend_fixup
objc[56474]: 5.45 ms: IMAGE TIMES: discover protocols
objc[56474]: 0.00 ms: IMAGE TIMES: fix up @protocol references
objc[56474]: 0.00 ms: IMAGE TIMES: discover categories
objc[56474]: 0.23 ms: IMAGE TIMES: realize non-lazy classes
objc[56474]: 0.00 ms: IMAGE TIMES: realize future classes
```


```c++
// intel i9 下
objc[11344]: 0.02 ms: IMAGE TIMES: first time tasks
objc[11344]: 0.48 ms: IMAGE TIMES: fix up selector references
objc[11344]: 5.05 ms: IMAGE TIMES: discover classes
objc[11344]: 0.00 ms: IMAGE TIMES: remap classes
objc[11344]: 0.16 ms: IMAGE TIMES: fix up objc_msgSend_fixup
objc[11344]: 6.52 ms: IMAGE TIMES: discover protocols
objc[11344]: 0.01 ms: IMAGE TIMES: fix up @protocol references
objc[11344]: 0.00 ms: IMAGE TIMES: discover categories
objc[11344]: 0.30 ms: IMAGE TIMES: realize non-lazy classes
objc[11344]: 0.00 ms: IMAGE TIMES: realize future classes
2021-06-15 22:22:18.339697+0800 KCObjc[11344:96876] 🦁🦁🦁 main_front
2021-06-15 22:22:18.340096+0800 KCObjc[11344:96876] 🤯🤯🤯
Program ended with exit code: 0
```
























## 参考链接
**参考链接:🔗**
+ [iOS dyld加载流程](https://www.jianshu.com/p/bda67b2a3465)
+ [dyld和ObjC的关联](https://www.jianshu.com/p/3cad4212892a)
+ [OC底层原理之-类的加载过程-上（ _objc_init实现原理）](https://juejin.cn/post/6883118074426294285)
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

## iOS 内存五大分区

1. 栈区

&emsp;又称堆栈 ，由编译器自动分配释放，是用户存放程序临时创建的局部变量，也就是说我们函数括弧“{}” 中定义的变量(但不包括 static 声明的变量, static 意味着在数据段中存放变量)。除此以外, 在函数被调用时,其参数也会被压入发起调用的进程栈中, 并且待到调用结束后, 函数的返回值 也会被存放回栈中。由于 栈的后进先出特点,所以 栈 特别方便用来保存/恢复调用现场。从这个意义上讲,我们可以把 堆栈 看成一个寄存、交换临时数据的内存区。

&emsp;栈 是向低地址扩展的数据结构，是一块连续的内存区域

2. 堆区

&emsp;由程序员分配释放，分配方式类似于链表，是向高地址扩展的数据结构，是不连续的内存区域。用于存放进程运行中被动态分配的内存段，堆区的大小并不固定，可动态扩张或缩减。当进程调用 alloc 等函数分配内存时，新分配的内存就被动态添加到堆上（堆被扩张）；当利用 realse 释放内存时，被释放的内存从堆中被剔除（堆被缩减）。如果应用程序没有释放掉，操作系统会自动回收。变量通过 new、alloc、malloc、realloc 分配的内存块就存放在堆区。

3. 全局/静态区

+ 全局/静态区 是存放全局变量和静态变量的。
+ 已初始化的全局变量和静态变量存放在一块区域。
+ 未初始化的全局变量和静态变量在相邻的另一块区域。
+ 由 static 修饰的变量会成为静态变量，该变量的内存由全局/静态区在编译阶段完成分配，且仅分配一次。
+ static 可以修饰局部变量也可以修饰全局变量。
+ 全局/静态区 的内存在编译阶段完成分配，程序运行时会一直存在内存中，只有当程序结束后才会由操作系统释放。

4. 常量区

+ 常量区 是一块比较特殊的存储区，常量区里面存放的是常量，常量字符串就存放在常量区。
+ 常量区 的内存在编译阶段完成分配，程序运行时会一直存在内存中，只有当程序结束后才会由操作系统释放。

5. 代码区

&emsp;代码区 是用来存放可执行文件的操作指令（存放函数的二进制代码），其实就是存放程序的所有代码。代码区 需要防止在运行时被非法修改，所以只准许读取操作，而不允许写入（修改）操作——它是不可写的。


// duishanji4822ee@163.com
// Heiye2121

// 赢赢转使用的苹果开发者账号
// shanghaiguwan@163.com
// 1Q@w3e4r5t

// 18780334870

// 快买他使用的苹果开发者账号
// cugme9@163.com Ww115511
// qwe999 oo123-oo456-oo789 => 密保顺序对应 朋友工作父母
// 1998/8/8

// cugme9@163.com 当前密码：Cwq17150198837 绑定的手机号码：17150198837

// 富富转使用的苹果开发者账号
// feiquhui407300@126.com // 绑定手机号码：17150198837
// feiquhui407300@126.com    Ass112211
// bu3309    香港    宝马    火箭 => 密保顺序对应 朋友工作父母
// 1990/1/1

// feiquhui407300@126.com 当前密码：Cwq17150198837 绑定的手机号码：17150198837

// qwe999
// Cwq17150198837

// 好好做新买账号的原始信息
// 账号ruhan32106@21cn.com   密码Knn12355
// 密保  ooo  ppp   qqq => 密保顺序对应 朋友工作父母
// 日期1997/11/24    Nsr9455613

// ruhan32106@21cn.com 当前密码：Cwq17150198837 绑定的手机号码：18611404599

