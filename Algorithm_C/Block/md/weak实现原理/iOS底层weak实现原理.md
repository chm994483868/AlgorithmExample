#  iOS底层-weak实现原理

> 提起 `weak` 我们脑海中大概会浮现出如下印象：
  1. 当我们直接把对象赋值给 `__weak` 变量时，编译器会提示我们 `Assigning retained object to weak variable; object will be released after assignment`，即把对象直接赋值给 `weak` 修饰的变量，`weak` 变量不会持有所赋值的对象，不会增加对象的引用计数，对象会立即得到释放。
  2. 当 `__weak` 修饰的变量所引用的对象释放后，`__weak` 变量会被自动置为 `nil` 而不是野指针，避免访问野指针导致的 `crash`。
  3. `weak` 修饰的属性，
  
  那么下面我们来一步一步分析 `weak` 的实现细节。

# weak 修饰符的实现原理
## 1、寻找源码入口
在 main 函数里面写如下代码，打上断点，并打开汇编模式：`debug -> debug workflow -> alway show disassembly` :
```objective-c
  #import <Foundation/Foundation.h>
  int main(int argc, const char * argv[]) {
      @autoreleasepool {
          // insert code here...
          NSObject *obj = [[NSObject alloc] init];
          __weak id weakPtr = obj; // ⬅️ 在这一行打断点
      }
      return 0;
  }
```
运行后会进入断点，并显示出这样的信息：
```c++
....
->  0x100000dcf <+63>:  movq   -0x10(%rbp), %rsi
0x100000dd3 <+67>:  leaq   -0x18(%rbp), %rdi
0x100000dd7 <+71>:  callq  0x100000ebe               ; symbol stub for: objc_initWeak // callq 指令表示要去执行 objc_initWeak 函数
0x100000ddc <+76>:  leaq   -0x18(%rbp), %rdi
0x100000de0 <+80>:  callq  0x100000eb2               ; symbol stub for: objc_destroyWeak
0x100000de5 <+85>:  leaq   -0x10(%rbp), %rdi
0x100000de9 <+89>:  xorl   %esi, %esi
0x100000deb <+91>:  callq  0x100000eca               ; symbol stub for: objc_storeStrong
0x100000df0 <+96>:  movq   %rbx, %rdi
0x100000df3 <+99>:  callq  0x100000ea6               ; symbol stub for: objc_autoreleasePoolPop
0x100000df8 <+104>: xorl   %eax, %eax
0x100000dfa <+106>: addq   $0x28, %rsp
0x100000dfe <+110>: popq   %rbx
0x100000dff <+111>: popq   %rbp
0x100000e00 <+112>: retq   
```
`callq` 指令表示函数调用，看到与 `weak` 相关的是: `objc_initWeak` 和 `objc_destroyWeak`。

## 2、探索源码实现
首先在 `objc4-781` 中找 `objc_initWeak` 实现:

在 `Private Headers/objc-internal.h` P771 看到 `objc_initWeak` 函数声明:
```
OBJC_EXPORT id _Nullable 
objc_initWeak(id _Nullable * _Nonnull location, id _Nullable val)
    OBJC_AVAILABLE(10.7, 5.0, 9.0, 1.0, 2.0);
```
看到是 iOS 5.0 后出现的，这里联想到 ARC、weak 关键字等都是 iOS 5.0 后推出的。

在 `Source/NSObject.mm` P415  是 `objc_initWeak` 函数实现:

```c++
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
 * 对于 weak 变量的并发修改，不是线程安全的。（并发的 weak 清除是安全的）
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
    
    // storeWeak 是一个模版函数 DontHaveOld 表示没有旧值，
    //（这是一个新的 __weak 变量）DoHaveNew 表示有新值，即 newObj 存在，
    // DoCrashIfDeallocating 表示如果在下面的函数执行过程中 newObj 释放了就 crash
    
    return storeWeak<DontHaveOld, DoHaveNew, DoCrashIfDeallocating>
        (location, (objc_object*)newObj);
        
    // storeWeak 是一个模版参数是 enum 的模版函数，
    // template <HaveOld haveOld, HaveNew haveNew, CrashIfDeallocating crashIfDeallocating>
    
    // HaveOld/HaveNew/CrashIfDeallocating 三个枚举值
    // 这里 storeWeak 传入的分别是
    // DontHaveOld = false（初始化新的 __weak 变量） 
    // DoHaveNew = true 
    // DoCrashIfDeallocating = true   
}
```
该方法接受两个参数:
1. `id *location`：__weak 指针的地址，即示例中 `weak` 指针取地址: `&weakPtr`，它是一个指针的指针，之所以要存储指针的地址，是因为引用对象释放后 __weak 指针指向的内容要置为 nil，如果仅存储指针（即指针所指向的地址值）的话，是不能够完成这个设置的。
  > 这里联想到了对链表做一些操作时，我们的函数入参会是链表头指针的指针。
    这里头脑好像转不过来，为什么用指针的指针，我们直接在函数内修改参数的指向时，不是同样也修改了外部指针的指向吗？其实非然！
    一定要理清，当函数形参是指针时，实参传入的是一个地址，然后在函数内部创建一个指针变量这个指针变量指向的地址是实参传入的地址，此时如果你修改指向的话，修改的只是函数内部的临时的一个指针变量。外部的指针变量是与它无关的，有关的只是它们两个指向的地址是一样的。而我们对这个地址的所有操作，都是可反应到外部指针变量那里的，这个地址是指针指向的地址，如果没有 `const` 限制，我们可以对该地址里面的内容做任何操作即使把内容置空放0，这些操作都是对这个地址的内存做的，不管怎样这块内存都是存在的，它地址一直都在这里，而我们的原始指针一直就是指向它，此时我们需要的是修改原始指针的指向，那我们只有知道指针自身的地址才行，我们把指针自身的地址对应的内存里面放 `0x0`, 才能表示把我们的指针指向置为 `nil` 了！

2. `id newObj`: 所用的对象，即示例代码中的 `obj`
该方法有一个返回值，返回的是 `storeWeak` 函数的返回值：
返回的其实还是 `obj`, 但是已经对 `obj` 的 `isa（isa_t）` 的 `weakly_referenced` 位设置为 1，标志该引用对象有弱引用指向了，当该对象销毁时，要处理之前指向它的弱引用，`__weak` 变量被置为 `nil` 的机制就是从这里开始的。 

内部做的操作是存储 `weak -- storeWeak`
`storeWeak` 函数实现的核心功能:
+ 将 `weak` 指针的地址 `location` 存入 `obj` 对应的 `weak_entry_t` 的数组（链表）中，用于在 `obj` 析构时，通过该数组（链表）找到其所有的 `weak` 指针引用，将指针指向的地址（*location）置为 `nil`。
+ 如果启用了 `isa` 优化，则将 `obj` 的 `isa_t` 的 `weakly_referenced` 位置为 1，置为 1 的作用主要标记 `obj` 被 `weak` 引用了，当 `dealloc` 时，`runtime` 会根据 `weakly_referenced` 标志位来判断是否需要查找 `obj` 对应的 `weak_entry_t`，并将引用置为 `nil`。

