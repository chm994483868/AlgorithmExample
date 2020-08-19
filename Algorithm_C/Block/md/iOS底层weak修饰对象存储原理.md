#  iOS底层weak修饰对象存储原理
问题：为何 weak 修饰符的变量可以打破循环引用？
因为 weak 修饰的变量存储在散列表中的**弱引用表**里，**不参与引用计数器的使用**，也就是说，在进行释放的时候，不管你怎么引用，直接把你置空了。

基本概念：
+ `SideTable`：散列表：系统自动维护，用于存储/管理一些信息。
  `SideTable` 的结构中能看到其管理是三种表：
  `spinlock_t slock` 自旋锁表；
  `RefcountMap refcnts` 引用计数表；
  `weak_table_t weak_table` 弱引用表；
  
+ `weak_table`: 弱引用对象存储的表，是 `SideTable` 的一张表。

+ `weak_entry_t`: `weak_table` 里面的一个单元，用于管理当前类的弱引用对象，可以理解为一个数组，查看 `weak_entry_t` 的结构，有助于更好的理解里面的存储结构，里面包含一个 `weak_referrer_t`，相当于一个数组，这里面的就是存储的弱引用对象，还有其他的一些信息，比如 `mask` (蒙板，等于容量 -1)、`num_refs`（当前存储的数量）等。

+ `weak_referrer_t`：`weak_entry_t` 中的弱引用对象，相当于是数组中的一个元素。

## 存储原理：
### 1、源码探索入口
  写如下代码，打上断点，并打开汇编模式：`debug -> debug workflow -> alway show disassembly` :

```
  LGPerson *person = [[LGPerson alloc] init];
  NSLog(@"🆚🆚🆚 %@", person);
  ▶️ __weak LGPerson *weakPer = person; // 这里打断点
  NSLog(@"🆚🆚🆚 %@", weakPer);
```
运行后会进入断点，出现这样的信息：
```
....
->  0x100000e5b <+91>:  movq   -0x20(%rbp), %rsi
0x100000e5f <+95>:  leaq   -0x28(%rbp), %rbx
0x100000e63 <+99>:  movq   %rbx, %rdi
0x100000e66 <+102>: callq  0x100000ef6 ; symbol stub for: objc_initWeak
....
```
找到 `callq` 方法：`objc_initWeak`，拿到这个方法就可以进入源码调试了。

#### 源码探索
`objc_initWeak` 函数：
```
/** 
 * Initialize a fresh weak pointer to some object location.
 * 初始化一个新的 weak 指针指向某个对象的位置。
 * It would be used for code like:
 * 如以下代码的使用：
 *
 * (The nil case) // nil 的情况
 * __weak id weakPtr;
 * (The non-nil case) // 非 nil 的情况
 * NSObject *o = ...;
 * __weak id weakPtr = o; // 把 o 赋给一个 weak 变量
 * 
 * This function IS NOT thread-safe with respect to concurrent
 *  modifications to the weak variable. (Concurrent weak clear is safe.)
 * 对于 weak 变量的并发修改，此函数不是线程安全的。（并发的 weak 清除是安全的）
 * @param location Address of __weak ptr. // __weak 变量的指针指针 (ptr 是 pointer 的缩写，id 是 struct objc_object *)
 * @param newObj Object ptr. // 对象指针
 */
id
objc_initWeak(id *location, id newObj)
{
    if (!newObj) { // 如果对象不存在
        *location = nil; // 看到这个赋值用的是 *location = nil; 表示 __weak 指针变量指向 nil
        return nil; // 并且返回 nil，目前还不知道这个返回值是干什么的
    }
    // storeWeak 是一个模版函数 DontHaveOld 表示没有旧值，（这是一个新的 __weak 变量）DoHaveNew 表示有新值，即 newObj 存在，DoCrashIfDeallocating 表示如果 newObj 已经释放了就 crash
    return storeWeak<DontHaveOld, DoHaveNew, DoCrashIfDeallocating>
        (location, (objc_object*)newObj);
    // storeWeak 是一个模版参数是 enum 的模版函数，
    // template <HaveOld haveOld, HaveNew haveNew, CrashIfDeallocating crashIfDeallocating>
    // HaveOld/HaveNew/CrashIfDeallocating 三个枚举值
    // 这里 storeWeak 传入的分别是 DontHaveOld = false DoHaveNew = true DoCrashIfDeallocating = true   
}
```

