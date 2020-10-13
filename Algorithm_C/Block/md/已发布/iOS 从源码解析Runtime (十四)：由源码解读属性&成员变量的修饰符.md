# iOS 从源码解析Runtime (十四)：由源码解读属性&成员变量的修饰符

> 当我们分别使用 `atomic` 和 `nonatomic` 来修饰属性时，编译器是怎么处理这两种不同的情况的呢？大家都知道即使使用 `atomic` 修饰属性也并不能保证线程安全，那它和 `nonatomic` 有什么区别呢，那 `atomic` 的性能损耗来自哪里呢？`copy` 属性是怎么实现的？等等，关于属性修饰符的各种疑问我们本篇来统一来解读。⛽️⛽️

## `@property 修饰符`
&emsp;首先定义一个 `LGPerson` 类，添加一系列如下不同修饰符的属性，属性的本质编译器自动帮我们生成:  成员变量 + `setter` + `getter`。
```objective-c
// LGPerson.h，.m 文件什么也不写
#import <Foundation/Foundation.h>
NS_ASSUME_NONNULL_BEGIN
@interface LGPerson : NSObject

@property (nonatomic, strong) NSObject *objc_nonatomic_strong;
@property (nonatomic, retain) NSObject *objc_nonatomic_retain;
@property (nonatomic, copy) NSObject *objc_nonatomic_copy;
@property (nonatomic, weak) NSObject *objc_nonatomic_weak;
@property (nonatomic, unsafe_unretained) NSObject *objc_nonatomic_unsafe_unretained;
@property (nonatomic, assign) NSObject *objc_nonatomic_assign;

// readonly 修饰的属性，编译器仅自动生成 getter 函数
@property (nonatomic, strong, readonly) NSObject *objc_nonatomic_strong_readonly;

@property (atomic, strong) NSObject *objc_atomic_strong;
@property (atomic, retain) NSObject *objc_atomic_retain;
@property (atomic, copy) NSObject *objc_atomic_copy;
@property (atomic, weak) NSObject *objc_atomic_weak;
@property (atomic, unsafe_unretained) NSObject *objc_atomic_unsafe_unretained;
@property (atomic, assign) NSObject *objc_atomic_assign;

@end
NS_ASSUME_NONNULL_END
```
&emsp;选择真机运行模式，保证编译出的是 `ARM` 下的汇编指令，（`x86` 下的看不懂😭）然后在 `xcode` 左侧用鼠标选中 `LGPerson.m` 文件，通过 `xcode` 菜单栏 `Product -> Perform Action -> Assemble "LGPerson.m"` 生成汇编指令，可以看到我们的所有属性所对应的 `setter getter` 方法的汇编实现。