下面分析 `storeWeak` 函数源码实现：
```c++
// Template parameters. 模版参数
enum HaveOld { DontHaveOld = false, DoHaveOld = true }; // 是否有旧值
enum HaveNew { DontHaveNew = false, DoHaveNew = true }; // 是否有新值

// Update a weak variable. 更新一个 weak 变量。

// If HaveOld is true, the variable has an existing value that needs to be cleaned up. This value might be nil.
// 如果 HaveOld 为 true，则该变量具有需要清除的现有值。该值可能为 nil。

// If HaveNew is true, there is a new value that needs to be assigned into the variable. This value might be nil.
// 如果 HaveNew 为 true，则需要将一个新值分配给变量。该值可能为 nil。

// If CrashIfDeallocating is true, the process is halted if newObj is deallocating or newObj's class does not support weak references.
// 如果 CrashIfDeallocating 为 true，则在 newObj 释放了或 newObj 的类不支持弱引用时，该函数执行将暂停（crash）。

// If CrashIfDeallocating is false, nil is stored instead.
// 如果 CrashIfDeallocating 为 false，则发生以上问题时只是存入 nil。

// 模版参数，如果要赋值的对象释放了，那函数执行会 crash
enum CrashIfDeallocating {
    DontCrashIfDeallocating = false, DoCrashIfDeallocating = true
};

// ASSERT(haveOld  ||  haveNew) 断言的宏定义，当括号里的条件不满足时则执行断言，即括号里面为假时则执行断言，如果为真函数就接着往下执行。同 Swift 的 guard 语句。为真时执行接下来的函数，为假时直接断言 crash（return）。

template <HaveOld haveOld, HaveNew haveNew,
          CrashIfDeallocating crashIfDeallocating>
static id
storeWeak(id *location, objc_object *newObj)
{
    ASSERT(haveOld  ||  haveNew); // 如果 haveOld 为假且 haveNew 为假，表示既没有新值也没有旧值，则执行断言
    if (!haveNew) ASSERT(newObj == nil); // 这里是表示，如果你开始就标识没有新值且你的 newObj == nil 确实没有新值，则能正常执行函数，否则直接断言 crash

    Class previouslyInitializedClass = nil; // 指向 objc_class 的指针，指向事先已经初始化的 Class
    id oldObj; // __weak 变量之前指向的旧对象
    SideTable *oldTable;
    SideTable *newTable;

    // Acquire locks for old and new values. // 为新值和旧值获取锁
    // Order by lock address to prevent lock ordering problems. // 根据锁地址排序，以防止出现 锁排序 问题。
    // Retry if the old value changes underneath us. // 重试，如果旧值在下面改变，这里用到 C 语言的 goto 语句，goto 语句可以直接跳到指定的位置执行（直接修改函数执行顺序）
 retry:
    if (haveOld) { 
        // 如果有旧值，这个旧值表示是传进来的 __weak 变量，之前指向的值
        // 把（*location）赋给 oldObj，
        // 把之前指向的旧值保存在 oldObj 中
        // 作为一个指针，双方现在指向同一个对象地址
        oldObj = *location;
        // 有旧值则表示 oldTable 也能有值，
        // 目前对 SideTables 还完全不了解
        // 大概是从全局的 SideTables 找到个这个
        // 旧对象所处的 SideTable 吗？
        
        // 如果 weak ptr 之前弱引用过一个 obj，则将这个 obj 所对应的 SideTable 取出，
        // 赋值给 oldTable
        oldTable = &SideTables()[oldObj];
    } else {
        // 如果 weak prt 之前没有弱引用过一个 obj，则 oldTable = nil
        oldTable = nil;
    }
    if (haveNew) {
        // 新对象所处的 SideTable 吗？
        // 如果 weak ptr 要 weak 引用一个新的 obj，则将该 obj 对应的 SideTable 取出，
        // 赋值给 newTable
        newTable = &SideTables()[newObj];
    } else {
        // 如果 weak ptr 不需要引用一个新 obj，
        // 则 newTable = nil
        newTable = nil;
    }

    // 这里是根据 haveOld 和 haveNew 两个值，
    // 判断是否对 oldTable 和 newTable 这两个 SideTable 加锁吗？
    
    // 加锁操作，防止多线程中竞争冲突
    SideTable::lockTwo<haveOld, haveNew>(oldTable, newTable);

    // location 应该与 oldObj 保持一致，如果不同，说明当前的 location 已经处理过 oldObj 
    // 可能又被其他线程所修改
    if (haveOld  &&  *location != oldObj) {
        // 觉的走到这里 *location 应该和 oldObj 是一样的吧，
        // 如果不一样则解锁，重到 tretry 处执行函数吗？
        SideTable::unlockTwo<haveOld, haveNew>(oldTable, newTable);
        goto retry;
    }

    // Prevent a deadlock between the weak reference machinery
    // and the +initialize machinery by ensuring that
    // no weakly-referenced object has an un-+initialized isa.
    // 通过确保没有弱引用的对象具有已经初始化的isa，
    // 防止 weak reference machinery 和 +initialize machinery 之间出现死锁
    
    // 有新值 haveNew 并且 newObj 不为空，
    // 判断类有没有初始化，如果没有初始化就进行初始化
    if (haveNew  &&  newObj) {
        Class cls = newObj->getIsa();
        if (cls != previouslyInitializedClass  &&
            !((objc_class *)cls)->isInitialized())
        { // 如果 cls 还没有初始化，先初始化，再尝试设置 weak
            // 解锁
            SideTable::unlockTwo<haveOld, haveNew>(oldTable, newTable);
            // 调用对象所在类的(不是元类)初始化方法，
            // 即 调用的是 [newObjClass initialize]; 类方法
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
            
            // 如果这个类在这个线程中完成了 +initialize 的任务，那么这很好。
            // 如果这个类还在这个线程中继续执行着 +initialize 任务，
            // (比如，这个类的实例在调用 storeWeak 方法，而 storeWeak 方法调用了 +initialize .)
            // 这样我们可以继续运行，但在上面它将进行初始化和尚未初始化的检查。
            // 相反，在重试时设置 previouslyInitializedClass 为这个类来识别它。
            // Instead set previouslyInitializedClass to recognize it on retry.
            // 这里记录一下 previouslyInitializedClass，防止该 if 分支再次进入
            previouslyInitializedClass = cls;

            goto retry; // 重新获取一遍 newObj，这时的 newObj 应该已经初始化过了
        }
    }

    // Clean up old value, if any.
    // 如果有旧值，则进行 weak_unregister_no_lock 操作
    if (haveOld) {
        weak_unregister_no_lock(&oldTable->weak_table, oldObj, location);
    }

    // Assign new value, if any.
    // 如果有新值，则进行 weak_register_no_lock 操作
    if (haveNew) { // 如果 weak_ptr 需要弱引用新的对象 newObj
        // (1) 调用 weak_register_no_lock 方法，
        // 将 weak ptr 的地址记录到 newObj 对应的 weak_entry_t 中
        newObj = (objc_object *)
            weak_register_no_lock(&newTable->weak_table, (id)newObj, location,
                                  crashIfDeallocating);
        // weak_register_no_lock returns nil if weak store should be rejected

        // Set is-weakly-referenced bit in refcount table.
        // 在 refcount 表中设置 weakly_referenced 位，
        // 表示该对象被弱引用了，当该对象被释放时就是通过这个标志位
        // 来清理 weak 变量，把它们设置为 nil 的
        // (2) 更新 newObj 的 isa 的 weakly_referenced bit 标志位
        if (newObj  &&  !newObj->isTaggedPointer()) {
            newObj->setWeaklyReferenced_nolock(); // 终于找到了，设置 struct objc_objcet 的 isa（isa_t）中的 uintptr_t weakly_referenced : 1;
        }

        // Do not set *location anywhere else. That would introduce a race.
        // 请勿在其他地方设置 *location，可能会引起竟态
        //（3）*location 赋值，也就是将weak ptr直接指向了newObj。可以看到，这里并没有将newObj的引用计数+1
        *location = (id)newObj;
    }
    else {
        // No new value. The storage is not changed.
        // 没有新值，则不发生改变
    }
    
    // 解锁，其他线程可以访问oldTable, newTable了
    SideTable::unlockTwo<haveOld, haveNew>(oldTable, newTable);
    // 返回 newObj，此时的 newObj 与刚传入时相比，weakly_referenced bit位置1
    return (id)newObj;
}
```
分析 `storeWeak` 方法：
`storeWeak` 方法实质上接受5个参数，其中`HaveOld haveOld, HaveNew haveNew, CrashIfDeallocating crashIfDeallocating` 这三个参数是以模板枚举的方式传入的，其实这是三个`bool`参数，分别表示：`weak ptr` 之前是否已经指向了一个弱引用，`weak ptr` 是否需要指向一个新引用， 如果被弱引用的对象正在析构，此时再弱引用该对象，是否应该 `crash`。