### 1.1、内部做的操作是存储 weak -- storeWeak
```
// Template parameters. 模版参数
enum HaveOld { DontHaveOld = false, DoHaveOld = true }; // 是否有旧值
enum HaveNew { DontHaveNew = false, DoHaveNew = true }; // 是否有新值

// Update a weak variable. 更新一个 weak 变量。

// If HaveOld is true, the variable has an existing value that needs to be cleaned up. This value might be nil.
// 如果 HaveOld 为 true，则该变量具有需要清除的现有值。该值可能为 nil。

// If HaveNew is true, there is a new value that needs to be assigned into the variable. This value might be nil.
// 如果 HaveNew 为 true，则需要将一个新值分配给变量。该值可能为 nil。

// If CrashIfDeallocating is true, the process is halted if newObj is deallocating or newObj's class does not support weak references.
// 如果 CrashIfDeallocating 为 true，则在 newObj 已经释放了或 newObj 的类不支持弱引用时，该函数执行将暂停（crash）。

// If CrashIfDeallocating is false, nil is stored instead.
// 如果 CrashIfDeallocating 为 false，则发生以上问题时只是存入 nil。

// 模版参数，如果要赋值的对象释放了，那函数执行中是否要 crash
enum CrashIfDeallocating {
    DontCrashIfDeallocating = false, DoCrashIfDeallocating = true
};

// ASSERT(haveOld  ||  haveNew) 断言的宏定义，当括号里的条件不满足时则执行断言，即括号里面为假时则执行断言，如果为真函数就接着往下执行。同 Swift 的 guard 语句。为真时执行直接接下来的函数，为假时直接断言 crash（return）。

template <HaveOld haveOld, HaveNew haveNew,
          CrashIfDeallocating crashIfDeallocating>
static id
storeWeak(id *location, objc_object *newObj)
{
    ASSERT(haveOld  ||  haveNew); // 如果 haveOld 为假且 haveNew 为假，表示既没有新值也没有旧值，则执行断言
    if (!haveNew) ASSERT(newObj == nil); // 这里是表示，如果你开始就标识没有新值且你的 newObj == nil 确实没有新值，则能正常执行函数，否则直接断言 crash

    Class previouslyInitializedClass = nil;
    id oldObj;
    SideTable *oldTable;
    SideTable *newTable;

    // Acquire locks for old and new values. // 获取为新值和旧值的锁
    // Order by lock address to prevent lock ordering problems. // 根据锁地址排序，以防止出现 锁排序 问题。
    // Retry if the old value changes underneath us. // 重试，如果旧值在下面改变
 retry:
    if (haveOld) { // 如果有旧值，这个旧值表示是传进来的 __weak 变量，之前指向的值
        oldObj = *location; // // 把 （*location） 赋给 oldObj，即表示 oldObj 和传入的 weak 变量一样，作为一个指针，双方指向同一个对象地址
        oldTable = &SideTables()[oldObj]; // 有旧值则表示 oldTable 也能有值，目前对 SideTables 还完全不了解
    } else {
        oldTable = nil;
    }
    if (haveNew) {
        newTable = &SideTables()[newObj];
    } else {
        newTable = nil;
    }

    SideTable::lockTwo<haveOld, haveNew>(oldTable, newTable);

    if (haveOld  &&  *location != oldObj) {
        SideTable::unlockTwo<haveOld, haveNew>(oldTable, newTable);
        goto retry;
    }

    // Prevent a deadlock between the weak reference machinery
    // and the +initialize machinery by ensuring that
    // no weakly-referenced object has an un-+initialized isa.
    // 通过确保没有弱引用的对象具有未初始化的isa，防止 弱引用机制 和 +初始化机制 之间出现死锁
    
    // 有新值并且 newObj 不为空，判断类有没有初始化，如果没有初始化就进行初始化
    if (haveNew  &&  newObj) {
        Class cls = newObj->getIsa(); //
        if (cls != previouslyInitializedClass  &&
            !((objc_class *)cls)->isInitialized())
        {
            SideTable::unlockTwo<haveOld, haveNew>(oldTable, newTable);
            class_initialize(cls, (id)newObj);

            // If this class is finished with +initialize then we're good.
            // 如果这个 class，通过 +initialize 完成了初始化，这对我们而言是一个好结果。
            
            // If this class is still running +initialize on this thread
            // (i.e. +initialize called storeWeak on an instance of itself)
            // then we may proceed but it will appear initializing and
            // not yet initialized to the check above.
            // 如果这个类仍然在这个线程上运行 +initialize
            //（即在它自己的一个实例上，+initialize 调用 storeWeak），
            // 那么我们可以继续，但它将显示为正在初始化一个尚未初始化的检查。
            
            // Instead set previouslyInitializedClass to recognize it on retry.
            previouslyInitializedClass = cls;

            goto retry;
        }
    }

    // Clean up old value, if any.
    // 如果有旧值，则进行 weak_unregister_no_lock 操作
    if (haveOld) {
        weak_unregister_no_lock(&oldTable->weak_table, oldObj, location);
    }

    // Assign new value, if any.
    // 如果有新值，则进行 weak_register_no_lock 操作
    if (haveNew) {
        newObj = (objc_object *)
            weak_register_no_lock(&newTable->weak_table, (id)newObj, location,
                                  crashIfDeallocating);
        // weak_register_no_lock returns nil if weak store should be rejected

        // Set is-weakly-referenced bit in refcount table.
        if (newObj  &&  !newObj->isTaggedPointer()) {
            newObj->setWeaklyReferenced_nolock(); // 终于找到了，设置 struct objc_objcet 的 isa（isa_t）中的 uintptr_t weakly_referenced : 1;
        }

        // Do not set *location anywhere else. That would introduce a race.
        *location = (id)newObj;
    }
    else {
        // No new value. The storage is not changed.
    }
    
    SideTable::unlockTwo<haveOld, haveNew>(oldTable, newTable);

    return (id)newObj;
}
```
> 关键步骤：
> 1. 如果 cls 还没有初始化，先初始化这个类，那什么情况下会遇到呢？
> 2.  如果 weak 对象有旧值，先对旧值进行 `weak_unregister_no_lock`，删除旧值。
> 3. 如果 weak 对象有新值，就对新值进行 `weak_register_no_lock`，新增新值。

