# iOS 从源码解析Runtime (十四)：由源码解读属性修饰符

## `atomic/nonatomic`
&emsp;当我们分别使用 `atomic` 和 `nonatomic` 来修饰属性时，编译器是怎么处理这两种不同的情况的呢？大家都知道即使使用 `atomic` 修饰属性也并不能保证线程安全，那它和 `nonatomic` 还有什么区别呢？下面我们一起通过汇编和源码来看一下，首先定义如下类，添加三个属性:
```c++
// LGPerson.h，.m 文件什么也不写
#import <Foundation/Foundation.h>
NS_ASSUME_NONNULL_BEGIN
@interface LGPerson : NSObject

@property NSMutableArray *arr_default;
@property (nonatomic, strong) NSMutableArray *arr_nonatomic;
@property (atomic, strong) NSMutableArray *arr_atomic;

@end
NS_ASSUME_NONNULL_END
```
选择真机运行模式，保证 `Assemble LGPerson` 出的是 `ARM` 下的汇编指令，（`x86` 下的看不懂😭）。然后在 `xcode` 左侧用鼠标选中 `LGPerson.m` 文件，通过 `xcode` 菜单栏 `Product -> Perform Action -> Assemble "LGPerson.m"` 生成汇编指令，可以看到我们的三个属性所对应的 `setter getter` 方法。

### `objc_getProperty`
```c++
id objc_getProperty(id self, SEL _cmd, ptrdiff_t offset, BOOL atomic) {
    if (offset == 0) {
        return object_getClass(self);
    }

    // Retain release world
    id *slot = (id*) ((char*)self + offset);
    if (!atomic) return *slot;
        
    // Atomic retain release world
    spinlock_t& slotlock = PropertyLocks[slot];
    slotlock.lock();
    id value = objc_retain(*slot);
    slotlock.unlock();
    
    // for performance, we (safely) issue the autorelease OUTSIDE of the spinlock.
    return objc_autoreleaseReturnValue(value);
}
```


## 参考链接
**参考链接:🔗**
+ [ObjC如何通过runtime修改Ivar的内存管理方式](https://www.cnblogs.com/dechaos/p/7246351.html) 
+ [iOS基础系列-- atomic, nonatomic](https://xiaozhuanlan.com/topic/2354790168)
+ [Declared Properties](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/ObjCRuntimeGuide/Articles/ocrtPropertyIntrospection.html)
+ [iOS @property 属性相关的总结](https://juejin.im/post/6844903824436494343)
+ [atomic关键字的一些理解](https://www.jianshu.com/p/5951cb93bcef)