具体到 `objc_initWeak`，这三个参数的值分别为`false，true，true`。

`storeWeak` 另外两个参数是`id *location, objc_object *newObj`，这两个参数和`objc_initWeak`是一样的，分别代表`weak` 指针的地址，以及被`weak`引用的对象。

涉及到两个关键函数: 
```c++
void weak_unregister_no_lock(weak_table_t *weak_table, id referent_id, id *referrer_id); // 将 weak ptr 地址 从 obj 的 weak_entry_t 中移除
id weak_register_no_lock(weak_table_t *weak_table, id referent_id, id *referrer_id, bool crashIfDeallocating); // 将 weak ptr 地址 注册到 obj 对应的 weak_entry_t 中
```
继续看下 `weak_unregister_no_lock`，将 `weak ptr` 地址 从 `obj` 的 `weak_entry_t` 中移除:
如果 `weak ptr` 在指向 `obj` 之前，已经 `weak` 引用了其他的对象，则需要先将 `weak ptr` 从其他对象的 `weak_entry_t` 的 `hash` 数组中移除。在 `storeWeak` 方法中，会调用 `weak_unregister_no_lock` 来做移除操作。
```c++
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
 * @param weak_table The global weak table. // 弱引用表
 * @param referent The object. // 旧值
 * @param referrer The weak reference. // weak 变量的指针
 */
void
weak_unregister_no_lock(weak_table_t *weak_table, id referent_id, 
                        id *referrer_id)
{
    // 旧对象的指针
    objc_object *referent = (objc_object *)referent_id;
    
    // referrer_id 是指向 weak 指针的指针，
    // 所以这里是 **
    objc_object **referrer = (objc_object **)referrer_id;
    
    // weak_entry_t 自动变量
    weak_entry_t *entry;

    if (!referent) return;
    // referent 是 weak 变量之前指向的旧值
    // 在 weak_table 中去找到有 referent 的 entry
    //（相当于在 weak_table 表中去找到包含 referent 元素的数组）
    // 查找到 referent 所对应的 weak_entry_t
    if ((entry = weak_entry_for_referent(weak_table, referent))) {
        // 找到了这个 entry，就删除 entry 中的引用对象 - referrer
        // entry 的结构大概是是 key 是对象的地址，value 是存储 __weak 变量的数组
        // 这时从数组中把当前的 __weak 变量从数组中移除
        // 在 referent 所对应的 weak_entry_t 的 hash 数组中，移除 referrer
        remove_referrer(entry, referrer);
        bool empty = true;
        // 移除元素之后， 要检查一下 weak_entry_t 的 hash 数组是否已经空了
        // 这里是检查哈希表
        if (entry->out_of_line()  &&  entry->num_refs != 0) {
            empty = false;
        }
        else {
        // 这里是检查长度为 4 的内部小数组
            for (size_t i = 0; i < WEAK_INLINE_COUNT; i++) {
                if (entry->inline_referrers[i]) {
                    empty = false; 
                    break;
                }
            }
        }
        
        // 如果 entry 中的引用对象没有了，删除这个 entry
        // 即没有 __weak 变量指向这个对象了
        // 如果 weak_entry_t 的 hash 数组已经空了，则需要将 weak_entry_t 从 weak_table 中移除
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
> 2. 如果找到了 entry 就删除 entry 中的 referent - 引用对象（删除的是 referrer 吧好像？）
> 3. 判断 entry 里面还有没有其他对象，如果没有，就把 entry 也 remove 掉（相当于数组中元素为空，就把这个数据也删除）

`weak_register_no_lock`，将 `weak ptr` 地址 注册到 `obj` 对应的 `weak_entry_t` 中:
```c++
/** 
 * Registers a new (object, weak pointer) pair. Creates a new weak
 * object entry if it does not exist.
 * 注册一个新的 (对象，weak 指针) 对。
 * 创建一个新的 weak object entry，如果它不存在的话。
 *
 * @param weak_table The global weak table. // weak 表
 * @param referent The object pointed to by the weak reference. // weak 引用指向的对象
 * @param referrer The weak pointer address. // weak 指针地址
 */
