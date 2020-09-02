# iOS weak 底层实现原理(终章)：objc_loadWeakRetained

> `ARC` 下我们想要获取 `weak` 变量指向的对象是通过：`objc_loadWeakRetained`，`MRC` 下通过: `objc_loadWeak`。这里要和通过指针直接找到对象内存读取内容的方式作出理解上的区别。
  通过分析函数实现，我们可以发现只要一个对象被标记为 `deallocating`，不管此时有没有执行到把该对象的弱引用置为 `nil`，只要通过该对象的弱引用获取对象都会得到 `nil`，即使此时该对象的弱引用还是指向对象内存且对象没有被完全释放。

## 示例代码
&emsp;在 `main.m` 中编写如下函数，并打开汇编模式：`debug -> debug workflow -> alway show disassembly`。下面我们来验证函数执行过程调用了哪些我们 “看不见” 的函数:
```c++
#import <Foundation/Foundation.h>
int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // insert code here...
        id obj = [NSObject new];
        
        printf("Start tag");
        {
            __weak id weakPtr = obj;
            NSLog(@"%@", weakPtr);
        }
        printf("End tag"); // ⬅️ 在这里打断点
    }
    return 0;
}
```
## 分析汇编代码
这里我们只专注 `Start tag` 和 `End tag` 中间的代码。运行代码我们可以捕捉到如下内容:
`ARC` 下:
```c++
    ...
    0x100000dc1 <+49>:  leaq   0x194(%rip), %rdi         ; "Start tag"
    0x100000dc8 <+56>:  movb   $0x0, %al
    0x100000dca <+58>:  callq  0x100000ece               ; symbol stub for: printf
    0x100000dcf <+63>:  movq   -0x18(%rbp), %rsi
    0x100000dd3 <+67>:  leaq   -0x20(%rbp), %rbx
    0x100000dd7 <+71>:  movq   %rbx, %rdi
    
    // 🍎 这里匹配 main 中的 __weak id weakPtr = obj; 初始化我们的 weak 变量 weakPtr
    0x100000dda <+74>:  callq  0x100000eb6               ; symbol stub for: objc_initWeak // ⬅️
    
    // 🍎 main 中一句 NSLog(@"%@", weakPtr); 则是顺序调用了 objc_loadWeakRetained 和 objc_release 函数 
    0x100000ddf <+79>:  movq   %rbx, %rdi
    
    // 🍎
    0x100000de2 <+82>:  callq  0x100000ebc               ; symbol stub for: objc_loadWeakRetained // ⬅️
    
    0x100000de7 <+87>:  movq   %rax, %rbx
    0x100000dea <+90>:  leaq   0x227(%rip), %rdi         ; @"%@"
    0x100000df1 <+97>:  movq   %rbx, %rsi
    0x100000df4 <+100>: xorl   %eax, %eax
    0x100000df6 <+102>: callq  0x100000e98               ; symbol stub for: NSLog
    0x100000dfb <+107>: jmp    0x100000dfd               ; <+109> at main.m:17:13
    0x100000dfd <+109>: movq   %rbx, %rdi
    
    // 🍎
    0x100000e00 <+112>: callq  *0x202(%rip)              ; (void *)0x00007fff71cb7d20: objc_release // ⬅️
    
    0x100000e06 <+118>: leaq   -0x20(%rbp), %rdi
    
    // 🍎 这里是出了右边花括号，我们的 weak 变量 weakPtr 要执行销毁
    0x100000e0a <+122>: callq  0x100000eb0               ; symbol stub for: objc_destroyWeak // ⬅️
    
->  0x100000e0f <+127>: leaq   0x150(%rip), %rdi         ; "End tag"
    0x100000e16 <+134>: movb   $0x0, %al
    0x100000e18 <+136>: callq  0x100000ece               ; symbol stub for: printf
    ...
```
`MRC` 下: (在 `Compile Sources` 中，把 `main.m` 文件的 `Compiler Flags` 设置为: `-fno-objc-arc` )
```c++
    0x100000df1 <+49>:  leaq   0x158(%rip), %rdi         ; "Start tag"
    0x100000df8 <+56>:  xorl   %eax, %eax
    0x100000dfa <+58>:  callq  0x100000ec8               ; symbol stub for: printf
    0x100000dff <+63>:  movq   -0x28(%rbp), %rsi
    0x100000e03 <+67>:  leaq   -0x18(%rbp), %rbx
    0x100000e07 <+71>:  movq   %rbx, %rdi
    
    // 🍎 这里匹配 main 中的 __weak id weakPtr = obj; 初始化我们的 weak 变量 weakPtr
    0x100000e0a <+74>:  callq  0x100000eb6               ; symbol stub for: objc_initWeak // ⬅️
    
    0x100000e0f <+79>:  movq   %rbx, %rdi
    
    // 🍎
    0x100000e12 <+82>:  callq  0x100000ebc               ; symbol stub for: objc_loadWeak // ⬅️
    
    0x100000e17 <+87>:  leaq   0x1f2(%rip), %rdi         ; @"%@"
    0x100000e1e <+94>:  movq   %rax, %rsi
    0x100000e21 <+97>:  xorl   %eax, %eax
    0x100000e23 <+99>:  callq  0x100000e98               ; symbol stub for: NSLog
    0x100000e28 <+104>: jmp    0x100000e2a               ; <+106> at main.m:18:9
    0x100000e2a <+106>: leaq   -0x18(%rbp), %rdi
    
    // 🍎 这里是出了右边花括号，我们的 weak 变量 weakPtr 要执行销毁
    0x100000e2e <+110>: callq  0x100000eb0               ; symbol stub for: objc_destroyWeak // ⬅️
    
->  0x100000e33 <+115>: leaq   0x120(%rip), %rdi         ; "End tag"
    0x100000e3a <+122>: movb   $0x0, %al
    0x100000e3c <+124>: callq  0x100000ec8               ; symbol stub for: printf
```
## 获取被弱引用的对象
通过上面的汇编代码我们可以做出如下总结并先抛出一些结论，后面我们会一步一步的进行证明:
1. 在 `ARC` 模式下，获取 `weak` 指针时，会调用 `objc_loadWeakRetained` 然后是紧接着调用了一次 `objc_release`，之所以这样，是因为在 `objc_loadWeakRetained` 中会对 `weak` 指针指向的对象调用 `objc_object::rootRetain` 函数，使该对象的引用计数加 1，为了抵消这一次加 1，所以在  `objc_loadWeakRetained` 后面调用 `objc_release` 函数（内部实现其实是: `objc_object::release`）使该对象的引用计数减 1。
    这个加 1 减 1 的操作其实是为了保证在通过 `weak` 指针读取其指向的对象时，防止对象中途被销毁。
    
