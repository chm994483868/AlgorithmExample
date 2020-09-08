# iOS Category 底层实现原理(二)：相关函数与加载流程

## `category` 相关函数
&emsp;`category` 的加载涉及到 `runtime` 的初始化及加载流程，因为 `runtime` 相关的内容比较多，这里只一笔带过，详细内容准备开新篇来讲。
本篇只研究`runtime` 初始化加载过程中涉及的 `category` 加载。
`Objective-C` 的运行是依赖 `runtime` 来做的，而 `runtime` 和其他系统库一样，是由 `OS X` 和 `iOS` 通过 `dyld(the dynamic link editor)` 来动态加载的。

### `_objc_init`
&emsp;在 `Source/objc-os.mm` P907 可看到其入口方法 `_objc_init`，`_objc_init` 是 `runtime` 的入口函数。
```c++
/***********************************************************************
* _objc_init
* Bootstrap initialization. 引导程序初始化。

* Registers our image notifier with dyld.
* 通过 dyld 来注册我们的境像（image）.

* Called by libSystem BEFORE library initialization time
* library 初始化之前由 libSystem 调用
**********************************************************************/
void _objc_init(void)
{
    // 用一个静态变量标记，保证只进行一次初始化
    // 下次再进入此函数会直接 return
    static bool initialized = false;
    if (initialized) return;
    initialized = true;
    
    // fixme defer initialization until an objc-using image is found?
    // fixme 推迟初始化，直到找到一个 objc-using image？
    
    // 读取会影响 runtime 的环境变量，
    // 如果需要，还可以打印一些环境变量。
    environ_init();
    
    tls_init();
    
    // 运行 C++ 静态构造函数，
    // 在 dyld 调用我们的静态构造函数之前，libc 调用 _objc_init（），因此我们必须自己做。
    static_init();
    
    runtime_init();
    
    // 初始化 libobjc 的异常处理系统，
    // 由 map_images（）调用。
    exception_init();
    
    cache_init();
    _imp_implementationWithBlock_init();

    _dyld_objc_notify_register(&map_images, load_images, unmap_image);

#if __OBJC2__
    // 标记 _dyld_objc_notify_register 的调用是否已完成。
    didCallDyldNotifyRegister = true;
#endif
}
```

### `_dyld_objc_notify_register`
```c++
_dyld_objc_notify_register(&map_images, load_images, unmap_image);

// Note: only for use by objc runtime
// Register handlers to be called when objc images are mapped, unmapped, and initialized.
// Dyld will call back the "mapped" function with an array of images that contain an objc-image-info section.
// Those images that are dylibs will have the ref-counts automatically bumped, so objc will no longer need to
// call dlopen() on them to keep them from being unloaded.  During the call to _dyld_objc_notify_register(),
// dyld will call the "mapped" function with already loaded objc images.  During any later dlopen() call,
// dyld will also call the "mapped" function.  Dyld will call the "init" function when dyld would be called
// initializers in that image.  This is when objc calls any +load methods in that image.

void _dyld_objc_notify_register(_dyld_objc_notify_mapped    mapped,
                                _dyld_objc_notify_init      init,
                                _dyld_objc_notify_unmapped  unmapped);
                                
// 三个函数指针
typedef void (*_dyld_objc_notify_mapped)(unsigned count, const char* const paths[], const struct mach_header* const mh[]);
typedef void (*_dyld_objc_notify_init)(const char* path, const struct mach_header* mh);
typedef void (*_dyld_objc_notify_unmapped)(const char* path, const struct mach_header* mh);
```
大概的意思是：
> 该方法是 `runtime` 特有的方法，该方法的调用时机是，当 `oc` 对象、镜像（ `images` ）被映射（ `mapped` ），未被映射（ `unmapped` ）以及被初始化了（ `initialized` ）。这个方法是 `dlyd` 中声明的，一旦调用该方法，调用结果会作为该函数的参数回传回来。比如，当所有的 `images` 以及 `section` 为 `“objc-image-info”` 被加载之后会回调 `mapped` 方法，在 `_objc_init` 中正是 `&map_images` 函数。`load` 方法也将在这个方法中被调用。

`map_images` 对应函数指针:
```c++
// count 文件数 paths 文件的路径 mh 文件的 header
typedef void (*_dyld_objc_notify_mapped)(unsigned count, const char* const paths[], const struct mach_header* const mh[]);
```
`load_images` 对应函数指针:
```c++
// path 文件的路径 mh 文件的 header
typedef void (*_dyld_objc_notify_init)(const char* path, const struct mach_header* mh);
```
`unmap_image` 对应函数指针:
```c++
// path 文件的路径 mh 文件的 header
typedef void (*_dyld_objc_notify_unmapped)(const char* path, const struct mach_header* mh);
```