id 
weak_register_no_lock(weak_table_t *weak_table, id referent_id, 
                      id *referrer_id, bool crashIfDeallocating)
{
    objc_object *referent = (objc_object *)referent_id;
    objc_object **referrer = (objc_object **)referrer_id;
    
    // 如果 referent为nil 或 referent 采用了 TaggedPointer 计数方式，直接返回，不做任何操作
    if (!referent  ||  referent->isTaggedPointer()) return referent_id;

    // ensure that the referenced object is viable
    // 确保引用对象是可见的
    
    // 确保被引用的对象可用（没有在析构，同时应该支持 weak 引用）
    // 1. 判断对象是否正在释放或者已经被释放了
    bool deallocating;
    if (!referent->ISA()->hasCustomRR()) {
        deallocating = referent->rootIsDeallocating();
    }
    else {
        // 2. 判断入参对象是否能进行 weak 引用
        // 如果对象是继承自 NSObject 则默认是 YES
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
    // 正在析构的对象，不能够被弱引用
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
    // 在 weak_table 中找到 referent 对应的 weak_entry,并将 referrer 加入到 weak_entry 中
    weak_entry_t *entry;
    // 在 weak_table 中去找到有 referent 的 entry
    //（相当于在 weak_table 表中去找到包含 referent 元素的数组）
    // 即自弱引用表里面包含 key 是新对象地址键
    // 如果能找到 weak_entry，则将 referrer 插入到 weak_entry 中
    if ((entry = weak_entry_for_referent(weak_table, referent))) {
        // 如果找到了，直接 append
        // 类似把 __weak 变量放进数组
        // 这里其实很复杂，有点类似与 cache_t 的机制，初始化为 4,并根据 3/4 进行扩容
        // 将 referrer 插入到 weak_entry_t 的引用数组中
        append_referrer(entry, referrer);
    } 
    else {
        // 创建一个新的 weak_entry_t，并将 referrer 插入到 weak_entry_t 的引用数组中
        weak_entry_t new_entry(referent, referrer);
        // weak_table 的 weak_entry_t 数组是否需要动态增长，若需要，则会扩容一倍
        weak_grow_maybe(weak_table);
        // 将 weak_entry_t 插入到 weak_table 中
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

关于 `referrer` 是如何插入到 `weak_entry_t` 中的，其 `hash` 算法是怎么样的，利用函数 `append_referrer`:
```c++
/** 
 * Add the given referrer to set of weak pointers in this entry.
 * 添加给定的 referrer 到这个 entry 的 weak 数组或者链表中。
 *
 * Does not perform duplicate checking (b/c weak pointers are never added to a set twice). 
 * 不执行重复检查，weak 指针永远不会添加到 set 中两次。
 * @param entry The entry holding the set of weak pointers.
 * @param new_referrer The new weak pointer to be added.
 */
static void append_referrer(weak_entry_t *entry, objc_object **new_referrer)
{
    if (! entry->out_of_line()) {
        // Try to insert inline.
        // 如果 weak_entry 尚未使用动态数组，走这里
        for (size_t i = 0; i < WEAK_INLINE_COUNT; i++) {
            if (entry->inline_referrers[i] == nil) {
                entry->inline_referrers[i] = new_referrer;
                return;
            }
        }

        // Couldn't insert inline. Allocate out of line.
        
        // 如果 inline_referrers 的位置已经存满了，则要转型为 referrers，做动态数组
        weak_referrer_t *new_referrers = (weak_referrer_t *)
            calloc(WEAK_INLINE_COUNT, sizeof(weak_referrer_t));
            
        // This constructed table is invalid, but grow_refs_and_insert will fix it and rehash it.
        // 此构造的 table 无效，grow_refs_and_insert 将修复它并重新哈希。
        for (size_t i = 0; i < WEAK_INLINE_COUNT; i++) {
            new_referrers[i] = entry->inline_referrers[i];
        }
        entry->referrers = new_referrers;
        entry->num_refs = WEAK_INLINE_COUNT;
        
        // 看到 out_of_line_ness 置为 REFERRERS_OUT_OF_LINE，标记是使用 长度为 4 的内部数组，还是哈希表
        entry->out_of_line_ness = REFERRERS_OUT_OF_LINE;
        
        // 看到这里有一个减 1 的操作
        entry->mask = WEAK_INLINE_COUNT-1;
        entry->max_hash_displacement = 0;
    }
    
    // 对于动态数组的附加处理：
    // 断言： 此时一定使用的动态数组
    ASSERT(entry->out_of_line());

    //#define TABLE_SIZE(entry) (entry->mask ? entry->mask + 1 : 0) // mask 又加了 1
    // 如果动态数组中元素个数大于或等于数组位置总空间的 3/4，则扩展数组空间为当前长度的一倍
    if (entry->num_refs >= TABLE_SIZE(entry) * 3/4) {
        // 扩容，并插入
        return grow_refs_and_insert(entry, new_referrer);
    }
    
    // 如果不需要扩容，直接插入到 weak_entry 中
    // 注意，weak_entry 是一个哈希表，key：w_hash_pointer(new_referrer) value: new_referrer
    
    // 细心的人可能注意到了，这里 weak_entry_t 的 hash 算法和 weak_table_t 的 hash 算法是一样的，同时扩容/减容的算法也是一样的
    // '& (entry->mask)' 确保了 begin 的位置只能大于或等于数组的长度
    size_t begin = w_hash_pointer(new_referrer) & (entry->mask);
    size_t index = begin; // 初始的 hash index
    size_t hash_displacement = 0; // 用于记录 hash 冲突的次数，也就是 hash 再位移的次数
    while (entry->referrers[index] != nil) {
        hash_displacement++;
        // index + 1, 移到下一个位置，再试一次能否插入。
        //（这里要考虑到entry->mask取值，一定是：0x111, 0x1111, 0x11111, ... ，
        // 因为数组每次都是*2增长，即8， 16， 32，对应动态数组空间长度-1的mask，也就是前面的取值。）
        index = (index+1) & entry->mask;
        // index == begin 意味着数组绕了一圈都没有找到合适位置，这时候一定是出了什么问题。
        if (index == begin) bad_weak_table(entry);
    }
    // 记录最大的 hash 冲突次数, max_hash_displacement 意味着: 
    // 我们尝试至多 max_hash_displacement 次，肯定能够找到 object 对应的 hash 位置
    if (hash_displacement > entry->max_hash_displacement) {
        entry->max_hash_displacement = hash_displacement;
    }
    // 将ref存入hash数组，同时，更新元素个数num_refs
    weak_referrer_t &ref = entry->referrers[index];
    ref = new_referrer;
    entry->num_refs++;
}
```

对象 dealloc 时针对 `weak ptr` 所做的操作：
当对象引用计数为 0 的时候会执行 `dealloc` 函数，可以在 dealloc 中去看看具体的销毁过程：
`dealloc -> rootDealloc -> object_dispose -> objc_destructInstance -> clearDeallocating -> clearDeallocating_slow`

1. `dealloc`函数：
```c++
// Replaced by NSZombies
- (void)dealloc {
    _objc_rootDealloc(self);
}
```
2. `_objc_rootDealloc` 函数：
```c++
void
_objc_rootDealloc(id obj)
{
    ASSERT(obj); // 如果 obj 不存在，则 crash
    obj->rootDealloc(); // 调用 objc_object 的 rootDealloc 函数
}
```
3.  `struct objc_object` 的 `rootDealloc` 函数：
```c++
inline void
objc_object::rootDealloc()
{
    if (isTaggedPointer()) return;  // fixme necessary? 有必要吗？这里是指 Tagged Pointer 计数的对象是不会析构的。

    // 这一步判断比较多，且符合条件是可直接调用 free 函数
    // 1. isa 是非指针类型，即优化的 ias_t 类型，除了类对象地址包含更多的信息
    // 2. 没有弱引用对象
    // 3. 没有关联对象
    // 4. 没有 C++ 析构函数 (在昨天写动态添加属性和成员变量的时候发现了方法列表里自己生成了 C++ 析构函数：name = ".cxx_destruct")，那么如果都有这个函数的话，是不是和 if 里面的 fastpath 冲突了？
    // 5. 引用计数没有超过 10
    // 满足以上条件后可以进行快速释放对象
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
> 1. 判断 `object` 是否采用了 `Tagged Pointer` 计数，如果是，则不进行任何析构操作。关于这一点，我们可以看出，用 `Tagged Pointer` 计数的对象，是不会析构的。 `Tagged Pointer` 计数的对象在内存中应该是类似于**字符串常量**的存在，**多个对象指针其实会指向同一块内存地址**。虽然官方文档中并没有提及，但可以推测，`Tagged Pointer` 计数的对象的内存位置很有可以就位于字符串常量区。
  2. 接下来判断对象是否采用了优化的`isa`计数方式（`isa.nonpointer`）。如果是，则判断是否能够进行快速释放（`free(this)` 用`C`函数释放内存）。可以进行快速释放的前提是：对象没有被`weak`引用`!isa.weakly_referenced`，没有关联对象`!isa.has_assoc`，没有自定义的`C++`析构方法`!isa.has_cxx_dtor`，没有用到`sideTable`来做引用计数 `!isa.has_sidetable_rc`。
  3. 其余的，则进入`object_dispose((id)this)`慢速释放分支。

4. 如果`obj`被`weak` 引用了则进入`object_dispose((id)this)`分支, 下面是 `object_dispose` 函数：
`object_dispose`方法中，会先调用`objc_destructInstance(obj)`来析构`obj`，再用 `free(obj)`来释放内存:
```c++
id 
object_dispose(id obj)
{
    if (!obj) return nil;

    objc_destructInstance(obj); // 可以理解为 free 前的清理工作    
    free(obj); // 这里才是 free 直接释放内存

    return nil;
}
```
5. `objc_destructInstance` 函数：
```c++
/***********************************************************************
* objc_destructInstance

* Destroys an instance without freeing memory. 
// 销毁实例而不释放内存，内存释放是上面的下面的 free 函数
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
        // 移除所有的关联对象，并将其自身从 Association Manager 的 map 中移除
        if (assoc) _object_remove_assocations(obj); // 移除关联对象
        
        // 如果该对象被 __weak 变量指向，则要把这些 __weak 变量指向 nil
        obj->clearDeallocating(); // 弱引用的释放在这里
    }

    return obj;
}
```
6. `clearDeallocating`中有两个分支，先判断`obj`是否采用了优化`isa`引用计数。没有，则要清理`obj`存储在`sideTable`中的引用计数等信息，这个分支在当前 64 位设备中应该不会进入，不必关心。如果启用了`isa`优化，则判断是否使用了`sideTable`，使用的原因是因为做了`weak`引用（`isa.weakly_referenced` ） 或 使用了`sideTable`的辅助引用计数（`isa.has_sidetable_rc`）。符合这两种情况之一，则进入慢析构路径。 `clearDeallocating` 函数：
```c++
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
```c++
// Slow path of clearDeallocating() 
// for objects with nonpointer isa
// that were ever weakly referenced 
// or whose retain count ever overflowed to the side table.
NEVER_INLINE void
objc_object::clearDeallocating_slow()
{
    ASSERT(isa.nonpointer  &&  (isa.weakly_referenced || isa.has_sidetable_rc));

    SideTable& table = SideTables()[this]; // 在全局的SideTables中，以 this 指针为 key，找到对应的 SideTable
    
    // 加锁
    table.lock(); 
    
    if (isa.weakly_referenced) { // 如果 obj 被弱引用
        // 在 SideTable 的 weak_table 中对 this 进行清理工作
        weak_clear_no_lock(&table.weak_table, (id)this);
    }
    
    if (isa.has_sidetable_rc) { // 如果采用了 SideTable 做引用计数
        // 在 SideTable 的引用计数中移除 this
        table.refcnts.erase(this);
    }
    
    // 开锁
    table.unlock();
}

#endif
```
8. 这里调用了 `weak_clear_no_lock` 来做 `weak_table` 的清理工作，同时将所有 `weak` 引用该对象的 `ptr` 置为`nil`.
  `weak_clear_no_lock`:
  ```c++
  /** 
   * Called by dealloc; nils out all weak pointers that point to the provided object so that they can no longer be used.
   * 由 dealloc 所调用；
   * 提供的对象的所有弱指针指向 nil，以便它们不能再使用
   * @param weak_table 
   * @param referent The object being deallocated. // 要释放的对象
   */
  void 
  weak_clear_no_lock(weak_table_t *weak_table, id referent_id) 
  {
      objc_object *referent = (objc_object *)referent_id;

      // 找到 referent 在 weak_table 中对应的 weak_entry_t
      weak_entry_t *entry = weak_entry_for_referent(weak_table, referent);
      if (entry == nil) {
          /// XXX shouldn't happen, but does with mismatched CF/objc
          //printf("XXX no entry for clear deallocating %p\n", referent);
          return;
      }

      // zero out references
      weak_referrer_t *referrers;
      size_t count;
      
      // 找出 weak 引用 referent 的 weak 指针地址数组以及数组长度
      if (entry->out_of_line()) {
          // 哈希表头部
          referrers = entry->referrers;
          // 长度是 mask + 1
          count = TABLE_SIZE(entry);
      } 
      else {
          // 内部的小数组的头部
          referrers = entry->inline_referrers;
          // 长度固定为 4
          count = WEAK_INLINE_COUNT;
      }
      
      for (size_t i = 0; i < count; ++i) {
          // 取出每个weak ptr的地址
          // 这里用了两个 **
          // 先取出指针
          // 再取出 weak 变量
          objc_object **referrer = referrers[i];
          if (referrer) {
              // 如果 weak ptr 确实 weak 引用了 referent，则将 weak ptr 设置为 nil，这也就是为什么 weak 指针会自动设置为 nil 的原因
              if (*referrer == referent) {
                  *referrer = nil;
              }
              else if (*referrer) {
              // 如果所存储的 weak ptr 没有 weak 引用 referent，这可能是由于 runtime 代码的逻辑错误引起的，报错
                  _objc_inform("__weak variable at %p holds %p instead of %p. "
                               "This is probably incorrect use of "
                               "objc_storeWeak() and objc_loadWeak(). "
                               "Break on objc_weak_error to debug.\n", 
                               referrer, (void*)*referrer, (void*)referent);
                  objc_weak_error();
              }
          }
      }
      // 由于 referent 要被释放了，因此 referent 的 weak_entry_t 也要从 weak_table 移除 
      weak_entry_remove(weak_table, entry);
  }
  ```
总结：

纵观`weak`引用的底层实现，其实原理很简单。就是将所有弱引用`obj`的指针地址都保存在`obj`对应的`weak_entry_t`中。当`obj`要析构时，会遍历`weak_entry_t`中保存的弱引用指针地址，并将弱引用指针指向`nil`，同时，将`weak_entry_t` 从 `weak_table` 移除。

总之，释放的时候就是 **找到散列表中的 weak_table 表，找到 weak_table 中的 entry，将 entry 中的 引用对象 referrer（__weak 变量）置为 nil，最后 remove entry**。

补充：
开头示例使用的是：
```objective-c
NSObject *obj = [[NSObject alloc] init];
__weak id weakPtr = obj; // ⬅️ 在这一行打断点
```
底层会使用 `storeWeak`，最终调用:
```c++
storeWeak<DontHaveOld, DoHaveNew, DoCrashIfDeallocating>
(location, (objc_object*)newObj);
```
这是，传入`storeWeak`的参数中，`haveOld`被设置为`false`，表明`weakObj`之前并没有`weak`指向其他的对象。
那么，什么时候`storeWeak`的参数`haveOld`被设置为`true`呢？当我们的`weakObj`已经指向一个`weak`对象，又要指向新的`weak`对象时，`storeWeak`的`haveOld`参数会被置为`true`：
```c++
NSObject *obj = [[NSObject alloc] init];
__weak NSObject *weakObj = obj; // 这里会调用 objc_initWeak 方法，storeWeak 的 haveOld == false
NSObject *obj2 = [[NSObject alloc] init];

// 这里会调用 objc_storeWeak 方法，
// storeWeak 的 haveOld == true，会将之前的引用先移除
weakObj = obj2;
```

**参考链接:🔗**
[Objective-C runtime机制(6)——weak引用的底层实现原理](https://blog.csdn.net/u013378438/article/details/82767947)
[iOS底层-- weak修饰对象存储原理](https://www.jianshu.com/p/bd4cc82e09c5)
[RunTime中SideTables, SideTable, weak_table, weak_entry_t](https://www.jianshu.com/p/48a9a9ec8779)