2. 在 `MRC` 模式下，获取 `weak` 指针时，会调用 `objc_loadWeak` 函数，其内部实现其实是: `objc_autorelease(objc_loadWeakRetained(location))`，即通过 `objc_autorelease` 保证对 `weak` 指针指向的对象的引用计数减 1，保证对象最后能正常释放。

## `objc_loadWeakRetained` 
下面我们来分析 `objc_loadWeakRetained` 函数源码，分析之前我们可以先验证一下 `location` 参数的内容。
还是前面的示例代码，这次我们把断点打在 `objc_loadWeakRetained` 处，然后通过 `LLDB` 我们可以看到如下内容:

```c++
// 控制台右侧部分数据：
// obj 对象地址
obj    NSObject *    0x10112f010    0x000000010112f010
// weakPtr 指向同为 obj 对象地址
weakPtr    NSObject *    0x10112f010    0x000000010112f010
```

```c++
// 控制台右侧：
(lldb) p weakPtr // 打印 weakPtr 指向
(NSObject *) $0 = 0x000000010112f010

// objc_loadWeakRetained 函数参数 location
(lldb) p location
(id *) $1 = 0x00007ffeefbff558

// 打印 location 内存空间里的内容，正是我们的的 obj 对象 
(lldb) p *location
(NSObject *) $2 = 0x000000010112f010

// 查看寄存器
(lldb) register read
General Purpose Registers:
       rax = 0x000000010112f010
       rbx = 0x00007ffeefbff558
       rcx = 0x0000000000000307
       rdx = 0x00007ffeefbff398
       
       rdi = 0x00007ffeefbff558 // rdi 放的 location 参数
       
       rsi = 0x0000000000000000
       rbp = 0x00007ffeefbff520
       rsp = 0x00007ffeefbff3f0
        r8 = 0x0000000000000001
        r9 = 0x0000000000000002
       r10 = 0x00007ffeefbff6e8
       r11 = 0x00000001003d3af0  libobjc.A.dylib`::objc_loadWeakRetained(id *) at NSObject.mm:464
       r12 = 0x0000000000000000
       r13 = 0x0000000000000000
       r14 = 0x0000000000000001
       r15 = 0x0000000000000000
       rip = 0x00000001003d3b02  libobjc.A.dylib`::objc_loadWeakRetained(id *) + 18 at NSObject.mm:473:12
    rflags = 0x0000000000000206
        cs = 0x000000000000002b
        fs = 0x0000000000000000
        gs = 0x0000000000000000

(lldb) memory read 0x00007ffeefbff558 // 读取 0x00007ffeefbff558 里面的内容
0x7ffeefbff558: 10 f0 12 01 01 00 00 00 00 00 00 00 00 00 00 00  ................
0x7ffeefbff568: 00 00 00 00 00 00 00 00 80 f5 bf ef fe 7f 00 00  ................
(lldb) 

0x010112f010 // 正是我们的 obj 对象
```

