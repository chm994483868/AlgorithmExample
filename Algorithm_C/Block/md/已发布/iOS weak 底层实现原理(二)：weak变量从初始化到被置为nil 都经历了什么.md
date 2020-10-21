# iOS weak 底层实现原理(二)：weak变量从初始化到被置为nil 都经历了什么

> &emsp;所有 `weak` 相关函数调用我们都通过在 `main` 函数里面写代码打断点，并打开汇编模式：`debug->debug workflow->alway show disassembly` 的方式来验证。

## 寻找 weak 变量初始化入口
&emsp;在 `main.m` 中编写如下代码:
```c++
#import <Foundation/Foundation.h>
int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // insert code here...
        id obj = [NSObject new];
        id obj2 = [NSObject new];
        printf("Start tag\n");
        {
            __weak id weakPtr = obj; // 调用 objc_initWeak 进行 weak 变量初始化
            weakPtr = obj2; // 修改 weak 变量指向
        } 
        // 出了这个右边花括号调用 objc_destroyWeak 函数进行 weak 变量销毁
        //（注意这里是 weak 变量的销毁，并不是 weak 变量指向的对象销毁）
        
        printf("End tag\n"); // ⬅️ 断点打在这里
    }
    return 0;
}
```
&emsp;`command + R` 运行后会进入断点，这里我们只关注`Start tag` 和 `End tag` 中间的部分，能捕捉到下面的内容：
```c++
...
0x100000c97 <+87>:  callq  0x100000e06               ; symbol stub for: objc_initWeak ⬅️ weak 变量初始化
0x100000c9c <+92>:  movq   -0x20(%rbp), %rsi
0x100000ca0 <+96>:  leaq   -0x18(%rbp), %rdi
0x100000ca4 <+100>: callq  0x100000e1e               ; symbol stub for: objc_storeWeak ⬅️ 修改 weak 变量的指向
0x100000ca9 <+105>: leaq   -0x18(%rbp), %rdi
0x100000cad <+109>: callq  0x100000e00               ; symbol stub for: objc_destroyWeak ⬅️ weak 变量销毁
...
```
&emsp;`callq` 指令表示函数调用，看到与 `weak` 变量相关函数是: `objc_initWeak`、`objc_storeWeak`、`objc_destroyWeak`，它们分别表示初始化 `weak` 变量、`weak` 变量赋值（修改指向）、销毁 `weak` 变量。

&emsp;下面首先分析下 `weak` 变量初始化函数，在 `objc4-781` 中全局搜索 `objc_initWeak`，在 `objc-internal.h` 文件中，看到 `objc_initWeak` 函数声明。
```c++
OBJC_EXPORT id _Nullable 
objc_initWeak(id _Nullable * _Nonnull location, id _Nullable val)
    OBJC_AVAILABLE(10.7, 5.0, 9.0, 1.0, 2.0);
```
&emsp;看到是 `iOS 5.0` 后出现的，这里联想到 `ARC`、`weak` 关键字等都是 `iOS 5.0` 后推出的。在 `NSObject.mm` 文件中找到了 `objc_initWeak` 函数实现。

## objc_initWeak
> &emsp;Initialize a fresh weak pointer to some object location. It would be used for code like: 
  ```c++
  (The nil case) 
  __weak id weakPtr; 
  (The non-nil case) 
  NSObject *o = ...; 
  __weak id weakPtr = o; 
  ```
  This function IS NOT thread-safe with respect to concurrent modifications to the weak variable. (Concurrent weak clear is safe.)
>
> &emsp;初始化指向某个对象位置的新的 weak pointer（旧的 weak pointer 赋值时要执行清理工作）。对于 weak 变量的并发修改，此函数不是线程安全的。（并发进行 weak clear 是线程安全的。）