`map_images` 方法只会调用一次，`load_images` 会调用多次，也很好理解，`map_images` 会把文件数以及文件的 `path`、`header` 等信息给到 `runtime`，`load_images` 则负责每个文件的加载等过程。

> ***看到 `_dyld_objc_notify_register` 函数的第一个参数是 `map_imags` 的函数地址。`_objc_init` 里面调用 `map_images`    最终会调用`objc-runtime-new.mm` 里面的 `_read_images` 函数，而 `category` 加载到类上面正是从 `_read_images` 函数里面开始的。***   可能这里已经发生修改，在 `load_images` 函数里面调用 `loadAllCategories()` 函数，且它的前面有一句 `didInitialAttachCategories =  true;` 这个全局静态变量默认为 `false`，在这里被设置为 `true`，且整个 `objc4` 唯一的一次赋值操作，那么可以断定: 在 `load_images`  函数里面调用 `loadAllCategories()` 一定是早于 `_read_images` 里面的 `for` 循环里面调用 `load_categories_nolock` 函数。

### `map_images`
```c++
/***********************************************************************
* map_images
* Process the given images which are being mapped in by dyld.
* 处理由 dyld 映射的给定 images。

* Calls ABI-agnostic code after taking ABI-specific locks.
* 取得 ABI-specific 锁后调用 ABI-agnostic.

* Locking: write-locks runtimeLock
* rutimeLock 是一个全局的互斥锁（mutex_t runtimeLock;）
**********************************************************************/
void
map_images(unsigned count, const char * const paths[],
           const struct mach_header * const mhdrs[])
{
    // 加锁
    mutex_locker_t lock(runtimeLock);
    // 调用 map_images_nolock 函数
    return map_images_nolock(count, paths, mhdrs);
}
```

### `map_images_nolock`
```c++
void 
map_images_nolock(unsigned mhCount, const char * const mhPaths[],
                  const struct mach_header * const mhdrs[])
{
...
if (hCount > 0) {
    _read_images(hList, hCount, totalClasses, unoptimizedTotalClasses);
}
...
}
```
`map_images_nolock` 主要做了 4 件事:
1. 拿到 `dlyd` 传过来的 `header`，进行封装 
2. 初始化 `selector` 
3. 初始化 `autorelease pool page`
4. 读取 `images`

`map_images_nolock` 参数:
+ `mhCount`: `mach-o header count`，即 `mach-o header` 个数
+ `mhPaths`: `mach-o header Paths`，即 `header` 的路径数组
+ `mhdrs`: `mach-o headers`，即 `headers`

### `_read_images`
&emsp;读取各个 `section` 中的数据并放到缓存中，这里的缓存大部分都是全局静态变量。
`GETSECT(_getObjc2CategoryList,        category_t *,    "__objc_catlist");`
之前用 `clang` 编译 `category` 文件时，看到 `DATA段下的` `__objc_catlist` 区，保存 `category` 数据。

```c++
/*
* _read_images
* Perform initial processing of the headers in the linked
* list beginning with headerList. 
* 从 headerList 起点开始对其中的 header 执行初始化
* 
* Called by: map_images_nolock
* 由 map_images_nolock 调用
*
* Locking: runtimeLock acquired by map_images
* 由 map_images 函数获取 runtimeLock 
*/
void _read_images(header_info **hList, uint32_t hCount, int totalClasses, int unoptimizedTotalClasses)
{
...
// Discover categories. Only do this after the initial category
// attachment has been done.
// 发现 categories。仅在完成初始类别附件（category_t 结构体列表，包含该类所有的类别）
// 后才执行此操作。
//（大概是指编译器生成并保存在 DATA段下的 `objc_catlist` `section` 的 `struct _category_t *` 数组吗？）

// For categories present at startup,
// discovery is deferred until the first load_images call after the
// call to _dyld_objc_notify_register completes. rdar://problem/53119145
// 对于启动时出现的类别，
// discovery 被推迟，直到 _dyld_objc_notify_register 的调用完成后第一次调用 load_images。

// 遍历加载每个 header_info 中的 category 数据
if (didInitialAttachCategories) {
    for (EACH_HEADER) {
        load_categories_nolock(hi);
    }
}
...
}

/*
 * didInitialAttachCategories
 * Whether the initial attachment of categories present at startup has been done.
 * 启动时出现的类别的初始附件是否已完成，
 */
static bool didInitialAttachCategories = false;
```

### `EACH_HEADER`
```c++
// header_info **hList 
// hList 是一个元素是 header_info * 的数组
// 循环控制
#define EACH_HEADER \
hIndex = 0;         \
hIndex < hCount && (hi = hList[hIndex]); \
hIndex++
```