### 1.2、来看下 `weak_unregister_no_lock`，删除旧值
```
/** 
 * Unregister an already-registered weak reference.
 * 注销以前注册的弱引用。
 * This is used when referrer's storage is about to go away, but referent
 * isn't dead yet. (Otherwise, zeroing referrer later would be a
 * bad memory access.)
 * 该方法用于 referrer 的存储即将消失，但是还没有 “死亡”时。（否则，referrer 置 0 后，可能会造成一个错误的内存访问）
 * Does nothing if referent/referrer is not a currently active weak reference.
 * 如果 referent/referrer 不是当前活动的弱引用，则什么也不做
 * Does not zero referrer.
 * 不能为 0 引用。
 * 
 * FIXME currently requires old referent value to be passed in (lame)
 * // 当前需要传递旧的引用值
 * FIXME unregistration should be automatic if referrer is collected
 * // 如果 referrer 被收集了，注销应该是自动进行。
 *
 * @param weak_table The global weak table. // 全局的弱引用表
 * @param referent The object. // 旧值
 * @param referrer The weak reference. // weak 变量的指针
 */
void
weak_unregister_no_lock(weak_table_t *weak_table, id referent_id, 
                        id *referrer_id)
{
    objc_object *referent = (objc_object *)referent_id;
    objc_object **referrer = (objc_object **)referrer_id;

    weak_entry_t *entry;

    if (!referent) return;
    // 在 weak_table 中去找到有 referent 的 entry
    //（相当于在 weak_table 表中去找到包含 referent 元素的数组）
    if ((entry = weak_entry_for_referent(weak_table, referent))) {
        // 找到了这个 entry，就删除 entry 中的引用对象 - referrer
        remove_referrer(entry, referrer);
        bool empty = true;
        if (entry->out_of_line()  &&  entry->num_refs != 0) {
            empty = false;
        }
        else {
            for (size_t i = 0; i < WEAK_INLINE_COUNT; i++) {
                if (entry->inline_referrers[i]) {
                    empty = false; 
                    break;
                }
            }
        }
        
        // 如果 entry 中的引用对象没有了，删除这个 entry
        if (empty) {
            weak_entry_remove(weak_table, entry);
        }
    }

    // Do not set *referrer = nil. objc_storeWeak() requires that the 
    // value not change.
}
```
>  关键步骤:
> 1. 在 weak_table 中去找到有 referent - 引用对象的 entry (相当于在 weak_table 表中去找到包含 referent 元素的数组)
> 2. 如果找到了 entry 就删除 entry 中的 referent - 引用对象
> 3. 判断 entry 里面还有没有其他对象，如果没有，就把 entry 也 remove 掉（相当于数组中元素为空，就把这个数据也删除）