```c++

// Template parameters. 模版参数
// 牢记这几个枚举值，
// HaveOld 如果是 true，表示 __weak 变量目前有指向一个对象，否则没有，可表示是新创建的 __weak 变量
// HaveNew 如果是 true，表示给 __weak 变量赋值的右侧对象是有值的，否则 __weak 变量会被指向 nil
enum HaveOld { DontHaveOld = false, DoHaveOld = true }; // 是否有旧值
enum HaveNew { DontHaveNew = false, DoHaveNew = true }; // 是否有新值

/**
 * @param location Address of __weak ptr. // __weak 变量的地址 (objc_object **) (ptr 是 pointer 的缩写，id 是 struct objc_object *)
 * @param newObj Object ptr. // 对象实例指针
 */
id
objc_initWeak(id *location, id newObj)
{
    // 如果对象不存在
    if (!newObj) {
        // 看到这个赋值用的是 *location = nil，表示把 __weak 变量指向 nil
        *location = nil; 
        return nil;
    }
    
    // storeWeak 是一个模版函数，DontHaveOld 表示没有旧值，表示这里是新初始化 __weak 变量。
    // DoHaveNew 表示有新值，新值即为 newObj
    // DoCrashIfDeallocating 表示如果在下面的函数执行过程中 newObj 中途释放了就 crash

    return storeWeak<DontHaveOld, DoHaveNew, DoCrashIfDeallocating>
        (location, (objc_object*)newObj);
}
```
&emsp;`objc_initWeak` 函数接收两个参数:
1. `id *location`：`__weak` 变量的地址，即示例代码中 `weak` 变量取地址: `&weakPtr`，它是一个指针的指针，之所以要存储指针的地址，是因为 `weak` 变量指向的对象释放后，要把 `weak` 变量指向置为 nil，如果仅存储指针（即 `weak` 变量所指向的地址值）的话，是不能够完成这个设置的。

  > 这里联想到了对链表做一些操作时，函数入参会是链表头指针的指针。
    这里如果对指针不是特别熟悉的话，可能会有一些迷糊，为什么用指针的指针，我们直接在函数内修改参数的指向时，不是同样也修改了外部指针的指向吗？其实非然！
    一定要理清，当函数形参是指针时，实参传入的是一个地址，然后在函数内部创建一个临时指针变量这个临时指针变量指向的地址是实参传入的地址，此时如果你修改指向的话，修改的只是函数内部的临时指针变量的指向。外部的指针变量是与它无关的，有关的只是初始时它们两个指向的地址是一样的。而我们对这个地址里面内容的所有操作，都是可反应到指向该地址的指针变量那里的。这个地址是指针指向的地址，如果没有 `const` 限制，我们可以对该地址里面的内容做任何操作即使把内容置空放0，这些操作都是对这个地址的内存做的，不管怎样这块内存都是存在的，它地址一直都在这里，而我们的原始指针一直就是指向它，此时我们需要的是修改原始指针的指向，那我们只有知道指针自身的地址才行，我们把指针自身的地址的内存空间里面放 `0x0`, 才能表示把我们的指针指向置为了 `nil`！

2. `id newObj`: 所用的对象，即示例代码中的 `obj`。
该方法有一个返回值，返回的是 `storeWeak` 函数的返回值：
返回的其实还是 `obj`, 但是已经对 `obj` 的 `isa（isa_t）` 的 `weakly_referenced` 位设置为 `1`，标识该对象有弱引用存在，当该对象销毁时，要处理之前指向它的那些弱引用，`weak` 变量被置为 `nil` 的机制就是从这里实现的。 

&emsp;看 `objc_initWeak` 函数实现可知，它内部是调用 `storeWeak` 函数，且执行时的模版参数是 `DontHaveOld`（没有旧值），这里是指 `weakPtr` 之前没有指向任何对象，我们的 `weakPtr` 是刚刚初始化的，自然没有指向旧值。这里涉及到的是，当 `weak` 变量改变指向时，要把该 `weak` 变量地址从它之前指向的对象的 `weak_entry_t` 的哈希数组中移除。`DoHaveNew` 表示有新值。