### `load_images`
```c++
void
load_images(const char *path __unused, const struct mach_header *mh)
{
    if (!didInitialAttachCategories && didCallDyldNotifyRegister) {
        // 全局的唯一一次把 didInitialAttachCategories 置为 true
        didInitialAttachCategories = true;
        loadAllCategories();
    }
    ...
}
```

### `loadAllCategories`
&emsp;循环调用 `load_categories_nolock` 函数，由于目前对 `runtime` 初始化加载流程不熟悉，暂时无法定论加载 `category` 是从哪开始的，但是目前可以确定的是加载 `category` 是调用 `load_categories_nolock` 函数来做的，下面我们就详细分析 `load_categories_nolock` 函数。
```c++
static void loadAllCategories() {
    mutex_locker_t lock(runtimeLock);
    for (auto *hi = FirstHeader; hi != NULL; hi = hi->getNext()) {
        load_categories_nolock(hi);
    }
}
```

### `load_categories_nolock` 

**这里会涉及懒加载的类和非懒加载的类的，此处先不表，不影响我们阅读原始代码，我们先硬着头把函数实现一行一行读完。**

```c++
static void load_categories_nolock(header_info *hi) {
    // 是否有类属性？（目前我们还没有见过给类添加属性的操作）
    bool hasClassProperties = hi->info()->hasCategoryClassProperties();

    // 这里语法有点像 OC 的 block 
    // 先定义函数内容，然后再调用执行
    
    // 此处是在尾部调用执行
    // processCatlist(_getObjc2CategoryList(hi, &count));
    // processCatlist(_getObjc2CategoryList2(hi, &count));
    
    // _getObjc2CategoryList 和 _getObjc2CategoryList2 会给 count 赋值
    // 并且函数返回 category_t * const *catlist
    
    size_t count;
    auto processCatlist = [&](category_t * const *catlist) {
        // catlist 是保存一个 category_t * 的指针，
        // 且有一个 const 修饰，表示该指针的指向是固定的，但是指向的内存里面的内容是可以修改的
        
        // 这个数据大概是指编译器生成并保存在 DATA段下的
        // `objc_catlist` `section` 的 `struct _category_t *` 数组吗？
        
        // 遍历数组
        for (unsigned i = 0; i < count; i++) {
            // 取得 category_t 指针
            category_t *cat = catlist[i];
            // 取得 category_t 所属的类
            Class cls = remapClass(cat->cls);
            
            // struct locstamped_category_t {
            // // category
            //    category_t *cat;
            // // header 数据
            //    struct header_info *hi;
            // };
            // 构建一个 locstamped_category_t 的局部变量
            locstamped_category_t lc{cat, hi};

            if (!cls) {
                // 如类不存在，执行 log
                // Category's target class is missing (probably weak-linked).
                // Ignore the category.
                if (PrintConnecting) {
                    _objc_inform("CLASS: IGNORING category \?\?\?(%s) %p with "
                                 "missing weak-linked target class",
                                 cat->name, cat);
                }
                continue;
            }
            
            // Process this category.
            // 处理此 category。
            // 判断 cls 是否是 Stub Class
            if (cls->isStubClass()) {
                // Stub classes are never realized. 
                // Stub classes don't know their metaclass until they're initialized,
                // so we have to add categories with class methods or properties to the stub itself.
                
                // methodizeClass() will find them and add them to the metaclass as appropriate.
                // 下面这些 key 是 cls value 是 category_list 的哈希表中的数据，
                // methodizeClass（）将找到它们并将它们添加到适当的元类中。
                
                if (cat->instanceMethods ||
                    cat->protocols ||
                    cat->instanceProperties ||
                    cat->classMethods ||
                    cat->protocols ||
                    (hasClassProperties && cat->_classProperties))
                {
                    // 这里可以理解为构建 cls 与它的 category 的一个映射
                    // 可参考上节 UnattachedCategories 解析
                    
                    // 可以理解为操作 key 是 cls value 是 category_list 的哈希表,
                    // 往 cls 对应的 category_list 的 locstamped_category_t 数组中添加 lc 
                    objc::unattachedCategories.addForClass(lc, cls);
                }
            } else {
                // First, register the category with its target class.
                // Then, rebuild the class's method lists (etc) if the class is realized.
                // 首先，将 category 注册给其目标类。然后，如果该类已实现了，则重建该类的方法列表（等）。
                
                // 把实例方法、协议、属性添加到类
                if (cat->instanceMethods ||  cat->protocols
                    ||  cat->instanceProperties)
                {
                    if (cls->isRealized()) {
                        // 该类已实现，则重建类的方法列表等
                        attachCategories(cls, &lc, 1, ATTACH_EXISTING);
                    } else {
                        
                        // 这里可以理解为构建 cls 与它的 category 的一个映射
                        // 可参考上节 UnattachedCategories 解析
                        
                        // 可以理解为操作 key 是 cls value 是 category_list 的哈希表,
                        // 往 cls 对应的 category_list 的 locstamped_category_t 数组中添加 lc 
                        objc::unattachedCategories.addForClass(lc, cls);
                    }
                }

                // 看到 cat->protocols 也会被添加到元类中
                // 把类方法、协议添加到元类
                if (cat->classMethods  ||  cat->protocols
                    ||  (hasClassProperties && cat->_classProperties))
                {
                    if (cls->ISA()->isRealized()) {
                        // 该元类已实现，则重建该元类的方法列表等
                        attachCategories(cls->ISA(), &lc, 1, ATTACH_EXISTING | ATTACH_METACLASS);
                    } else {
                        // 这里可以理解为构建 cls 与它的 category 的一个映射
                        // 可参考上节 UnattachedCategories 解析
                        
                        // 可以理解为操作 key 是 cls value 是 category_list 的哈希表,
                        // 往 cls 对应的 category_list 的 locstamped_category_t 数组中添加 lc 
                        objc::unattachedCategories.addForClass(lc, cls->ISA());
                    }
                }
            }
        }
    };
    
    // 对应
    // GETSECT(_getObjc2CategoryList, category_t *, "__objc_catlist");
    // GETSECT(_getObjc2CategoryList2, category_t * const, "__objc_catlist2");
    
    // _getObjc2CategoryList 取得 DATA 段 "__objc_catlist" section 中的 category 数据
    processCatlist(_getObjc2CategoryList(hi, &count));
    
    // _getObjc2CategoryList2 取得 DATA 段 "__objc_catlist2" section 中的 category 数据
    processCatlist(_getObjc2CategoryList2(hi, &count));
}
```
看到 `category` 中的协议会同时添加到类和元类。
这里还涉及到一个点: `objc::unattachedCategories.addForClass(lc, cls);` 可理解为操作 `key` 是 `cls` `value` 是 `category_list` 的哈希表，当前 `cls` 还没有实现，那这些 `category` 的内容什么是否附加到类上的。在上一节我们看 `UnattachedCategories` 数据结构时，看到 `attachToClass` 函数就是做这个事情的，把事先保存的 `category` 数据附加到 `cls`  上。全局搜索，我们可以发现 `attachToClass` 只会在 `methodizeClass` 里面调用，然后全局搜索 `methodizeClass` 函数，发现只有在 `realizeClassWithoutSwift` 中调用它。

