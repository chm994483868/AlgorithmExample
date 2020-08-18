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
    if (!haveNew) ASSERT(newObj == nil); // 这里是表示，如果你开始就标识没有新值且你的 newObj == nil，则能正常执行函数，否则直接断言 crash

    Class previouslyInitializedClass = nil;
    id oldObj;
    SideTable *oldTable;
    SideTable *newTable;

    // Acquire locks for old and new values.
    // Order by lock address to prevent lock ordering problems. 
    // Retry if the old value changes underneath us.
 retry:
    if (haveOld) {
        oldObj = *location;
        oldTable = &SideTables()[oldObj];
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
    // and the +initialize machinery by ensuring that no 
    // weakly-referenced object has an un-+initialized isa.
    if (haveNew  &&  newObj) {
        Class cls = newObj->getIsa();
        if (cls != previouslyInitializedClass  &&  
            !((objc_class *)cls)->isInitialized()) 
        {
            SideTable::unlockTwo<haveOld, haveNew>(oldTable, newTable);
            class_initialize(cls, (id)newObj);

            // If this class is finished with +initialize then we're good.
            // If this class is still running +initialize on this thread 
            // (i.e. +initialize called storeWeak on an instance of itself)
            // then we may proceed but it will appear initializing and 
            // not yet initialized to the check above.
            // Instead set previouslyInitializedClass to recognize it on retry.
            previouslyInitializedClass = cls;

            goto retry;
        }
    }

    // Clean up old value, if any.
    if (haveOld) {
        weak_unregister_no_lock(&oldTable->weak_table, oldObj, location);
    }

    // Assign new value, if any.
    if (haveNew) {
        newObj = (objc_object *)
            weak_register_no_lock(&newTable->weak_table, (id)newObj, location, 
                                  crashIfDeallocating);
        // weak_register_no_lock returns nil if weak store should be rejected

        // Set is-weakly-referenced bit in refcount table.
        if (newObj  &&  !newObj->isTaggedPointer()) {
            newObj->setWeaklyReferenced_nolock();
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


  