&emsp;`storeWeak` 函数实现的核心功能:
+ 将 `weak` 变量的地址 `location` 存入 `obj` 对应的 `weak_entry_t` 的哈希数组（或定长为 `4` 的数组）中，用于在 `obj` 析构时，通过该哈希数组找到其所有的 `weak` 变量的地址，将 `weak` 变量指向的地址（`*location`）置为 `nil`。
+ 如果启用了 `isa` 优化，则将 `obj` 的 `isa_t` 的 `weakly_referenced` 位置为 1，置为 1 的作用是标识 `obj` 存在 `weak` 引用。当对象 `dealloc` 时，`runtime` 会根据 `weakly_referenced` 标志位来判断是否需要查找 `obj` 对应的 `weak_entry_t`，并将它的所有的弱引用置为 `nil`。

从 `storeWeak` 函数实现就要和我们前几篇的内容联系起来啦，想想还有些激动 😊。

## objc_storeWeak
&emsp;分析 `storeWeak` 函数源码实现：
> &emsp;Update a weak variable. If HaveOld is true, the variable has an existing value that needs to be cleaned up. This value might be nil. If HaveNew is true, there is a new value that needs to be assigned into the variable. This value might be nil. If CrashIfDeallocating is true, the process is halted if newObj is deallocating or newObj's class does not support weak references. If CrashIfDeallocating is false, nil is stored instead.
>
> &emsp;更新一个 weak 变量。如果 HaveOld 为 true，则该 weak 变量具有需要清除的现有值。该值可能为 nil。如果 HaveNew 为 true，则需要将一个新值分配给 weak 变量。该值可能为 nil。如果 CrashIfDeallocating 为 true，则在 storeWeak 函数执行过程中  newObj 释放了或 newObj 的类不支持弱引用时，程序将 crash。如果 CrashIfDeallocating 为 false，则发生以上问题时只是在 weak 变量中存入 nil。