### `attachCategories`
```c++
// Attach method lists and properties and protocols from categories to a class.
// 将 方法列表 以及 属性 和 协议 从 categories 附加到类。

// Assumes the categories in cats are all loaded and sorted by load order, oldest categories first.
// 假定 cats 中的所有 categories 均按加载顺序进行加载和排序，最早的类别在前。（这里的按加载顺序，应该就是我们平时说的编译顺序）
// oldest categories first 是指后编译的分类在前面吗 ？
static void
attachCategories(Class cls, const locstamped_category_t *cats_list, uint32_t cats_count,
                 int flags)
{
    // log 打印被替换的函数，等结束时有时间再解析
    if (slowpath(PrintReplacedMethods)) {
        printReplacements(cls, cats_list, cats_count);
    }
    
    // log
    if (slowpath(PrintConnecting)) {
        _objc_inform("CLASS: attaching %d categories to%s class '%s'%s",
                     cats_count, (flags & ATTACH_EXISTING) ? " existing" : "",
                     cls->nameForLogging(), (flags & ATTACH_METACLASS) ? " (meta)" : "");
    }

    /*
     * Only a few classes have more than 64 categories during launch.
     * This uses a little stack, and avoids malloc.
     *
     * 在启动期时，很少能有类添加超过 64 个分类，
     * 这里直接准备一个长度为 64 的数组来存放 categories 的内容，
     * 并能避免使用 malloc。
     *
     * （这个 64 表示一个类很少会有超过 64 个分类）
     *
     * Categories must be added in the proper order, which is back to front.
     * 必须按正确的顺序添加 categories，这里是从前到后的。
     *
     * To do that with the chunking, we iterate cats_list from front to back,
     * build up the local buffers backwards, and call attachLists on the chunks. 
     * attachLists prepends the lists, so the final result is in the expected order.
     *
     * 为此，我们从前到后迭代 cats_list，然后把内容从后往前放在长度为 64 的数组内，然后再调用 attachLists。
     * attachLists 在列表的前面，因此最终结果按预期顺序排列。
     *
     */
     
    // 下面准备的长度为 64 的数组算是把 category 内容追加到本类时做的一个效率优化
    
    // 在编译时即可得出常量值，定值为 64
    constexpr uint32_t ATTACH_BUFSIZ = 64;
    // 方法列表 数组元素是 method_list_t *（其实是二维数组，不单单是指针数组）
    // 循环添加不同 category 中的方法列表
    method_list_t   *mlists[ATTACH_BUFSIZ];
    // 属性列表 数组元素是 property_list_t *（其实是二维数组，不单单是指针数组）
    // 循环添加不同 category 中的属性列表
    property_list_t *proplists[ATTACH_BUFSIZ];
    // 协议列表 数组元素是 protocol_list_t *（其实是二维数组，不单单是指针数组）
    // 循环添加不同 category 中的协议列表
    protocol_list_t *protolists[ATTACH_BUFSIZ];

    uint32_t mcount = 0;
    uint32_t propcount = 0;
    uint32_t protocount = 0;
    
    // 记录 header 的 filetype
    bool fromBundle = NO;
    
    // 根据入参 flags 判断是否是元类
    bool isMeta = (flags & ATTACH_METACLASS);
    
    // 所有的 category 的内容都是追加到 auto rwe 的
    // class_rw_ext_t  
    auto rwe = cls->data()->extAllocIfNeeded();
    
    // struct class_rw_ext_t {
    //    const class_ro_t *ro;
    //    method_array_t methods;
    //    property_array_t properties;
    //    protocol_array_t protocols;
    //    char *demangledName;
    //    uint32_t version;
    // };
    
    // 遍历入参 cats_list 
    //（使用 category_list list, 时传入的是: list.array() 取出 category_list 的 array 来遍历的）
    // 这里使用的 const locstamped_category_t *cats_list 同 category_list 中的 array 数组
    
    for (uint32_t i = 0; i < cats_count; i++) {
        // 取得 locstamped_category_t
        auto& entry = cats_list[i];
        
        // 取得 entry 的 cat（category_t *）的成员变量函数列表（instanceMethods/classMethods）
        method_list_t *mlist = entry.cat->methodsForMeta(isMeta);
        
        if (mlist) {
            // 这里如果 mcount 自增到 64 （表示 mlists 存满了。里面存放的是函数数组） 
            if (mcount == ATTACH_BUFSIZ) {
            
                // prepareMethodLists 主要做了两件事
                // 1. 保证方法列表 uniqued and sorted
                // 2. 如果分类重写了 RR/AWZ/Core 这些 NSObject 的默认函数，则进行覆盖处理
                prepareMethodLists(cls, mlists, mcount, NO, fromBundle);
                
                // 把 mlists 拼到 class_rw_ext_t 的函数列表中
                // 且是放在原函数列表的前面去
                rwe->methods.attachLists(mlists, mcount);
                
                // 处理完以后把 mcount 置为 0
                mcount = 0;
            }
            
            // 如果两个分类里面有同名函数，
            // 在 Compile Sources 里面后编译的分类文件里的函数会执行。
            // 这里的可能是涉及到的外面的 
            
            // 把 mlist 从后向前放在 mlists 数组里面
            // mcount 做自增操作
            mlists[ATTACH_BUFSIZ - ++mcount] = mlist;
            
            // 取得 hi 的 isBundle 是 true 或 false  
            fromBundle |= entry.hi->isBundle();
        }

        // 把 proplist 添加到的 class_rw_ext_t 的 properties 中
        property_list_t *proplist =
            entry.cat->propertiesForMeta(isMeta, entry.hi);
        if (proplist) {
            if (propcount == ATTACH_BUFSIZ) {
                rwe->properties.attachLists(proplists, propcount);
                propcount = 0;
            }
            proplists[ATTACH_BUFSIZ - ++propcount] = proplist;
        }

        // 把 protolist 添加到 class_rw_ext_t 的 protocols 中
        protocol_list_t *protolist = entry.cat->protocolsForMeta(isMeta);
        if (protolist) {
            if (protocount == ATTACH_BUFSIZ) {
                rwe->protocols.attachLists(protolists, protocount);
                protocount = 0;
            }
            protolists[ATTACH_BUFSIZ - ++protocount] = protolist;
        }
    }

    // 大部分类可能都从这里添加，分类不会超过 64，或者 超过了 64 的余数部分
    // 上面的 for 循环内部，只有 mcount 自增到 ATTACH_BUFSIZ 以后才会进行添加
    if (mcount > 0) {
        prepareMethodLists(cls, mlists + ATTACH_BUFSIZ - mcount, mcount, NO, fromBundle);
        rwe->methods.attachLists(mlists + ATTACH_BUFSIZ - mcount, mcount);
        if (flags & ATTACH_EXISTING) flushCaches(cls);
    }
    
    // 同上分类少于 64 或者超过了 64 的余数部分
    
    // 把 proplist 添加到的 class_rw_ext_t 的 properties 中 
    rwe->properties.attachLists(proplists + ATTACH_BUFSIZ - propcount, propcount);
    // 把 protolist 添加到 class_rw_ext_t 的 protocols 中
    rwe->protocols.attachLists(protolists + ATTACH_BUFSIZ - protocount, protocount);
}
```