### 1.3 存储新值：`weak_register_no_lock`
```
/** 
 * Registers a new (object, weak pointer) pair. Creates a new weak
 * object entry if it does not exist.
 * 注册一个新的 (对象，weak 指针) 对。
 * 创建一个新的 weak object entry，如果它不存在的话。
 *
 * @param weak_table The global weak table. // 全局的 weak 表
 * @param referent The object pointed to by the weak reference. // weak 引用指向的对象
 * @param referrer The weak pointer address. // weak 指针地址
 */
id 
weak_register_no_lock(weak_table_t *weak_table, id referent_id, 
                      id *referrer_id, bool crashIfDeallocating)
{
    objc_object *referent = (objc_object *)referent_id;
    objc_object **referrer = (objc_object **)referrer_id;

    if (!referent  ||  referent->isTaggedPointer()) return referent_id;

    // ensure that the referenced object is viable
    // 确保引用对象是可见的
    
    // 1. 判断对象是否正在释放或者已经被释放了
    bool deallocating;
    if (!referent->ISA()->hasCustomRR()) {
        deallocating = referent->rootIsDeallocating();
    }
    else {
        // 2. 判断入参对象是否能进行 weak 引用
        BOOL (*allowsWeakReference)(objc_object *, SEL) = 
            (BOOL(*)(objc_object *, SEL))
            object_getMethodImplementation((id)referent, 
                                           @selector(allowsWeakReference));
        if ((IMP)allowsWeakReference == _objc_msgForward) {
            return nil;
        }
        deallocating =
            ! (*allowsWeakReference)(referent, @selector(allowsWeakReference)); // 通过函数指针执行函数
    }

    // 如果对象释放了且 crashIfDeallocating 为 true，则抛出 crash
    if (deallocating) {
        if (crashIfDeallocating) {
            _objc_fatal("Cannot form weak reference to instance (%p) of "
                        "class %s. It is possible that this object was "
                        "over-released, or is in the process of deallocation.",
                        (void*)referent, object_getClassName((id)referent));
        } else {
            return nil;
        }
    }

    // now remember it and where it is being stored
    weak_entry_t *entry;
    // 在 weak_table 中去找到有 referent 的 entry
    //（相当于在 weak_table 表中去找到包含 referent 元素的数组）
    if ((entry = weak_entry_for_referent(weak_table, referent))) {
        // 如果找到了，直接 append
        append_referrer(entry, referrer);
    } 
    else {
        // 如果没有找到 entry，就创建一个 entry，并插入 weak_table
        weak_entry_t new_entry(referent, referrer);
        weak_grow_maybe(weak_table);
        weak_entry_insert(weak_table, &new_entry);
    }

    // Do not set *referrer. objc_storeWeak() requires that the 
    // value not change.
    // 不要设置 *referrer。objc_storeWeak() 要求值不能改变。

    return referent_id;
}
```
> 关键步骤：
> 1. 在 weak_table 中去找有 referent 的 entry （相当于在 weak_table 表中去找到包含 referent 元素的数组）
> 2. 如果找到 entry，进行添加操作：append_referrer，如果有空位，直接插进去，这里有一个疑问：为什么会有一个空位呢？这里可以看 new_entry 的实现：初始容量为 4，并默认 4 个空值。如果数量超过容量的 3/4，进行扩容，再添加。（这里想到，方法缓存机制，方法缓存也是超过 3/4 进行扩容，方法的扩容是：扩容之后，以前的方法删掉了，再把需要缓存的方法插进去）
> 3. 如果没有找到 entry，创建一个 entry，再进行插入。

### 释放原理
弱引用对象在释放的时候，可以在 dealloc 中去看具体怎么释放的：
`dealloc -> rootDealloc -> object_dispose -> objc_destructInstance -> clearDeallocating -> clearDeallocating_slow`