```c++
// DoCrashIfDeallocating：如果在 storeWeak 函数执行过程中 newObj 被释放了，那函数执行会 crash，
// DontCrashIfDeallocating：不 crash，并把 *location = nil
enum CrashIfDeallocating {
    DontCrashIfDeallocating = false, DoCrashIfDeallocating = true
};

// Template parameters.
enum HaveOld { DontHaveOld = false, DoHaveOld = true }; // 是否有旧值
enum HaveNew { DontHaveNew = false, DoHaveNew = true }; // 是否有新值

// ASSERT(haveOld || haveNew) 断言的宏定义，当括号里的条件不满足时则执行断言，
// 即括号里面为假时则执行断言，如果为真函数就接着往下执行，
// 类似 Swift 的 guard 语句，为真时执行接下来的函数，为假时执行 return。

template <HaveOld haveOld, HaveNew haveNew,
          CrashIfDeallocating crashIfDeallocating>
static id
storeWeak(id *location, objc_object *newObj)
{
    // 如果 haveOld 为假且 haveNew 为假，表示既没有新值也没有旧值，则执行断言
    ASSERT(haveOld || haveNew);
    
    // 这里是表示，如果你开始就标识没有新值且你的 newObj == nil 确实没有新值，则能正常执行函数，否则直接断言 crash。
    if (!haveNew) ASSERT(newObj == nil);

    // 指向 objc_class 的指针，指向 newObj 的 Class，标记 newObj 的 Class 已经完成初始化
    Class previouslyInitializedClass = nil;
    
    // __weak 变量之前指向的旧对象
    id oldObj;
    
    // 这里一直很好奇对象是在什么时候放进 SideTable 里面的？
    
    // 旧值对象所处的 SideTable
    SideTable *oldTable;
    
    // 新值对象所处的 SideTable 
    SideTable *newTable;

    // Acquire locks for old and new values.
    // Order by lock address to prevent lock ordering problems.
    // Retry if the old value changes underneath us. 
    
    // 取得旧值和新值所处的 SideTable 里面的 spinlock_t。（SideTable->slock）
    // 根据上面两个锁的锁地址进行排序，以防止出现加锁时出现锁排序问题。
    // 重试，如果旧值在下面函数执行过程中发生了改变。
    // 这里用到 C 语言的 goto 语句，goto 语句可以直接跳到指定的位置执行（直接修改函数执行顺序）
 retry:
    if (haveOld) { 
        // 如果有旧值，这个旧值表示是传进来的 weak 变量目前指向的对象，解引用把（*location）赋给 oldObj
        oldObj = *location;
        
        // 如果 weak ptr 目前有指向其它的对象，则取出这个对象的 SideTable 并赋值给 oldTable
        oldTable = &SideTables()[oldObj];
    } else {
        // 如果 weak prt 目前没有指向其它对象，则 oldTable = nil
        oldTable = nil;
    }
    
    if (haveNew) {
        // newObj 所处的 SideTable
        newTable = &SideTables()[newObj];
    } else {
        // newObj 为 nil，则 newTable 也赋值为 nil
        newTable = nil;
    }

    // 这里是根据 haveOld 和 haveNew 两个值，判断是否对 oldTable 和 newTable 这两个 SideTable 加锁
    
    // 加锁操作，防止多线程中竞争冲突
    SideTable::lockTwo<haveOld, haveNew>(oldTable, newTable);

    // 此处 *location 应该与 oldObj 保持一致，如果不同，说明在加锁之前 *location 已被其他线程修改
    if (haveOld  &&  *location != oldObj) {
        // 解锁，跳转到 retry 处再重新执行函数
        SideTable::unlockTwo<haveOld, haveNew>(oldTable, newTable);
        goto retry;
    }

    // Prevent a deadlock between the weak reference machinery
    // and the +initialize machinery by ensuring that 
    // no weakly-referenced object has an un-+initialized isa.
    
    // 确保没有弱引用的对象具有未初始化的isa。
    // 防止 weak reference machinery 和 +initialize machinery 之间出现死锁。
    
    // 有新值 haveNew 并且 newObj 不为 nil，判断 newObj 所属的类有没有初始化，如果没有初始化就进行初始化，
    if (haveNew  &&  newObj) {
        // newObj 所属的类
        Class cls = newObj->getIsa();
        
        // previouslyInitializedClass 记录正在进行初始化的类防止重复进入
        if (cls != previouslyInitializedClass  &&
            !((objc_class *)cls)->isInitialized())
        { 
            // 如果 cls 还没有初始化，先初始化，再尝试设置 weak，
            
            SideTable::unlockTwo<haveOld, haveNew>(oldTable, newTable); // 解锁
            
            // 调用对象所在类的(不是元类)初始化方法，即调用的是 [newObjClass initialize] 类方法
            class_initialize(cls, (id)newObj);

            // If this class is finished with +initialize then we're good.
            // 如果这个 class，完成了 +initialize 初始化，这对我们而言是一个好结果。
            
            // If this class is still running +initialize on this thread
            // (i.e. +initialize called storeWeak on an instance of itself)
            // then we may proceed but it will appear initializing and
            // not yet initialized to the check above.
            
            // 如果这个类在这个线程中完成了 +initialize 的任务，那么这很好。
            // 如果这个类还在这个线程中继续执行着 +initialize 任务，
            // (比如，这个类的实例在调用 storeWeak 方法，而 storeWeak 方法调用了 +initialize .)
            // 这样我们可以继续运行，但在上面它将进行初始化和尚未初始化的检查。
            // 相反，在重试时设置 previouslyInitializedClass 为 newObj 的 Class 来识别它。
            // Instead set previouslyInitializedClass to recognize it on retry.
            // 这里记录一下 previouslyInitializedClass，防止该 if 分支再次进入
            previouslyInitializedClass = cls;
            
            goto retry;
        }
    }

    // Clean up old value, if any.
    // 清理旧值，如果有旧值，则进行 weak_unregister_no_lock 操作。
    
    if (haveOld) {
        // 把 location 从 oldObj 的 weak_entry_t 的 hash 数组中移除
        weak_unregister_no_lock(&oldTable->weak_table, oldObj, location);
    }

    // Assign new value, if any.
    // 如果有新值，则进行 weak_register_no_lock 操作。
    
    if (haveNew) { 
        // 调用 weak_register_no_lock 方法把 weak ptr 的地址记录到 newObj 的 weak_entry_t 的哈希数组中
        newObj = (objc_object *)
            weak_register_no_lock(&newTable->weak_table, (id)newObj, location,
                                  crashIfDeallocating);
                                  
        // weak_register_no_lock returns nil if weak store should be rejected
        // Set is-weakly-referenced bit in refcount table.
        
        // 设置一个对象有弱引用分为两种情况：
        // 1): 当对象的 isa 是优化的 isa 时，更新 newObj 的 isa 的 weakly_referenced bit 标识位。
        // 2): 另外如果对象的 isa 是原始的 class 指针时，它的引用计数和弱引用标识位等信息都是在 refcount 中的引用计数值内。（不同的位表示不同的信息）
        //     需要从 refcount 中找到对象的引用计数值(类型是 size_t)，该引用计数值的第一位标识该对象有弱引用（SIDE_TABLE_WEAKLY_REFERENCED）。

        if (newObj  &&  !newObj->isTaggedPointer()) {
            // 终于找到了，设置 struct objc_objcet 的 isa（isa_t）中的 uintptr_t weakly_referenced : 1;
            //（isa 是原始指针时，设置 isa 最后一位是 1）
            newObj->setWeaklyReferenced_nolock();
        }

        // Do not set *location anywhere else. That would introduce a race.
        // 请勿在其他地方设置 *location，可能会引起竟态。
        // *location 赋值，weak ptr 直接指向 newObj，可以看到，这里并没有将 newObj 的引用计数 +1
        
        *location = (id)newObj;
    }
    else {
        // No new value. The storage is not changed.
        // 没有新值，则不发生改变
    }
    
    // 解锁，其他线程可以访问 oldTable, newTable 了
    SideTable::unlockTwo<haveOld, haveNew>(oldTable, newTable);
    
    // 返回 newObj，此时的 newObj 与刚传入时相比，weakly_referenced bit 位为 1
    return (id)newObj;
}
```