```c++
/*
  Once upon a time we eagerly cleared *location if we saw the object 
  was deallocating. This confuses code like NSPointerFunctions which 
  tries to pre-flight the raw storage and assumes if the storage is 
  zero then the weak system is done interfering. That is false: the 
  weak system is still going to check and clear the storage later. 
  This can cause objc_weak_error complaints and crashes.
  So we now don't touch the storage until deallocation completes.
  
  ⚠️⚠️⚠️
  所以只要 deallocating 已被标记，不管对象有没有释放完成，weak 变量有没有被置为 nil,
  此时通过 weak 变量获取弱引用的对象都会得到 nil。
*/

id
objc_loadWeakRetained(id *location)
{
    // *location 指向的对象
    id obj;
    id result;
    Class cls;

    SideTable *table;
    
 retry:
    // fixme std::atomic this load
    // 取出 location 内保存的指针指向的对象
    obj = *location;
    
    // 如果对象不存在则返回 nil
    if (!obj) return nil;
    // 如果对象是 tagged Pointer 则返回 obj
    if (obj->isTaggedPointer()) return obj;
    
    // 从全局 SideTables 取得 对象所处的 SideTable
    table = &SideTables()[obj];
    
    // 加锁
    table->lock();
    if (*location != obj) {
        // 如果 *location 被其他线程修改了
        // 则解锁，从 retry 处重新执行
        table->unlock();
        goto retry;
    }
    
    result = obj;

    cls = obj->ISA();
    
    // 在 objc-runtime-new.h 中 __LP64__ 环境下:
    // class or superclass has default retain/release/autorelease/retainCount/
    //   _tryRetain/_isDeallocating/retainWeakReference/allowsWeakReference
    // #define FAST_HAS_DEFAULT_RR     (1UL<<2)
    
    // hasCustomRR 表示重写了 cls 的:
    // 1. retain/release/autorelease
    // 2. retainCount/_tryRetain
    // 3. _isDeallocating
    // 4. retainWeakReference/allowsWeakReference
    // 一般不会重写此方法，因此此值一般是 false，取反则是 true
    if (! cls->hasCustomRR()) {
    
        // Fast case. We know +initialize is complete because
        // default-RR can never be set before then.
        // 我们知道 + initialize 已完成，因为在此之前永远无法设置 default-RR。
        
        // 如果未初始化化则执行断言
        ASSERT(cls->isInitialized());
        
        // 尝试对 obj 做 Retain 操作
        // rootTryRetain 内部实现是:
        // return rootRetain(true, false) ? true : false;
        // 如果 obj 正在析构 deallocating
        // 即如果 obj 的 isa_t 位域：
        // uintptr_t deallocating      : 1;  
        // 为真的话则 rootTryRetain 则会返回 false
        // 即会执行 if 内部的 result = nil
        // 则此时我们读取 weak 指针指向的对象只能得到 nil
        if (! obj->rootTryRetain()) {
            result = nil;
        }
    }
    else {
    
        // Slow case. We must check for +initialize and call it outside
        // the lock if necessary in order to avoid deadlocks.
        // 我们必须检查 + initialize 并在必要时在 lock 解锁后调用它，以避免死锁。
        
        // 保证 cls 已经完成初始化
        if (cls->isInitialized() || _thisThreadIsInitializingClass(cls)) {
            // 判断 cls 是否实现了 retainWeakReference 函数，如果未实现则返回 nil
            BOOL (*tryRetain)(id, SEL) = (BOOL(*)(id, SEL))
                class_getMethodImplementation(cls, @selector(retainWeakReference));
            if ((IMP)tryRetain == _objc_msgForward) {
                result = nil;
            }
            else if (! (*tryRetain)(obj, @selector(retainWeakReference))) {
            // 如果 retainWeakReference 函数返回 false，则返回 nil
                result = nil;
            }
        }
        else {
            // 解锁，执行初始化后，并从 retry 处重新执行
            table->unlock();
            class_initialize(cls, obj);
            goto retry;
        }
    }
    // 解锁    
    table->unlock();
    // 返回 result
    return result;
}
```
&emsp;通过上面一行一行分析，我们可以直白的把 `objc_loadWeakRetained` 函数的功能理解为：**返回弱引用指向的对象，并把该对象的引用计数加 1**，而减 1 的操作 `ARC` 下则是在其后面直接插入一条 `objc_release` 函数，`MRC` 下是放在 `objc_autorelease()` 内，两种方式都保证对象的正常释放。