### `[LGPerson objc_nonatomic_strong]` 
```c++
    .p2align    2               ; -- Begin function -[LGPerson objc_nonatomic_strong]
"-[LGPerson objc_nonatomic_strong]":    ; @"\01-[LGPerson objc_nonatomic_strong]"
Lfunc_begin0:
    .loc    1 27 0                  ; Simple_iOS/LGPerson.h:27:0
    .cfi_startproc
; %bb.0:
    sub    sp, sp, #16             ; =16
    .cfi_def_cfa_offset 16
    str    x0, [sp, #8]
    str    x1, [sp]
Ltmp1:
    .loc    1 27 41 prologue_end    ; Simple_iOS/LGPerson.h:27:41
    ldr    x0, [sp, #8]
    ldr    x0, [x0, #8]
    add    sp, sp, #16             ; =16
    ret
Ltmp2:
Lfunc_end0:
    .cfi_endproc
                                        ; -- End function
```
&emsp;`objc_nonatomic_strong` 属性的 `getter` 函数内部没有调用任何函数，只是地址偏移取值。
### `[LGPerson setObjc_nonatomic_strong:]`
```c++
    .p2align    2               ; -- Begin function -[LGPerson setObjc_nonatomic_strong:]
"-[LGPerson setObjc_nonatomic_strong:]": ; @"\01-[LGPerson setObjc_nonatomic_strong:]"
Lfunc_begin1:
    .loc    1 27 0                  ; Simple_iOS/LGPerson.h:27:0
    .cfi_startproc
; %bb.0:
    sub    sp, sp, #48             ; =48
    stp    x29, x30, [sp, #32]     ; 16-byte Folded Spill
    add    x29, sp, #32            ; =32
    .cfi_def_cfa w29, 16
    .cfi_offset w30, -8
    .cfi_offset w29, -16
    stur    x0, [x29, #-8]
    str    x1, [sp, #16]
    str    x2, [sp, #8]
Ltmp3:
    .loc    1 0 0 prologue_end      ; Simple_iOS/LGPerson.h:0:0
    ldr    x0, [sp, #8]
    ldur    x1, [x29, #-8]
    add    x1, x1, #8              ; =8
    str    x0, [sp]                ; 8-byte Folded Spill
    mov    x0, x1
    ldr    x1, [sp]                ; 8-byte Folded Reload
    bl    _objc_storeStrong // ⬅️ 看到 bl 指令跳转到 objc_storeStrong 函数
    .loc    1 27 41                 ; Simple_iOS/LGPerson.h:27:41
    ldp    x29, x30, [sp, #32]     ; 16-byte Folded Reload
    add    sp, sp, #48             ; =48
    ret
Ltmp4:
Lfunc_end1:
    .cfi_endproc
                                        ; -- End function
```
&emsp;`objc_nonatomic_strong` 属性的 `setter` 函数内部看到 `bl` 指令跳转到 `objc_storeStrong` 函数。`objc_storeStrong` 函数我们之前解读 `retain、release` 的时候解析过，这里再解析一遍，它实现的事情就是 `retain` 新值，`release` 旧值。