### `prepareMethodLists`
```c++
static void 
prepareMethodLists(Class cls, method_list_t **addedLists, int addedCount,
                   bool baseMethods, bool methodsFromBundle)
{
    // 加锁
    runtimeLock.assertLocked();

    if (addedCount == 0) return;

    // There exist RR/AWZ/Core special cases for some class's base methods.
    // 对于某些类的 base methods，存在 RR/AWZ/Core 特殊情况。
    
    // RR: 类或其父类具有默认的: retain/release/autorelease/retainCount/
    //   _tryRetain/_isDeallocating/retainWeakReference/allowsWeakReference
    
    // AWZ: 类或其父类具有默认的: alloc/allocWithZone：（它们是类方法，存储在元类中）
    // Core: 类或其父类具有默认的：new/self/class/respondsToSelector/isKindOfClass 
    
    // But this code should never need to scan base methods for RR/AWZ/Core: 
    // default RR/AWZ/Core cannot be set before setInitialized().
    // 但是此代码应从不需要扫描 RR/AWZ/Core 方法，默认的 RR/AWZ/Core 方法不能在 setInitialized() 之前设置。
    
    // Therefore we need not handle any special cases here.
    // 因此，我们无需在这里处理任何特殊情况
    if (baseMethods) {
        // 如果 baseMethods 为 true，则 RR/AWZ/Core 必须存在，否则会直接断言
        ASSERT(cls->hasCustomAWZ() && cls->hasCustomRR() && cls->hasCustomCore());
    }

    // Add method lists to array. 将方法列表添加到数组。
    // Reallocate un-fixed method lists. 重新分配 un-fixed 的方法列表。
    // The new methods are PREPENDED to the method list array.
    // 新方法 在 方法列表数组 之前。
    for (int i = 0; i < addedCount; i++) {
        method_list_t *mlist = addedLists[i];
        ASSERT(mlist);

        // Fixup selectors if necessary
        // 这里涉及到: struct method_list_t : entsize_list_tt<method_t, method_list_t, 0x3> { ... }
        
        // 它的 FlagMask 默认是 0x3(0b11)
        // static uint32_t fixed_up_method_list = 3;
        
        // isFixedUp() == ((entsizeAndFlags & FlagMask) == fixed_up_method_list)
        // 这里真的不知道：uint32_t entsizeAndFlags; 是什么值，也看不到 method_list_t 的构造函数啥的  
        
        if (!mlist->isFixedUp()) {
        
            // 大概是指把方法列表 fixup uniqued and sorted
            // Mark method list as uniqued and sorted
            // mlist->setFixedUp();
            
            fixupMethodList(mlist, methodsFromBundle, true/*sort*/);
        }
    }

    // If the class is initialized, then scan for method implementations tracked by the class's flags. 
    // 如果该类已经完成初始化，通过该类的一些 flag 来扫描追踪它的函数实现。
    // If it's not initialized yet, then objc_class::setInitialized() will take care of it.
    // 如果该类还没有完成初始化，则 objc_class::setInitialized() 会处理它。
    
    // 处理在分类中重写了 RR/AWZ/Core 函数
    if (cls->isInitialized()) {
        objc::AWZScanner::scanAddedMethodLists(cls, addedLists, addedCount);
        objc::RRScanner::scanAddedMethodLists(cls, addedLists, addedCount);
        objc::CoreScanner::scanAddedMethodLists(cls, addedLists, addedCount);
    }
}
```