`storeWeak` 函数流程图:
![storeWeak 流程图](https://p9-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/07f3cce0e53f43f591e5869d73ab5868~tplv-k3u1fbpfcp-zoom-1.image)

&emsp;`storeWeak` 函数实质上接受5个参数，其中 `HaveOld haveOld, HaveNew haveNew, CrashIfDeallocating crashIfDeallocating` 这三个参数是以模板枚举的方式传入的，其实这是三个 `bool` 参数，具体到 `objc_initWeak` 函数，这三个参数的值分别为 `false，true，true`，因为是初始化 `weak` 变量必然要有新值，没有旧值。

那 `storeWeak` 大概分析到这里，下面我们来看另外一个函数。

## objc_destroyWeak
&emsp;示例代码中作为局部变量的 `__weak` 变量出了右边花括号它的作用域就结束了，必然会进行释放销毁，汇编代码中我们看到了 `objc_destroyWeak` 函数被调用，看名字它应该是 `__weak` 变量销毁时所调用的函数。如果 `__weak` 变量比它所指向的对象更早销毁，那么它所指向的对象的 `weak_entry_t` 的哈希数组中存放该 `__weak` 变量的地址要怎么处理呢？那么一探 `objc_destroyWeak` 函数的究竟应该你能找到答案。

> &emsp;Destroys the relationship between a weak pointer and the object it is referencing in the internal weak table. If the weak pointer is not referencing anything, there is no need to edit the weak table. This function IS NOT thread-safe with respect to concurrent modifications to the weak variable. (Concurrent weak clear is safe.)
>
> &emsp;销毁 weak pointer 和其所指向的对象的弱引用表中的关系。（对象的 weak_entry_t 的哈希数组中保存着该对象的所有弱引用的地址，这里意思是把指定的弱引用的地址从 weak_entr_t 的哈希数组中移除。）如果 weak pointer 未指向任何内容，则无需编辑 weak_entry_t 的哈希数组。对于弱引用的并发修改，此函数不是线程安全的。 （并发进行 weak clear 是线程安全的）


```c++
/** 
 * @param location The weak pointer address. // location 是 __weak 变量的地址（objc_object **）
 */
void
objc_destroyWeak(id *location)
{
    // 看到内部是直接调用了 storeWeak 函数，参数的话这里我们要细看一下，
    // DoHaveOld 
    // DontHaveNew
    // DontCrashIfDeallocating
    // location
    // nil 
    (void)storeWeak<DoHaveOld, DontHaveNew, DontCrashIfDeallocating>
        (location, nil);
}
```
&emsp;我们看到函数内部只有一行对 `storeWeak` 函数的调用，且模版参数直接表明 `DoHaveOld` 有旧值、 `DontHaveNew` 没有新值、`DontCrashIfDeallocating` 不需要 crash，`newObj` 为 `nil`，参数只有 `location` 即要销毁的弱引用的指针，回忆我们上面详细分析的 `storeWeak` 函数：
```c++
...
// Clean up old value, if any.
// 如果有旧值，则进行 weak_unregister_no_lock 操作
if (haveOld) {
    // 把 location 从 oldObj 对应的 weak_entry_t 的 hash 数组中移除
    weak_unregister_no_lock(&oldTable->weak_table, oldObj, location);
}
...
```
正验证了我们上面的盲猜，直接调用 `weak_unregister_no_lock` 函数，这也是 `objc_destroyWeak` 函数唯一的功能。

### `objc_initWeakOrNil`
```c++
id
objc_initWeakOrNil(id *location, id newObj)
{
    if (!newObj) {
        *location = nil;
        return nil;
    }

    return storeWeak<DontHaveOld, DoHaveNew, DontCrashIfDeallocating>
        (location, (objc_object*)newObj);
}
```
与 `objc_initWeak` 区别就是 `DontCrashIfDeallocating`，如果 `newObj` 析构不会 `crash`。

### `objc_storeWeakOrNil`
```c++
id
objc_storeWeakOrNil(id *location, id newObj)
{
    return storeWeak<DoHaveOld, DoHaveNew, DontCrashIfDeallocating>
        (location, (objc_object *)newObj);
}
```
与 `objc_storeWeak` 区别也只是 `DontCrashIfDeallocating`，如果 `newObj` 析构不会 `crash`。

## `weak` 变量被置为 `nil`
&emsp;当对象引用计数为 0 的时候会执行 `dealloc` 函数，我们可以在 `dealloc` 中去看具体的销毁过程：
`dealloc -> _objc_rootDealloc -> rootDealloc -> object_dispose -> objc_destructInstance -> clearDeallocating -> clearDeallocating_slow`。

### `dealloc`
`dealloc` 函数：
```c++
// Replaced by NSZombies
- (void)dealloc {
    _objc_rootDealloc(self);
}
```

### `_objc_rootDealloc`
`_objc_rootDealloc` 函数：
```c++
void
_objc_rootDealloc(id obj)
{
    ASSERT(obj); // 如果 obj 不存在，则 crash
    obj->rootDealloc(); // 调用 objc_object 的 rootDealloc 函数
}
```

###  `rootDealloc`
`struct objc_object` 的 `rootDealloc` 函数：
```c++
inline void
objc_object::rootDealloc()
{
    if (isTaggedPointer()) return;  
    // fixme necessary? 有必要吗？这里是指 Tagged Pointer 计数的对象是不会析构的。

    // 这一步判断比较多，且符合条件是可直接调用 free 函数
    // 1. isa 是非指针类型，即优化的 ias_t 类型，除了类对象地址包含更多的信息
    // 2. 没有弱引用对象
    // 3. 没有关联对象
    // 4. 没有 C++ 析构函数 
    // (在昨天写动态添加属性和成员变量的时候发现了方法列表里自己生成了 C++ 析构函数：name = ".cxx_destruct")，
    // 那么如果都有这个函数的话，是不是和 if 里面的 fastpath 冲突了？
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
`rootDealloc` 函数我们来详细分析下:
1. 判断 `object` 是否采用了 `Tagged Pointer` 计数，如果是，则不进行任何析构操作。关于这一点，我们可以看出，用 `Tagged Pointer` 计数的对象，是不会析构的。 `Tagged Pointer` 计数的对象在内存中应该是类似于**字符串常量**的存在，**多个对象指针其实会指向同一块内存地址**。虽然官方文档中并没有提及，但可以推测，`Tagged Pointer` 计数的对象的内存位置很有可以就位于字符串常量区。
2. 接下来判断对象是否采用了优化的`isa`计数方式（`isa.nonpointer`）。如果是，则判断是否能够进行快速释放（`free(this)` 用`C`函数释放内存）。可以进行快速释放的前提是：对象没有被 `weak` 引用 `!isa.weakly_referenced`，没有关联对象 `!isa.has_assoc`，没有自定义的 `C++` 析构方法 `!isa.has_cxx_dtor`，没有用到 `sideTable` 来做引用计数 `!isa.has_sidetable_rc`。
3. 其它情况，则进入 `object_dispose((id)this)` 慢速释放分支。

### `object_dispose` 
如果 `obj` 有被弱引用则进入 `object_dispose((id)this)` 分支, 下面是 `object_dispose` 函数：
  `object_dispose` 方法中，会先调用 `objc_destructInstance(obj)` 来析构 `obj`，再用 `free(obj)` 来释放内存空间:
```c++
id 
object_dispose(id obj) {
    if (!obj) return nil;
    // 可以理解为 free 前的清理工作    
    objc_destructInstance(obj);
    // 这里才是 free 直接释放内存
    free(obj);
    return nil;
}
```

### `objc_destructInstance`
`objc_destructInstance` 函数：
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

### `clearDeallocating`
`clearDeallocating`中有两个分支，先判断`obj`是否采用了优化`isa`引用计数。没有，则要清理`obj`存储在 `sideTable` 中的引用计数等信息，这个分支在当前 64 位设备中应该不会进入，不必关心。如果启用了`isa`优化，则判断是否使用了 `sideTable`，使用的原因是因为做了 `weak`引用（`isa.weakly_referenced` ） 或 使用了 `sideTable` 的辅助引用计数（`isa.has_sidetable_rc`）。符合这两种情况之一，则进入慢析构路径。 
`clearDeallocating` 函数：
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
### `clearDeallocating_slow`
`clearDeallocating_slow` 函数，从全局的 `SideTables` 中找到对象所处的 `SideTable`，然后调用 `weak_clear_no_lock` 函数。
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
### `weak_clear_no_lock`
这里调用了 `weak_clear_no_lock` 来做 `weak_table` 的清理工作，将所有该对象的弱引用置为`nil`。
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
          // 哈希数组起始地址
          referrers = entry->referrers;
          // 长度是 mask + 1
          count = TABLE_SIZE(entry);
      } 
      else {
          // 内部定长数组起始地址
          referrers = entry->inline_referrers;
          count = WEAK_INLINE_COUNT;
      }
      
      for (size_t i = 0; i < count; ++i) {
          // 取出每个weak ptr的地址
          // 这里用了两个 **
          // 先取出指针
          // 再取出 weak 变量
          objc_object **referrer = referrers[i];
          if (referrer) {
              // 如果 weak ptr 确实 weak 引用了 referent，则将 weak ptr 设置为 nil
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
  &emsp;将所有弱引用 `obj` 的指针地址都保存在 `obj` 对应的 `weak_entry_t` 中。当 `obj` 要析构时，遍历 `weak_entry_t` 中保存的弱引用指针地址，并将弱引用指针指向 `nil`，最后将 `weak_entry_t` 从 `weak_table` 移除。

## 参考链接
**参考链接:🔗**
+ [Objective-C runtime机制(6)——weak引用的底层实现原理](https://blog.csdn.net/u013378438/article/details/82767947)
+ [iOS底层-- weak修饰对象存储原理](https://www.jianshu.com/p/bd4cc82e09c5)
+ [RunTime中SideTables, SideTable, weak_table, weak_entry_t](https://www.jianshu.com/p/48a9a9ec8779)