`objc_storeStrong` 函数实现:
```c++
void
objc_storeStrong(id *location, id obj)
{
    // 1. 取出属性原本指向的旧值
    id prev = *location;
    
    // 2. 如果旧值和入参传入的新值相同，就没有赋值的必要了，直接 return 
    if (obj == prev) {
        return;
    }
    
    // 3. 先 retain 新值 obj，obj 的引用计数 +1
    objc_retain(obj);
    
    // 4. 把我的属性指向新值
    *location = obj;
    
    // 5. 释放旧值
    objc_release(prev);
}
```
### `[LGPerson objc_nonatomic_retain]/[LGPerson setObjc_nonatomic_retain:]`
&emsp;`objc_nonatomic_retain` 属性的 `setter` 和 `getter` 函数和 `objc_nonatomic_strong` 一致，这里不再分析。
### `[LGPerson objc_nonatomic_copy]` 
```c++
    .p2align    2               ; -- Begin function -[LGPerson objc_nonatomic_copy]
"-[LGPerson objc_nonatomic_copy]":      ; @"\01-[LGPerson objc_nonatomic_copy]"
Lfunc_begin4:
    .loc    1 29 0                  ; Simple_iOS/LGPerson.h:29:0
    .cfi_startproc
; %bb.0:
    sub    sp, sp, #32             ; =32
    .cfi_def_cfa_offset 32
    str    x0, [sp, #24]
    str    x1, [sp, #16]
Ltmp11:
    .loc    1 29 39 prologue_end    ; Simple_iOS/LGPerson.h:29:39
    ldr    x0, [sp, #16]
    ldr    x1, [sp, #24]
    orr    w8, wzr, #0x18
    mov    x2, x8
    
    mov    w8, #0 // 低字节存 0
    mov    x3, x8 // x3 存的值是 0，表示下面跳转到 objc_getProperty 函数时，第 4 个参数 BOOL atomic 是 0（false）
                  // x0 - x7 寄存器保存函数参数
                  
    str    x0, [sp, #8]            ; 8-byte Folded Spill
    mov    x0, x1
    ldr    x1, [sp, #8]            ; 8-byte Folded Reload
    add    sp, sp, #32             ; =32
    b    _objc_getProperty // ⬅️ 看到 b 指令跳转到 objc_getProperty 函数
Ltmp12:
Lfunc_end4:
    .cfi_endproc
                                        ; -- End function
```
&emsp;`objc_nonatomic_copy` 属性的 `getter` 函数内部看到最后 `b` 指令跳转到了 `objc_getProperty` 函数。下面我们来看一下 `objc_getProperty` 函数实现。
如果属性不是 `atomic` 修饰的话不需要对读取过程加锁，`objc_getProperty` 函数的前半部分就已经 `return` 成员变量了，看到成员变量的依然是通过 `self` 指针偏移找到并返回。
如果属性是 `atomic` 修饰的话，会通过 `PropertyLocks[slot]` 取得一把锁，而加锁的内容是 `id value = objc_retain(*slot)` 会对成员变量执行一次 `retain` 操作，引用计数 `+1`，然后为了性能，在解锁后才调用 `objc_autoreleaseReturnValue(value)` 把成员变量放进自动释放池，保证和刚刚的 `retain` 操作抵消，保证成员变量能正常释放销毁。 
```c++
// ptrdiff_t offset
// ptrdiff_t 是 C/C++ 标准库中定义的一个与机器相关的数据类型。
// ptrdiff_t 类型变量通常用来保存两个指针减法操作的结果。
// ptrdiff_t 类型则应保证足以存放同一数组中两个指针之间的差距,它有可能是负数。

// offset 是成员变量距离对象起始地址的偏移量。

id objc_getProperty(id self, SEL _cmd, ptrdiff_t offset, BOOL atomic) {
    // 如果 offset 为 0，则返回该对象的所属的类对象的地址
    if (offset == 0) {
        return object_getClass(self);
    }

    // Retain release world
    // self 指针偏移找到成员变量
    id *slot = (id*) ((char*)self + offset);
    // 如果 atomic 为 false 则直接返回成员变量
    if (!atomic) return *slot;
        
    // Atomic retain release world
    
    // 从全局的属性锁列表内取得锁
    spinlock_t& slotlock = PropertyLocks[slot];
    // 加锁
    slotlock.lock();
    // retain
    id value = objc_retain(*slot);
    // 解锁
    slotlock.unlock();
    
    // for performance, we (safely) issue the autorelease OUTSIDE of the spinlock.
    // 把 value 放进自动释放池，是引用计数与上面的 retain 操作保持平衡
    return objc_autoreleaseReturnValue(value);
}
```
### `[LGPerson setObjc_nonatomic_copy:]`
```c++
    .p2align    2               ; -- Begin function -[LGPerson setObjc_nonatomic_copy:]
"-[LGPerson setObjc_nonatomic_copy:]":  ; @"\01-[LGPerson setObjc_nonatomic_copy:]"
Lfunc_begin5:
    .loc    1 29 0                  ; Simple_iOS/LGPerson.h:29:0
    .cfi_startproc
; %bb.0:
    sub    sp, sp, #48             ; =48
    stp    x29, x30, [sp, #32]     ; 16-byte Folded Spill
    add    x29, sp, #32            ; =32
    .cfi_def_cfa w29, 16
    .cfi_offset w30, -8
    .cfi_offset w29, -16
    stur    x0, [x29, #-8]
    str    x1, [sp, #16]
    str    x2, [sp, #8]
Ltmp13:
    .loc    1 29 39 prologue_end    ; Simple_iOS/LGPerson.h:29:39
    ldr    x1, [sp, #16]
    ldur    x0, [x29, #-8]
    ldr    x2, [sp, #8]
    orr    x3, xzr, #0x18
    bl    _objc_setProperty_nonatomic_copy // ⬅️ 看到 bl 指令跳转到 objc_setProperty_nonatomic_copy 函数
    ldp    x29, x30, [sp, #32]     ; 16-byte Folded Reload
    add    sp, sp, #48             ; =48
    ret
Ltmp14:
Lfunc_end5:
    .cfi_endproc
                                        ; -- End function
```
&emsp;`objc_nonatomic_copy` 属性的 `setter` 函数内部看到 `bl` 指令跳转到了 `objc_setProperty_nonatomic_copy` 函数。下面我们来看一下 `objc_setProperty_nonatomic_copy` 函数实现。
```c++
void objc_setProperty_nonatomic_copy(id self, SEL _cmd, id newValue, ptrdiff_t offset)
{
    // 直接调用 reallySetProperty 函数
    reallySetProperty(self, _cmd, newValue, offset, false, true, false);
}
```
```c++
// atomic: false
// copy: true
// mutableCopy: false

static inline void reallySetProperty(id self, SEL _cmd, id newValue, ptrdiff_t offset, bool atomic, bool copy, bool mutableCopy)
{
    // 如果 offset 为 0，则调用 changeIsa 修改对象的 isa
    if (offset == 0) {
        object_setClass(self, newValue);
        return;
    }

    // 用于记录旧值的临时变量，主要是在最后对旧值进行 release 操作，
    // 释放旧值
    id oldValue;
    
    // 根据 offset 取得对象当前要进行 setter 的成员变量（旧值）
    id *slot = (id*) ((char*)self + offset);
    
    if (copy) {
        // 如果是 copy 的话，对 newValue 执行一次 copy 操作，
        // 这里直接把 copy 结果赋值给 newValue
        newValue = [newValue copyWithZone:nil];
    } else if (mutableCopy) {
        // 如果是 mutableCopy 的话，对 newValue 执行一次 mutableCopy 操作，
        // 这里直接把 mutableCopy 结果赋值给 newValue
        newValue = [newValue mutableCopyWithZone:nil];
    } else {
        // 如果旧值和新值相同的话，则直接 return
        if (*slot == newValue) return;
        
        // retain 新值
        newValue = objc_retain(newValue);
    }

    if (!atomic) {
        // 如果不是 atomic 的话，不需要加锁，
        // 把 *slot 赋值给 oldValue，会在函数末尾释放 oldValue
        oldValue = *slot;
        
        // 把新值赋值给对象的成员变量
        *slot = newValue;
    } else {
        // 如果是 atomic 的话，则从对赋值的过程进行加锁，
        // 看到 atomic 修饰的属性只是对新值旧值赋值的过程进行了加锁，
        // 和 nonatomic 不加锁相比，这个加锁的操作就是性能损耗的来源。
        // 看到这里我们也发现了，atomic 只是对 setter 和 getter 加锁，
        // 只能保证 setter 和 getter 是线程安全的，但是我们日常开发中几乎都是复合操作，
        // 如 self.a = self.a + 1;
        // 此操作包含getter、setter、加操作。
        // atomic 只是给单个 getter、setter 操作加锁了，
        // 无法保证这种复合操作的线程安全。如果要实现线程安全需要额外加锁。
        
        // 如下伪代码:
        // lock.lock();
        // self.a = self.a + 1;
        // lock.unlock();
        
        // 从全局的属性锁列表内取得锁
        spinlock_t& slotlock = PropertyLocks[slot];
        // 加锁
        slotlock.lock();
        
        // 把 *slot 赋值给 oldValue，会在函数末尾释放 oldValue
        oldValue = *slot;
        // 把新值赋值给对象的成员变量
        *slot = newValue;
        
        // 解锁
        slotlock.unlock();
    }

    // 释放旧值
    objc_release(oldValue);
}
```
### `[LGPerson objc_nonatomic_weak]`
```c++
    .p2align    2               ; -- Begin function -[LGPerson objc_nonatomic_weak]
"-[LGPerson objc_nonatomic_weak]":      ; @"\01-[LGPerson objc_nonatomic_weak]"
Lfunc_begin6:
    .loc    1 30 0                  ; Simple_iOS/LGPerson.h:30:0
    .cfi_startproc
; %bb.0:
    sub    sp, sp, #32             ; =32
    stp    x29, x30, [sp, #16]     ; 16-byte Folded Spill
    add    x29, sp, #16            ; =16
    .cfi_def_cfa w29, 16
    .cfi_offset w30, -8
    .cfi_offset w29, -16
    str    x0, [sp, #8]
    str    x1, [sp]
Ltmp15:
    .loc    1 30 39 prologue_end    ; Simple_iOS/LGPerson.h:30:39
    ldr    x0, [sp, #8]
    add    x0, x0, #32             ; =32
    bl    _objc_loadWeakRetained // ⬅️ 看到 bl 指令跳转到 objc_loadWeakRetained 函数
    ldp    x29, x30, [sp, #16]     ; 16-byte Folded Reload
    add    sp, sp, #32             ; =32
    b    _objc_autoreleaseReturnValue // ⬅️ 看到 bl 指令跳转到 objc_autoreleaseReturnValue 函数
Ltmp16:
Lfunc_end6:
    .cfi_endproc
                                        ; -- End function
```
&emsp;`objc_nonatomic_weak` 属性的 `getter` 函数内部看到 `bl` 指令跳转到 `objc_loadWeakRetained` 函数，在结尾处 `b` 指令跳转到 `objc_autoreleaseReturnValue`，这里不尽想到 `objc_loadWeak` 函数。
```c++
id
objc_loadWeak(id *location)
{
    if (!*location) return nil;
    return objc_autorelease(objc_loadWeakRetained(location));
}
```
在 `weak` 篇有详细分析过该函数，这里就不重复了。（`retain` 和 `autorelease` 配对使用，保证对读值过程中对象释放，同时自动释放池的延迟释放也能保证对象的正常销毁）
### `[LGPerson setObjc_nonatomic_weak:]`
```c++
...
bl    _objc_storeWeak
...
```
&emsp;`objc_nonatomic_weak` 属性的 `setter` 函数内部看到 `bl` 指令跳转到了 `objc_storeWeak` 函数，该函数特别长特别重要，在 `weak` 篇有非常详细的分析过，这里就不重复了。(`weak` 不会 `retain` 新值)
### `[LGPerson objc_nonatomic_unsafe_unretained]`
&emsp;`objc_nonatomic_unsafe_unretained` 属性的 `getter` 函数和 `objc_nonatomic_strong` 属性的 `getter` 函数 一样，内部没有调用任何函数，只是地址偏移取值。
### `[LGPerson setObjc_nonatomic_unsafe_unretained:]`
```c++
    .p2align    2               ; -- Begin function -[LGPerson objc_nonatomic_unsafe_unretained]
"-[LGPerson objc_nonatomic_unsafe_unretained]": ; @"\01-[LGPerson objc_nonatomic_unsafe_unretained]"
Lfunc_begin8:
    .loc    1 31 0                  ; Simple_iOS/LGPerson.h:31:0
    .cfi_startproc
; %bb.0:
    sub    sp, sp, #16             ; =16
    .cfi_def_cfa_offset 16
    str    x0, [sp, #8]
    str    x1, [sp]
Ltmp20:
    .loc    1 31 52 prologue_end    ; Simple_iOS/LGPerson.h:31:52
    ldr    x0, [sp, #8]
    ldr    x0, [x0, #40]
    add    sp, sp, #16             ; =16
    ret
Ltmp21:
Lfunc_end8:
    .cfi_endproc
                                        ; -- End function
```
&emsp;`objc_nonatomic_unsafe_unretained` 属性的 `setter` 函数看到内部没有调用任何其它函数，就是纯粹的入参、地址偏移、存储入参到成员变量的位置。这里也验证了 `unsafe_unretained` 的 `setter` 的本质，即不 `retain` 新值也不 `release` 旧值。`setter` 和 `getter` 函数都是简单的根据地址存入值和读取值。所以这里也引出另一个问题，赋值给 `unsafe_unretained` 属性的对象并不会被 `unsafe_unretained` 属性所持有，那么当此对象正常释放销毁以后，也并没有把 `unsafe_unretained` 属性置为 `nil`，此时我们如果再用 `unsafe_unretained` 属性根据地址读取对象，会直接引发野指针访问导致 `crash`。
### `[LGPerson objc_nonatomic_assign]/[LGPerson setObjc_nonatomic_assign:]`
&emsp;`objc_nonatomic_assign` 属性的 `setter` 和 `getter` 函数和 `objc_nonatomic_unsafe_unretained` 属性如出一辙，这里就不展开了。
### `[LGPerson objc_nonatomic_strong_readonly]`
&emsp;`objc_nonatomic_strong_readonly` 属性只生成了 `getter` 函数，也符合我们的预期。
### `[LGPerson objc_atomic_strong]/[LGPerson setObjc_atomic_strong:]`
```c++
// getter
...
...
// 零寄存器的值和 0x1 做或操作，并把结果存入 w3，表示 w3 = 1，同时表示下面调用 objc_getProperty 函数是第 4 个参数 BOOL atomic 是 true
// x0 - x7 寄存器保存函数参数
orr    w3, wzr, #0x1
...
b    _objc_getProperty
...

// setter
...
bl    _objc_setProperty_atomic
...
```
```c++
void objc_setProperty_atomic(id self, SEL _cmd, id newValue, ptrdiff_t offset)
{
    // atomic 值使用的是 true
    reallySetProperty(self, _cmd, newValue, offset, true, false, false);
}
```
&emsp;`objc_atomic_strong` 属性在 `setter` 和 `getter` 函数中都加了锁。
### `[LGPerson objc_atomic_retain]/[LGPerson setObjc_atomic_retain:]`
&emsp;`objc_atomic_retain` 属性 和 `objc_atomic_strong` 属性的 `setter` 和 `getter` 函数如出一辙，不再展开。
### `[LGPerson objc_atomic_copy]/[LGPerson setObjc_atomic_copy:]`
```c++
// getter
...
orr    w3, wzr, #0x1 // 第 4 个参数 BOOL atomic 是 true
...
b    _objc_getProperty
...

// setter
bl    _objc_setProperty_atomic_copy
```
```c++
void objc_setProperty_atomic_copy(id self, SEL _cmd, id newValue, ptrdiff_t offset)
{
    // atomic 值使用的是 true
    reallySetProperty(self, _cmd, newValue, offset, true, true, false);
}
```
`objc_atomic_weak`、`objc_atomic_unsafe_unretained`、`objc_atomic_assign` 和对应的 `nonatomic` 修饰的属性的 `setter` `getter` 函数相同，就不再展开了。
属性修饰符的内容看完了，那么我们常用的 `__strong`、`__weak`、`__unsafe_unretained` 等等修饰成员变量的修饰符系统又是如何处理的呢？下面我们来一探究竟。
## 成员变量修饰符
&emsp;定义如下类:
```c++
@interface LGPerson : NSObject {
    __strong NSObject *ivar_strong; // 无修饰符的对象默认会加 __strong
    __weak NSObject *ivar_weak;
    __unsafe_unretained NSObject *ivar_unsafe_unretained;
}
@end
```

## 参考链接
**参考链接:🔗**
+ [ObjC如何通过runtime修改Ivar的内存管理方式](https://www.cnblogs.com/dechaos/p/7246351.html) 
+ [Objective-C类成员变量深度剖析](http://quotation.github.io/objc/2015/05/21/objc-runtime-ivar-access.html)
+ [iOS基础系列-- atomic, nonatomic](https://xiaozhuanlan.com/topic/2354790168)
+ [低于0.01%的极致Crash率是怎么做到的？](https://wetest.qq.com/lab/view/393.html?from=content_csdnblog)
+ [[iOS]深入理解ivar及property](https://developer.aliyun.com/article/58989)
+ [Declared Properties](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/ObjCRuntimeGuide/Articles/ocrtPropertyIntrospection.html)
+ [iOS @property 属性相关的总结](https://juejin.im/post/6844903824436494343)
+ [atomic关键字的一些理解](https://www.jianshu.com/p/5951cb93bcef)