### `fixupMethodList`
&emsp;最终把方法列表: `Mark method list as uniqued and sorted`，调用的 `sel_registerNameNoLock` 函数涉及到 `SEL`，看的一头雾水，本文是专注于 `category` 的，先对其有个大概理解，等到 `SEL` 部分再深入学习。
```c++
static void 
fixupMethodList(method_list_t *mlist, bool bundleCopy, bool sort)
{
    runtimeLock.assertLocked();
    ASSERT(!mlist->isFixedUp());

    // fixme lock less in attachMethodLists ?
    // dyld3 may have already uniqued, but not sorted, the list
    if (!mlist->isUniqued()) {
        mutex_locker_t lock(selLock);
    
        // Unique selectors in list.
        for (auto& meth : *mlist) {
            const char *name = sel_cname(meth.name);
            meth.name = sel_registerNameNoLock(name, bundleCopy);
        }
    }

    // Sort by selector address.
    if (sort) {
        method_t::SortBySELAddress sorter;
        std::stable_sort(mlist->begin(), mlist->end(), sorter);
    }
    
    // Mark method list as uniqued and sorted
    mlist->setFixedUp();
}
```

### `attachLists` 
&emsp;呼...终于到这里了，真正进行分类中的内容追加到类中。开冲...
这里是把 `category` 的内容追加到 `class_rw_ext_t` 中去，这里先看函数实现。`objc_class` 涉及到的数据结构暂时不在这里展开，等开新篇再讲。 
```c++
void attachLists(List* const * addedLists, uint32_t addedCount) {
    if (addedCount == 0) return;

    if (hasArray()) {
        // many lists -> many lists
        
        // 记录之前的长度
        uint32_t oldCount = array()->count;
        uint32_t newCount = oldCount + addedCount;
        
        // realloc 原型: extern void *realloc(void *mem_address, unsigned int newsize);
        // 指针名 =（数据类型*）realloc（要改变内存大小的指针名，新的大小）
        // 返回值: 如果重新分配成功则返回指向被分配内存的指针，否则返回空指针 NULL
        
        // 先判断当前的指针是否有足够的连续空间，如果有，扩大 mem_address 指向的地址，
        // 并且将 mem_address 返回，如果空间不够，先按照 newsize 指定的大小分配空间，
        // 将原有数据从头到尾拷贝到新分配的内存区域，
        // 而后释放原来 mem_address 所指内存区域（注意：原来指针是自动释放，不需要使用 free ），
        // 同时返回新分配的内存区域的首地址，即重新分配存储器块的地址。
        
        // 新的大小 可大可小（如果新的大小大于原内存大小，则新分配部分不会被初始化）
        // 如果新的大小 小于原内存大小，可能会导致数据丢失
        // 注意事项: 
        // 重分配成功旧内存会被自动释放，旧指针变成了野指针，当内存不再使用时，应使用free()函数将内存块释放。
        
        // 扩展空间
        setArray((array_t *)realloc(array(), array_t::byteSize(newCount)));
        // 更新 array 长度 
        array()->count = newCount;
        
        // 原型：void *memmove(void* dest, const void* src, size_t count);
        // 由 src 所指内存区域复制 count 个字节到 dest 所指内存区域。
        // memmove 用于拷贝字节，如果目标区域和源区域有重叠的话，
        // memmove 能够保证源串在被覆盖之前将重叠区域的字节拷贝到目标区域中，
        // 但复制后源内容会被更改。但是当目标区域与源区域没有重叠则和 memcpy 函数功能相同。
        
        // 把方法列表向后移动，给 addedLists 留出空间 addedCount 长的空间
        memmove(array()->lists + addedCount, array()->lists, 
                oldCount * sizeof(array()->lists[0]));
        
        // 原型：void *memcpy(void *destin, void *source, unsigned n);
        // 从源 source 所指的内存地址的起始位置开始拷贝 n 个字节到目标 destin 所指的内存地址的起始位置中
        
        // 把 addedLists 复制到 array()->lists 起始的内存空间
        memcpy(array()->lists, addedLists, 
               addedCount * sizeof(array()->lists[0]));
    }
    else if (!list  &&  addedCount == 1) {
        // 0 lists -> 1 list
        // 如果目前为空，赋值操作（这里是赋值操作，这里是赋值操作）
        list = addedLists[0];
    } 
    else {
        // 1 list -> many lists
        List* oldList = list;
        uint32_t oldCount = oldList ? 1 : 0;
        uint32_t newCount = oldCount + addedCount;
        
        // 扩容
        setArray((array_t *)malloc(array_t::byteSize(newCount)));
        // 更新 count 
        array()->count = newCount;
        // 把 oldList 放在 lists 末尾
        if (oldList) array()->lists[addedCount] = oldList;
        // 把 addedLists 复制到 array()->lists 起始的内存空间
        memcpy(array()->lists, addedLists, 
               addedCount * sizeof(array()->lists[0]));
    }
}
```
这里可明确确认 `category` 中添加的函数会放在原函数的前面，当调用同名函数时，原函数会被 “覆盖”。