验证：
`main.m` 中三条读取，下面可看到：
`ARC` 环境下 3 对: `objc_loadWeakRetained` 和 `objc_release` 一一对应。

```c++
#import <Foundation/Foundation.h>
int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // insert code here...
        id obj = [NSObject new];
        
        printf("Start tag");
        {
            __weak id weakPtr = obj;
            
            NSLog(@"%@", weakPtr);
            NSLog(@"%@", weakPtr);
            NSLog(@"%@", weakPtr);
        }
        printf("End tag"); // ⬅️ 这里打断点
    }
    return 0;
}
```
```c++
    0x100000d21 <+49>:  leaq   0x228(%rip), %rdi         ; "Start tag"
    ...
    0x100000d42 <+82>:  callq  0x100000e9c               ; symbol stub for: objc_loadWeakRetained // 1⃣️
    ...
    0x100000d56 <+102>: callq  0x100000e78               ; symbol stub for: NSLog
    ...
    0x100000d5d <+109>: movq   0x2a4(%rip), %rax         ; (void *)0x00007fff71cb7d20: objc_release // 1⃣️
    ...
    0x100000d6d <+125>: callq  0x100000e9c               ; symbol stub for: objc_loadWeakRetained // 2⃣️
    ...
    0x100000d81 <+145>: callq  0x100000e78               ; symbol stub for: NSLog
    ...
    0x100000d88 <+152>: movq   0x279(%rip), %rax         ; (void *)0x00007fff71cb7d20: objc_release // 2⃣️
    ...
    0x100000d98 <+168>: callq  0x100000e9c               ; symbol stub for: objc_loadWeakRetained // 3⃣️
    ...
    0x100000dac <+188>: callq  0x100000e78               ; symbol stub for: NSLog
    ...
    0x100000db6 <+198>: callq  *0x24c(%rip)              ; (void *)0x00007fff71cb7d20: objc_release // 3⃣️
    0x100000dbc <+204>: leaq   -0x20(%rbp), %rdi
    0x100000dc0 <+208>: callq  0x100000e90               ; symbol stub for: objc_destroyWeak
->  0x100000dc5 <+213>: leaq   0x18e(%rip), %rdi         ; "End tag"
    ...
```

