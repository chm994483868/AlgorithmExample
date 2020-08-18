#  iOS底层weak修饰对象存储原理
问题：为何 weak 修饰符的变量可以打破循环引用？
因为 weak 修饰的变量存储在散列表中的弱引用表里，不参与引用计数器的使用，也就是说，在进行释放的时候，不管你怎么引用，直接把你置空了。

基本概念：
+ `SideTable`：散列表：系统自动维护，用于存储/管理一些信息。
  `SideTable` 的结构中能看到其管理是三种表：
  `spinlock_t slock` 自旋锁表；
  `RefcountMap refcnts` 引用计数表；
  `weak_table_t weak_table` 弱引用表；
  + `weak_table`: 弱引用对象存储的表，是 `SideTable` 的一张表。
  + `weak_entry_t`: `weak_table` 里面的一个单元，用于管理当前类的弱引用对象，可以理解为一个数组，查看 `weak_entry_t` 的结构，有助于更好的理解里面的存储结构，里面包含一个 `weak_referrer_t`，相当于一个数组，这里面的就是存储的弱引用对象，还有其他的一些信息，比如 `mask` (蒙板，容量 -1)、`num_refs`（当前存储的数量）等。
  + `weak_referrer_t`：`weak_entry_t` 中的弱引用对象，相当于是数组中的一个元素。

## 存储原理：
### 1、源码探索入口
  写如下代码，打上断点，并打开汇编模式：`debug -> debug workflow -> alway show disassembly` :

```
  LGPerson *person = [[LGPerson alloc] init];
  NSLog(@"🆚🆚🆚 %@", person);
  __weak LGPerson *weakPer = person; // 这里打断点
  NSLog(@"🆚🆚🆚 %@", weakPer);
```
运行后会进入断点，出现这样的信息：
```
....
->  0x100000e5b <+91>:  movq   -0x20(%rbp), %rsi
0x100000e5f <+95>:  leaq   -0x28(%rbp), %rbx
0x100000e63 <+99>:  movq   %rbx, %rdi
0x100000e66 <+102>: callq  0x100000ef6               ; symbol stub for: objc_initWeak
...
```
找到 `callq` 方法：`objc_initWeak`，拿到这个方法就可以进入源码调试了。

#### 源码探索
`objc_initWeak` 函数：
```
/** 
 * Initialize a fresh weak pointer to some object location. 
 * 初始化指向某个对象位置的新的 weak 指针。
 * It would be used for code like: 
 * 类似如下代码：
 
 * (The nil case) 
 * __weak id weakPtr;
 
 * (The non-nil case) 
 * NSObject *o = ...;
 * __weak id weakPtr = o;
 * 
 * 对于 weak 变量的并发修改，此函数不是线程安全的。 （并发进行 弱清除 是安全的。）
 * This function IS NOT thread-safe with respect to concurrent
 * modifications to the weak variable. (Concurrent weak clear is safe.)
 *
 * @param location Address of __weak ptr. 
 * @param newObj Object ptr. 
 */
id
objc_initWeak(id *location, id newObj)
{
    if (!newObj) {
        *location = nil;
        return nil;
    }

    return storeWeak<DontHaveOld, DoHaveNew, DoCrashIfDeallocating>
        (location, (objc_object*)newObj);
}
```
### 1.1、内部做的操作是存储 weak -- storeWeak
```

```


  