1. `dealloc`函数：
```
// Replaced by NSZombies
- (void)dealloc {
    _objc_rootDealloc(self);
}
```
2. `_objc_rootDealloc` 函数：
```
void
_objc_rootDealloc(id obj)
{
    ASSERT(obj); // 如果 obj 不存在，则 crash
    obj->rootDealloc();
}
```
3. `rootDealloc` 函数：
```
inline void
objc_object::rootDealloc()
{
    if (isTaggedPointer()) return;  // fixme necessary? 有必要吗？

    // 这一步判断比较多，且符合条件是可直接调用 free 函数
    // 1. isa 是非指针类型，即优化的 ias_t 类型，除了类对象地址包含更多的信息
    // 2. 没有弱引用对象
    // 3. 没有关联对象
    // 4. 没有 C++ 析构函数 (在昨天写动态添加属性和成员变量的时候发现了方法列表里自己生成了 C++ 析构函数：name = ".cxx_destruct")，那么如果都有这个函数的话，是不是和 if 里面的 fastpath 冲突了？
    // 5. 引用计数没有超过 10
    // 满足以上条件后可以进行快速释放
    if (fastpath(isa.nonpointer  &&  
                 !isa.weakly_referenced  &&  
                 !isa.has_assoc  &&  
                 !isa.has_cxx_dtor  &&  
                 !isa.has_sidetable_rc))
    {
        assert(!sidetable_present());
        free(this);
    } 
    else {
        object_dispose((id)this);
    }
}
```
4. `object_dispose` 函数：
```
id 
object_dispose(id obj)
{
    if (!obj) return nil;

    objc_destructInstance(obj);    
    free(obj);

    return nil;
}
```
5. `objc_destructInstance` 函数：
```
/***********************************************************************
* objc_destructInstance

* Destroys an instance without freeing memory. 
// 销毁实例而不释放内存 ？
* Calls C++ destructors. // 调用 C++ destructors 函数
* Calls ARC ivar cleanup. // 清理 ARC 下的 ivar
* Removes associative references. // 移除关联对象
* Returns `obj`. Does nothing if `obj` is nil. // 返回 "obj"。如果 "obj" 为 nil，则不执行任何操作
**********************************************************************/
void *objc_destructInstance(id obj) 
{
    if (obj) {
        // Read all of the flags at once for performance.
        bool cxx = obj->hasCxxDtor();
        bool assoc = obj->hasAssociatedObjects();

        // This order is important.
        if (cxx) object_cxxDestruct(obj); // C++ 析构函数
        if (assoc) _object_remove_assocations(obj); // 移除关联对象
        
        obj->clearDeallocating(); // 弱引用的释放在这里
    }

    return obj;
}
```
6. `clearDeallocating` 函数：
```
inline void 
objc_object::clearDeallocating()
{
    if (slowpath(!isa.nonpointer)) {
        // Slow path for raw pointer isa.
        // 原始指针类型的 isa 
        sidetable_clearDeallocating();
    }
    else if (slowpath(isa.weakly_referenced  ||  isa.has_sidetable_rc)) {
        // Slow path for non-pointer isa with weak refs and/or side table data.
        clearDeallocating_slow();
    }

    assert(!sidetable_present());
}
```
7. `clearDeallocating_slow` 函数：找到散列表中的 weak_table 表，找到 weak_talbe 中的 entry，将 entry 中的引用对象 referrer 置空，最后 remove entry。
```
// Slow path of clearDeallocating() 
// for objects with nonpointer isa
// that were ever weakly referenced 
// or whose retain count ever overflowed to the side table.
NEVER_INLINE void
objc_object::clearDeallocating_slow()
{
    ASSERT(isa.nonpointer  &&  (isa.weakly_referenced || isa.has_sidetable_rc));

    SideTable& table = SideTables()[this];
    table.lock();
    if (isa.weakly_referenced) {
        weak_clear_no_lock(&table.weak_table, (id)this);
    }
    if (isa.has_sidetable_rc) {
        table.refcnts.erase(this);
    }
    table.unlock();
}

#endif
```
总之，释放的时候就是 **找到散列表中的 weak_table 表，找到 weak_table 中的 entry，将 entry 中的 引用对象 referrer  置空，最后 remove entry**。

**那个这里每一个 referrer 是指每一个 __weak 变量，entry 是指赋值给 __weak 变量的对象。**

**参考链接:🔗**
[iOS底层-- weak修饰对象存储原理](https://www.jianshu.com/p/bd4cc82e09c5)