`objc_loadWeak` 函数源码:
```c++
/** 
 * This loads the object referenced by a weak pointer and returns it, 
 * after retaining and autoreleasing the object to ensure that it stays
 * alive long enough for the caller to use it. 
 * This function would be used anywhere a __weak variable is used in an expression.
 * 这将加载由弱指针引用的对象，并在保留并自动释放该对象以确保其保持足够长的生存期以供调用者使用后，
 * 将其返回。在表达式中使用 __weak 变量的任何地方都可以使用此函数。
 * 
 * @param location The weak pointer address
 * 
 * @return The object pointed to by \e location, or \c nil if \e location is \c nil.
 */
id
objc_loadWeak(id *location)
{
    if (!*location) return nil;
    return objc_autorelease(objc_loadWeakRetained(location));
}
```

## `objc_copyWeak`
在把一个 `weak` 变量赋值给另一个 `weak` 变量时会调用该函数，如下代码:
```c++
#import <Foundation/Foundation.h>
int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // insert code here...
        id obj = [NSObject new];
        
        printf("Start tag");
        {
            __weak id weakPtr = obj;
            __weak id weakPtr2 = weakPtr;
        }
        printf("End tag");
    }
    return 0;
}
```
汇编代码:
```c++
...
0x100000e52 <+82>:  callq  0x100000eda               ; symbol stub for: objc_copyWeak // ⬅️
...
```

`objc_copyWeak` 函数源码:
```c++
/** 
 * This function copies a weak pointer from one location to another,
 * when the destination doesn't already contain a weak pointer. It
 * would be used for code like:
 *
 *  __weak id src = ...;
 *  __weak id dst = src;
 * 
 * This function IS NOT thread-safe with respect to concurrent 
 * modifications to the destination variable. (Concurrent weak clear is safe.)
 *
 * @param dst The destination variable.
 * @param src The source variable.
 */
void
objc_copyWeak(id *dst, id *src)
{
    // 首先从 src weak 变量获取所知对象，并引用加 1，保证不释放
    id obj = objc_loadWeakRetained(src);
    
    // 初始化 dst weak 变量
    objc_initWeak(dst, obj);
    
    // obj 引用计数减 1，保证能正常释放
    objc_release(obj);
}
```
## `objc_moveWeak`
`objc_moveWeak` 函数源码:
```c++
/** 
 * Move a weak pointer from one location to another.
 * 将弱指针从一个位置移动到另一位置。
 * Before the move, the destination must be uninitialized.
 * 在移动之前，destination 必须未初始化。
 * After the move, the source is nil.
 * 移动后，source 置为 nil。
 *
 * This function IS NOT thread-safe with respect to concurrent 
 * modifications to either weak variable. (Concurrent weak clear is safe.)
 * 此函数不是线程安全的。
 */
void
objc_moveWeak(id *dst, id *src)
{
    // 把 src weak 变量复制给 dst weak 变量
    objc_copyWeak(dst, src);
    // 销毁 src
    objc_destroyWeak(src);
    // src 置为 nil
    *src = nil;
}
```
写到这里，觉得 `weak` 可以暂时做一个小完结了。😊

## 参考链接
**参考链接:🔗**
+ [详解获取weak对象的过程](https://www.jianshu.com/p/b22a446f8a8d)
+ [ObjC Runtime 中 Weak 属性的实现 (上)](https://www.jianshu.com/p/fb6b15e8acfd)
+ [weak指针的线程安全和自动置nil的深度探讨](https://www.jianshu.com/p/edbd1ec314aa)