### `flushCaches`
```c++
/*
* _objc_flush_caches
* Flushes all caches.
* 刷新所有缓存。
* 
* (Historical behavior: flush caches for cls, its metaclass, and subclasses thereof. 
* Nil flushes all classes.)
*
* 刷新 cls metaclass subclass 的缓存，如果传 nil 刷新所有类
*
* Locking: acquires runtimeLock
*/
static void flushCaches(Class cls)
{
    runtimeLock.assertLocked();
#if CONFIG_USE_CACHE_LOCK
    mutex_locker_t lock(cacheUpdateLock);
#endif

    if (cls) {
        foreach_realized_class_and_subclass(cls, [](Class c){
            cache_erase_nolock(c);
            return true;
        });
    }
    else {
        foreach_realized_class_and_metaclass([](Class c){
            cache_erase_nolock(c);
            return true;
        });
    }
}
```

### `methodizeClass`
&emsp;把之前类不存在时，保存的类与 `category` 的映射中的数据追加到类中。
```c++
/*
* methodizeClass
* Fixes up cls's method list, protocol list, and property list.
* Attaches any outstanding categories.
* Locking: runtimeLock must be held by the caller
*/
static void methodizeClass(Class cls, Class previously)
{
    // 加锁
    runtimeLock.assertLocked();

    // 是否是元类
    bool isMeta = cls->isMetaClass();
    
    // 下节详细分析这些数据结构
    auto rw = cls->data();
    auto ro = rw->ro();
    auto rwe = rw->ext();

    // log
    // Methodizing for the first time
    if (PrintConnecting) {
        _objc_inform("CLASS: methodizing class '%s' %s", 
                     cls->nameForLogging(), isMeta ? "(meta)" : "");
    }

    // Install methods and properties that the class implements itself.
    // 从 ro 读取 baseMethods，它里面保存到都是类定义和延展中函数
    method_list_t *list = ro->baseMethods();
    
    if (list) {
        // 准备（排序和唯一，这个逻辑还没有看懂）
        prepareMethodLists(cls, &list, 1, YES, isBundleClass(cls));
        // 这里比较重要，可以理解为 ro 中的函数数据都附加到 rw 中
        if (rwe) rwe->methods.attachLists(&list, 1);
    }

    // 属性同理，也是从 ro 附加到 rw 中去
    property_list_t *proplist = ro->baseProperties;
    if (rwe && proplist) {
        rwe->properties.attachLists(&proplist, 1);
    }

    // 协议亦是同理
    protocol_list_t *protolist = ro->baseProtocols;
    if (rwe && protolist) {
        rwe->protocols.attachLists(&protolist, 1);
    }

    // Root classes get bonus method implementations if they don't have them already. 
    // These apply before category replacements.
    if (cls->isRootMetaclass()) {
        // root metaclass
        // 给 cls 添加 initialize 方法
        addMethod(cls, @selector(initialize), (IMP)&objc_noop_imp, "", NO);
    }

    // Attach categories.
    // 附加 categories
    if (previously) {
        if (isMeta) {
        
            // 附加内容
            objc::unattachedCategories.attachToClass(cls, previously,
                                                     ATTACH_METACLASS);
        } else {
            // When a class relocates, categories with class methods may be registered on the class itself rather than on the metaclass. Tell attachToClass to look for those.
            // 当为 category 而重置类时，具有类方法的 category 可能会在类本身而不是在元类上注册，
            // 告诉 attachToClass 去寻找元类附加分类内容。
            objc::unattachedCategories.attachToClass(cls, previously,
                                                     ATTACH_CLASS_AND_METACLASS);
        }
    }
    
    objc::unattachedCategories.attachToClass(cls, cls,
                                             isMeta ? ATTACH_METACLASS : ATTACH_CLASS);

#if DEBUG
    // Debug: sanity-check all SELs; log method list contents
    for (const auto& meth : rw->methods()) {
        if (PrintConnecting) {
            _objc_inform("METHOD %c[%s %s]", isMeta ? '+' : '-', 
                         cls->nameForLogging(), sel_getName(meth.name));
        }
        ASSERT(sel_registerName(sel_getName(meth.name)) == meth.name); 
    }
#endif
}

```

## 参考链接
**参考链接:🔗**
+ [结合 category 工作原理分析 OC2.0 中的 runtime](http://www.cocoachina.com/articles/17293)
+ [深入理解Objective-C：Category](https://tech.meituan.com/2015/03/03/diveintocategory.html)
+ [iOS 捋一捋Category加载流程及+load](https://www.jianshu.com/p/fd176e806cf3)
+ [iOS开发之runtime（17）：_dyld_objc_notify_register方法介绍](https://xiaozhuanlan.com/topic/6453890217)
+ [iOS开发之runtime(27): _read_images 浅析](https://xiaozhuanlan.com/topic/1452730698)
