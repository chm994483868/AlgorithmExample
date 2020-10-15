# Block 临时文件


## 此段代码可作为区分全局、堆区、栈区 block 的实例代码
```c++
typedef void(^Blk_T)(void);
void (^globalBlock0)(void) = ^{
    NSLog(@"全局区的 block");
};

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        
        // 0. 在全局区定义的 NSGlobalBlock
        NSLog(@"🎉🎉🎉 GlobalBlock0 is %@", globalBlock0);
        // 🎉🎉🎉 GlobalBlock0 is <__NSGlobalBlock__: 0x100002020>
        
        // 1. 不捕获外部变量时是 NSGlobalBlock。
        //（此处即使发生赋值时 ARC 下会调用 copy，但是由于左值是 NSGlobalBlock，它调用 copy 函数时依然返回它自己）
        void (^globalBlock1)(void) = ^{ };
        NSLog(@"🎉🎉🎉 GlobalBlock1 is %@", globalBlock1);
        // 🎉🎉🎉 GlobalBlock1 is <__NSGlobalBlock__: 0x100002060>
        
        static int b = 10;
        // 2. 仅捕获外部静态局部变量的是 NSGlobalBlock
        //（此处即使发生赋值时 ARC 下会调用 copy，但是由于左值是 NSGlobalBlock，它调用 copy 函数时依然返回它自己）
        void (^globalBlock2)(void) = ^{
            b = 20;
        };
        NSLog(@"🎉🎉🎉 GlobalBlock2 is %@", globalBlock2);
        // 🎉🎉🎉 GlobalBlock2 is <__NSGlobalBlock__: 0x100002080>

        int a = 0;
        // 3. 仅捕获外部局部变量是的 NSStackBlock
        NSLog(@"🎉🎉🎉 StackBlock3 is %@", ^{ NSLog(@"%d", a); });
        // 🎉🎉🎉 StackBlock3 is <__NSStackBlock__: 0x7ffeefbff4c8>

        // 4. ARC 下 NSStackBlock 赋值给 __strong 变量时发生 copy，创建一个 NSMallocBlock 赋给右值
        // MRC 下编译器不会自动发生 copy，赋值以后右值同样也是 NSStackBlock，如果想实现和 ARC 同样效果需要手动调用 copy
        void (^mallocBlock)(void) = ^{
            NSLog(@"%d", a);
        };
        NSLog(@"🎉🎉🎉 MallocBlock4 is %@", mallocBlock);
        // 🎉🎉🎉 MallocBlock4 is <__NSMallocBlock__: 0x1005005e0>
        
        // 5. ARC 或 MRC 下赋值给 __weak/__unsafe_unretained 变量均不发生 copy，
        // 手动调用 copy 是可转为 NSMallocBlock
        // __unsafe_unretained / __weak
        __unsafe_unretained Blk_T mallocBlock2;
        mallocBlock2 = ^{
            NSLog(@"%d", a);
        };
        // mallocBlock2 是：NSStackBlock，其实应该和上面的 StackBlock 写在一起
        NSLog(@"🎉🎉🎉 MallocBlock5 is %@", mallocBlock2);
        // 🎉🎉🎉 MallocBlock5 is <__NSStackBlock__: 0x7ffeefbff518>
        
    }
    return 0;
}
```

## **小测试**
+ A:
```
void exampleA() {
    // ARC 和 MRC 下均为栈区 Block
    char a = 'A';
    NSLog(@"🔔🔔🔔 %@", ^{ printf("%c\n", a);});
}
// MRC: 🔔🔔🔔 <__NSStackBlock__: 0x7ffeefbff538>
// ARC: 🔔🔔🔔 <__NSStackBlock__: 0x7ffeefbff538>

void exampleA() {
    // ARC 和 MRC 下均为全局 Block
    NSLog(@"🔔🔔🔔 %@", ^{ printf("🟪🟪🟪");});
}
// ARC: 🔔🔔🔔 <__NSGlobalBlock__: 0x100002048>
// MRC: 🔔🔔🔔 <__NSGlobalBlock__: 0x100001038>
```
```
void exampleA() {
    // ARC 和 MRC 下均为栈区 Block
    char a = 'A';
    // 没有赋值操作
    ^{
        printf("🔔🔔🔔 %c\n", a);
    }();
}

// MRC: 🔔🔔🔔 A
// ARC: 🔔🔔🔔 A
```
+ B:
```
void exampleB_addBlockToArray(NSMutableArray *array) {
    char b = 'B';
    // 原以为栈区 Block，ARC 下是堆区 Block
    // MRC 下估计是栈区 Block，执行的时候崩溃了
    [array addObject:^{
        printf("🔔🔔🔔 %c\n", b);
    }];
    NSLog(@"🔔🔔🔔 %@", array);
}

void exampleB() {
    NSMutableArray *array = [NSMutableArray array];
    exampleB_addBlockToArray(array);
    
    NSLog(@"🔔🔔🔔 %@", [array objectAtIndex:0]);
    
    void(^block)() = [array objectAtIndex:0];
    
    NSLog(@"🔔🔔🔔 %@", block);
    block();
}

// ARC: 🔔🔔🔔 ( "<__NSMallocBlock__: 0x102840050>")
        🔔🔔🔔 <__NSMallocBlock__: 0x100611690>
        🔔🔔🔔 <__NSMallocBlock__: 0x100611690>
        🔔🔔🔔 B
// MRC: 崩溃 ， 在 addObject 添加 block 时调用 copy 函数，能正常运行。
```
+ C:
```
void exampleC_addBlockToArray(NSMutableArray *array) {
  // 全局 Global 
  [array addObject:^{
    printf("🔔🔔🔔 C\n");
  }];
}

void exampleC() {
    NSMutableArray *array = [NSMutableArray array];
    exampleC_addBlockToArray(array);
    NSLog(@"🔔🔔🔔 %@", [array objectAtIndex:0]);
    void(^block)() = [array objectAtIndex:0];
    NSLog(@"🔔🔔🔔 %@", block);
    block();
}

// ARC: 🔔🔔🔔 <__NSGlobalBlock__: 0x100002068>
        🔔🔔🔔 <__NSGlobalBlock__: 0x100002068>
        🔔🔔🔔 C
        
// MRC: 🔔🔔🔔 <__NSGlobalBlock__: 0x100001078>
        🔔🔔🔔 <__NSGlobalBlock__: 0x100001078>
        🔔🔔🔔 C
```
+ D:
```
typedef void(^dBlock)();
dBlock exampleD_getBlock() {
    // ARC 栈区 block 作为函数返回值时会自动调用 copy
    // MRC 下编译直接报错: Returning block that lives on the local stack
    // 主动 block 尾部调 copy 可解决
    
    char d = 'D';
    return ^{
        printf("🔔🔔🔔 %c\n", d);
    };
}

void exampleD() {
    NSLog(@"🔔🔔🔔 %@", exampleD_getBlock());
    exampleD_getBlock()();
}
// ARC: 🔔🔔🔔 <__NSMallocBlock__: 0x100500d00>
        🔔🔔🔔 D
```
+ E:
```
typedef void(^eBlock)();
eBlock exampleE_getBlock() {
    char e = 'E';
    void(^block)() = ^{
        printf("🔔🔔🔔 %c\n", e);
    };
    NSLog(@"🔔🔔🔔 %@", block);
    return block;
}

void exampleE() {
    NSLog(@"one 🔔🔔🔔 %@", exampleE_getBlock());
    
    eBlock block = exampleE_getBlock();
    NSLog(@"two 🔔🔔🔔 %@", block);
    block();
}
// MRC 下即使是栈区 Block 也正常执行了，且两次调用函数返回的是一样的地址
// MRC: 🔔🔔🔔 <__NSStackBlock__: 0x7ffeefbff508>
        🔔🔔🔔 <__NSStackBlock__: 0x7ffeefbff508>
        
        🔔🔔🔔 <__NSStackBlock__: 0x7ffeefbff508>
        🔔🔔🔔 <__NSStackBlock__: 0x7ffeefbff508>
        🔔🔔🔔 P
        
        // 两次地址不同
// ARC: 🔔🔔🔔 <__NSMallocBlock__: 0x100550d10>
        🔔🔔🔔 <__NSMallocBlock__: 0x100550d10>
        
        🔔🔔🔔 <__NSMallocBlock__: 0x100602d00>
        🔔🔔🔔 <__NSMallocBlock__: 0x100602d00>
        🔔🔔🔔 E
```
**_Block_object_assign 源码分析之前:**
```
BLOCK_EXPORT void _Block_object_assign(void *, const void *, const int) __OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);
```
const int 参数对应一个枚举:

Block_private.h 文件 332 行:
```
// Runtime support functions used by compiler when generating copy/dispose helpers
// 当编译器生成 copy/dispose helpers 时 Runtime 支持的函数
// Values for _Block_object_assign() and _Block_object_dispose() parameters
// 作为 _Block_object_assign() 和 _Block_object_dispose() 函数的参数
enum {
    // see function implementation for a more complete description of these fields and combinations
    // 有关这些字段及其组合的更完整说明，参见函数实现
    
    // OC 对象类型
    BLOCK_FIELD_IS_OBJECT   =  3,  // id, NSObject, __attribute__((NSObject)), block, ...
    // 为另一个 Block
    BLOCK_FIELD_IS_BLOCK    =  7,  // a block variable
    // 为一个被 __block 修饰后生成的结构体
    BLOCK_FIELD_IS_BYREF    =  8,  // the on stack structure holding the __block variable
    // 被 __weak 修饰过的弱引用，只在 Block_byref 管理内部对象内存时使用
    // 也就是 __block __weak id;
    BLOCK_FIELD_IS_WEAK     = 16,  // declared __weak, only used in byref copy helpers
    // 在处理 Block_byref 内部对象内存的时候会加一个额外标记，配合上面的枚举一起使用
    BLOCK_BYREF_CALLER      = 128, // called from __block (byref) copy/dispose support routines.
};

enum {
    // 上述情况的整合，即以上都会包含 copy_dispose 助手
    BLOCK_ALL_COPY_DISPOSE_FLAGS = 
        BLOCK_FIELD_IS_OBJECT | BLOCK_FIELD_IS_BLOCK | BLOCK_FIELD_IS_BYREF |
        BLOCK_FIELD_IS_WEAK | BLOCK_BYREF_CALLER
};
```
源码验证：
```
NSObject *is_object = [[NSObject alloc] init]; // 对象类型
void (^is_block)() = ^{ NSLog(@"is_block 参数"); }; // block 
__block NSObject *is_byref = [[NSObject alloc] init]; // __block 对象
NSObject *tt = [[NSObject alloc] init];
__block __unsafe_unretained NSObject *is_weak = tt; // __weak __block 同时修饰

NSLog(@"⛈⛈⛈ is_byref retainCount = %lu ---%p---%p", (unsigned long)[is_byref arcDebugRetainCount], is_byref, &is_byref); // 堆区 栈区

void (^aBlock)() = ^{
    NSLog(@"⛈⛈⛈ is_object retainCount = %lu ---%p---%p", (unsigned long)[is_object arcDebugRetainCount], is_object, &is_object);
    is_block();
    
    NSLog(@"⛈⛈⛈ is_byref retainCount = %lu ---%p---%p", (unsigned long)[is_byref arcDebugRetainCount], is_byref, &is_byref);
    NSLog(@"⛈⛈⛈ is_weak retainCount = %lu ---%p---%p", (unsigned long)[is_weak arcDebugRetainCount], is_weak, &is_weak);
    NSLog(@"⛈⛈⛈ is_only_weak retainCount = %lu ---%p---%p", (unsigned long)[is_only_weak arcDebugRetainCount], is_only_weak, &is_only_weak);
};

// 部分转换后的代码:

struct __main_block_impl_1 {
  struct __block_impl impl;
  struct __main_block_desc_1* Desc;
  
  // 捕获的变量
  NSObject *is_object;
  struct __block_impl *is_block;
  NSObject *is_only_weak;
  __Block_byref_is_byref_0 *is_byref; // by ref
  __Block_byref_is_weak_1 *is_weak; // by ref
    
  __main_block_impl_1(void *fp, struct __main_block_desc_1 *desc, NSObject *_is_object, void *_is_block, NSObject *_is_only_weak, __Block_byref_is_byref_0 *_is_byref, __Block_byref_is_weak_1 *_is_weak, int flags=0) : is_object(_is_object), is_block((struct __block_impl *)_is_block), is_only_weak(_is_only_weak), is_byref(_is_byref->__forwarding), is_weak(_is_weak->__forwarding) {
    impl.isa = &_NSConcreteStackBlock;
    impl.Flags = flags;
    impl.FuncPtr = fp;
    Desc = desc;
  }
};

// copy
static void __main_block_copy_1(struct __main_block_impl_1*dst, struct __main_block_impl_1*src) {
    
    _Block_object_assign((void*)&dst->is_object, (void*)src->is_object, 3/*BLOCK_FIELD_IS_OBJECT*/);
    _Block_object_assign((void*)&dst->is_block, (void*)src->is_block, 7/*BLOCK_FIELD_IS_BLOCK*/);
    _Block_object_assign((void*)&dst->is_byref, (void*)src->is_byref, 8/*BLOCK_FIELD_IS_BYREF*/);
    _Block_object_assign((void*)&dst->is_weak, (void*)src->is_weak, 8/*BLOCK_FIELD_IS_BYREF*/);
    _Block_object_assign((void*)&dst->is_only_weak, (void*)src->is_only_weak, 3/*BLOCK_FIELD_IS_OBJECT*/);
    
}

// dispose
static void __main_block_dispose_1(struct __main_block_impl_1*src) {
    
    _Block_object_dispose((void*)src->is_object, 3/*BLOCK_FIELD_IS_OBJECT*/);
    _Block_object_dispose((void*)src->is_block, 7/*BLOCK_FIELD_IS_BLOCK*/);
    _Block_object_dispose((void*)src->is_byref, 8/*BLOCK_FIELD_IS_BYREF*/);
    _Block_object_dispose((void*)src->is_weak, 8/*BLOCK_FIELD_IS_BYREF*/);
    _Block_object_dispose((void*)src->is_only_weak, 3/*BLOCK_FIELD_IS_OBJECT*/);
    
}

```

**这里针对 __block 变量解释一下：**
+ __block NSObject *object = [[NSObject alloc] init]; 
+ __Block_byref_object_0 结构体
+ 首先 NSObject 对象是处于堆区的，__block 结构体实例是处于栈区的。
+ Block 发生 copy 操作从栈区到堆区时：原始的 NSObject 对象是不动的，是 __block 结构体实例被复制到了堆区。
+ 且复制以后，原始栈区的 __block 结构体实例会断开对 NSObject 对象的引用
+ 堆区的 __block 结构体实例持有 NSObject 对象实例，NSObject 对象实例的引用计数此时还是 1
```
__block NSObject *object = [[NSObject alloc] init];
NSLog(@"⛈⛈⛈ object retainCount = %lu ---%p---%p", (unsigned long)[object arcDebugRetainCount], object, &object); // 堆区 栈区

void (^aBlock)() = ^{
    NSLog(@"⛈⛈⛈ object retainCount = %lu ---%p---%p", (unsigned long)[object arcDebugRetainCount], object, &object);
};

aBlock(); // 堆区 堆区
void (^bBlock)() = [aBlock copy];
bBlock(); // 堆区 堆区
NSObject *temp = object;
bBlock(); // 堆区 堆区
aBlock(); // 堆区 堆区
NSLog(@"⛈⛈⛈ object retainCount = %lu ---%p---%p", (unsigned long)[object arcDebugRetainCount], object, &object); // 堆区 堆区

// 打印：
⛈⛈⛈ object retainCount = 1 ---0x100738890---0x7ffeefbff578
⛈⛈⛈ object retainCount = 1 ---0x100738890---0x10073a628
⛈⛈⛈ object retainCount = 1 ---0x100738890---0x10073a628
⛈⛈⛈ object retainCount = 2 ---0x100738890---0x10073a628
⛈⛈⛈ object retainCount = 2 ---0x100738890---0x10073a628
⛈⛈⛈ object retainCount = 2 ---0x100738890---0x10073a628
```

**block 捕获的对象类型变量，在block 结构体中有个对应的对象类型指针，一直指向该对象类型的实例。**
**__block 结构体实例的对象类型的成员变量作为一个指针，一直指向该对象的实例。**

**堆区地址小于栈区地址，基本类型存在栈区**

// 这里

```
// 示例 1：
NSObject *obj = [[NSObject alloc] init];
__block NSObject *object = obj;
NSLog(@"⛈⛈⛈ obj retainCount = %lu", (unsigned long)[obj arcDebugRetainCount]);
// 打印：
⛈⛈⛈ obj retainCount = 2 // 被 obj 和 object 持有

// 示例 2:
__block NSObject *object = [[NSObject alloc] init];
NSLog(@"⛈⛈⛈ object retainCount = %lu", (unsigned long)[object arcDebugRetainCount]);
// 打印：
⛈⛈⛈ object retainCount = 1 // 只被 object 持有

// 示例 3:
__block NSObject *object = [[NSObject alloc] init];
^{
    NSLog(@"%@", object);
};

NSLog(@"⛈⛈⛈ object retainCount = %lu", (unsigned long)[object arcDebugRetainCount]);
// 打印：
⛈⛈⛈ object retainCount = 1 // 只被 object 持有，栈区的 block 持有 object 结构体 

// 示例 4：对比上面不用 __block 修饰，引用为 2，一次被变量 object 持有，一次被 block 持有
// 接下来为了区分变量在栈区还是堆区，打印它们的地址方便比较
NSObject *object = [[NSObject alloc] init];
NSLog(@"⛈⛈⛈ object retainCount = %lu ---%p---%p", (unsigned long)[object arcDebugRetainCount], object, &object);
^{
    NSLog(@"⛈⛈⛈ object retainCount = %lu ---%p---%p", (unsigned long)[object arcDebugRetainCount], object, &object);
}();

NSLog(@"⛈⛈⛈ object retainCount = %lu ---%p---%p", (unsigned long)[object arcDebugRetainCount], object, &object);
// 打印：这里有个细节，看三次 object 打印地址是相同的，都指向原始的 NSObject 对象，这没有问题，
// 第二行 block 内部的打印 &object 地址不同与上下两次，因为这个 object 是 block 结构体的 object 成员变量（类型是个指针）的地址
// 上下两次还是栈中的 object 变量
⛈⛈⛈ object retainCount = 1 ---0x102800750---0x7ffeefbff578
⛈⛈⛈ object retainCount = 2 ---0x102800750---0x7ffeefbff560
⛈⛈⛈ object retainCount = 2 ---0x102800750---0x7ffeefbff578

// 示例 5：
 __block NSObject *object = [[NSObject alloc] init];
 NSLog(@"⛈⛈⛈ object retainCount = %lu ---%p---%p", (unsigned long)[object arcDebugRetainCount], object, &object);
^{
    NSLog(@"⛈⛈⛈ object retainCount = %lu ---%p---%p", (unsigned long)[object arcDebugRetainCount], object, &object);
}();

NSLog(@"⛈⛈⛈ object retainCount = %lu ---%p---%p", (unsigned long)[object arcDebugRetainCount], object, &object);

// 打印：都是 1 ，只被 __block 变量 object 持有，在栈 Block中，自始只有强指针指向 object，就是__block生成的结构体。
// 三次地址完全相同，这里涉及到 __block 变量对应的结构体中的 __forwarding 指针
// 上下是：
// &(object.__forwarding->object)
// 中间 block 执行是：
&(object->__forwarding->object)
// 至此取的的 object 就是原始的 NSObject 对象
⛈⛈⛈ object retainCount = 1 ---0x102802820---0x7ffeefbff578
⛈⛈⛈ object retainCount = 1 ---0x102802820---0x7ffeefbff578
⛈⛈⛈ object retainCount = 1 ---0x102802820---0x7ffeefbff578

// 示例 6:
 __block NSObject *object = [[NSObject alloc] init];
 NSLog(@"⛈⛈⛈ object retainCount = %lu ---%p---%p", (unsigned long)[object arcDebugRetainCount], object, &object);
void (^aBlock)() = ^{
    NSLog(@"⛈⛈⛈ object retainCount = %lu ---%p---%p", (unsigned long)[object arcDebugRetainCount], object, &object);
};
aBlock();

NSLog(@"⛈⛈⛈ object retainCount = %lu ---%p---%p", (unsigned long)[object arcDebugRetainCount], object, &object);
// 打印：

// 极其重要的一句: "并断开栈中的obj结构体对obj对象的指向" "并断开栈中的obj结构体对obj对象的指向" "并断开栈中的obj结构体对obj对象的指向" 

可以看到，obj的内存地址一直在栈中，而执行了BlockCopy后，obj指针的地址从栈变到了堆中，而obj的引用计数一直是1。在copy操作之后，结构体obj也被复制到了堆中，并断开栈中的obj结构体对obj对象的指向。那如果这个时候取栈中的obj不就有问题了？__forwarding就派上用场了，上面编译的结果发现，结构体对象在使用obj的时候会使用obj->__forwarding->obj，如果所有__forwarding都指向自己，这一步还有什么意义？栈Block在执行copy操作后，栈obj结构体的__forwarding就会指向copy到堆中的obj结构体。此时再取值，操作的就是同一份指针了。证明如下:

// 示例 7：
__block NSObject *object = [[NSObject alloc] init];
NSLog(@"⛈⛈⛈ object retainCount = %lu ---%p---%p", (unsigned long)[object arcDebugRetainCount], object, &object);

void (^aBlock)() = ^{
    NSLog(@"⛈⛈⛈ object retainCount = %lu ---%p---%p", (unsigned long)[object arcDebugRetainCount], object, &object);
};

aBlock();
void (^bBlock)() = [aBlock copy];
bBlock();
aBlock();
NSLog(@"⛈⛈⛈ object retainCount = %lu ---%p---%p", (unsigned long)[object arcDebugRetainCount], object, &object);

// MRC 下打印：
⛈⛈⛈ object retainCount = 1 ---0x10065bd50---0x7ffeefbff570 // 原始状态: 堆区 栈区
⛈⛈⛈ object retainCount = 1 ---0x10065bd50---0x7ffeefbff570 // 堆区 栈区 // 这里虽然发生了赋值操作，但是并没有主动被复制到堆区

⛈⛈⛈ object retainCount = 1 ---0x10065bd50---0x1010083f8 // 堆区 堆区 // 这里开始主动调用了 copy 才被复制到堆区
⛈⛈⛈ object retainCount = 1 ---0x10065bd50---0x1010083f8 // 堆区 堆区
⛈⛈⛈ object retainCount = 1 ---0x10065bd50---0x1010083f8 // 堆区 堆区

// ARC 下打印：
⛈⛈⛈ object retainCount = 1 ---0x1006002e0---0x7ffeefbff578  // 原始状态: // 堆区 栈区
⛈⛈⛈ object retainCount = 1 ---0x1006002e0---0x1007396e8 // 堆区 堆区 // 这里发生了赋值操作，__block 变量被复制到堆区
⛈⛈⛈ object retainCount = 1 ---0x1006002e0---0x1007396e8 // 堆区 堆区
⛈⛈⛈ object retainCount = 1 ---0x1006002e0---0x1007396e8 // 堆区 堆区
⛈⛈⛈ object retainCount = 1 ---0x1006002e0---0x1007396e8 // 堆区 堆区

由上可知，在copy之前，aBlock的打印结果都是初始化生成的指针，而copy之后打印就和bBlock的打印结果相同了。总结一下就是，在栈中的obj结构体__forwarding指向的就是栈中的自己，执行copy之后，会在堆中生成一份obj结构体并断开栈中对obj的引用，此时堆中的obj结构体__forwarding就指向自己，而栈中的__forwarding就指向堆中的obj结构体。下面也会通过分析源码来具体解释。

__block NSObject *object = [[NSObject alloc] init];
NSLog(@"⛈⛈⛈ object retainCount = %lu ---%p---%p", (unsigned long)[object arcDebugRetainCount], object, &object); // 堆区 栈区

void (^aBlock)() = ^{
    NSLog(@"⛈⛈⛈ object retainCount = %lu ---%p---%p", (unsigned long)[object arcDebugRetainCount], object, &object);
};

NSObject *temp = object; // + 1
aBlock(); // 堆区 堆区
void (^bBlock)() = [aBlock copy];
bBlock(); // 堆区 堆区
aBlock(); // 堆区 堆区
NSLog(@"⛈⛈⛈ object retainCount = %lu ---%p---%p", (unsigned long)[object arcDebugRetainCount], object, &object); // 堆区 堆区
// 打印：
⛈⛈⛈ object retainCount = 1 ---0x10053e1b0---0x7ffeefbff578
⛈⛈⛈ object retainCount = 2 ---0x10053e1b0---0x10053e988
⛈⛈⛈ object retainCount = 2 ---0x10053e1b0---0x10053e988
⛈⛈⛈ object retainCount = 2 ---0x10053e1b0---0x10053e988
⛈⛈⛈ object retainCount = 2 ---0x10053e1b0---0x10053e988
```

**堆 Block __NSMallocblock__ 内存由 ARC 控制，没有强指针指向时释放。而在 MRC 中，赋值不会执行 copy 操作，所以左侧 block 依然存在于栈中，所以在 MRC 中一般都需要执行 copy，否则很容易造成 crash.在 ARC 中，当 Block 作为属性被 strong、copy 修饰或被强指针引用或作为返回值时，都会默认执行 copy。而 MRC 中，只有被 copy 修饰时，block 才会执行 copy。所以 MRC 中 Block 都需要用 copy 修饰，而在 ARC 中用 copy 修饰只是沿用了 MRC 的习惯，此时用 copy 和 strong效果是相同的。**

**Block 在捕获外部变量的操作基本一致，都是在生成结构体的时候将所有 Block 里用到的外部变量作为属性保存起来。self.block 里面调用 self 会造成循环引用，因为 Block 捕获了 self 并把 self 当做一个值保存了起来。**

**Block里的a只是copy过去的a的值，在Block里改变a的值也不会影响外面，编译器避免这个错误就报错。**

**同样的，捕获对象时也是对指针的copy，生成一个指针指向obj对象，所以如果在Block中直接让obj指针指向另外一个对象也会报错。这点编译器已经加了注释 // bound by copy。**

**多了__main_block_copy_0 和 __main_block_dispose_0 这两个函数，并在描述 __main_block_desc_0 结构体中保存了这两个函数指针。这就是上面所说的 copy_dispose 助手，C 语言结构体中，编译器很难处理对象的初始化和销毁操作，所以使用 runtime 来管理相关内存。BLOCK_FIELD_IS_OBJECT 是在捕获对象时添加的特别标识，此时是3，下面会细讲。**

**此 Block 是为栈 Block_NSConcreteStackBlock，不过在 ARC 中，因为赋值给 aBlock，会执行一次 copy，将其中栈中 copy 到堆中，所以在 MRC 中 aBlock 为 _NSConcreteStackBlock，在 ARC 中就是 _NSConcreteMallocBlock。**


+++++++++++++++++++++++++++++++++++++

void (*__Block_byref_id_object_copy)(void*, void*);
void (*__Block_byref_id_object_dispose)(void*);

NSObject *object;
};

struct __main_block_impl_0 {
 struct __block_impl impl;
 struct __main_block_desc_0* Desc;
 
 // 对应 5 个 __block 变量的结构体类型的成员变量
 __Block_byref_fmt_0 *fmt; // by ref
 __Block_byref_val_1 *val; // by ref
 __Block_byref_temp_2 *temp; // by ref
 __Block_byref_array_3 *array; // by ref
 __Block_byref_object_4 *object; // by ref
 
 // 初始化列表：
 // fmt(_fmt->__forwarding), 
 // val(_val->__forwarding), 
 // temp(_temp->__forwarding),
 // array(_array->__forwarding),
 // object(_object->__forwarding)
 
 __main_block_impl_0(void *fp, struct __main_block_desc_0 *desc, __Block_byref_fmt_0 *_fmt, __Block_byref_val_1 *_val, __Block_byref_temp_2 *_temp, __Block_byref_array_3 *_array, __Block_byref_object_4 *_object, int flags=0) : fmt(_fmt->__forwarding), val(_val->__forwarding), temp(_temp->__forwarding), array(_array->__forwarding), object(_object->__forwarding) {
   impl.isa = &_NSConcreteStackBlock;
   impl.Flags = flags;
   impl.FuncPtr = fp;
   Desc = desc;
 }
 
 static void __main_block_func_0(struct __main_block_impl_0 *__cself) {
 // 取出成员变量
 __Block_byref_fmt_0 *fmt = __cself->fmt; // bound by ref
 __Block_byref_val_1 *val = __cself->val; // bound by ref
 __Block_byref_temp_2 *temp = __cself->temp; // bound by ref
 __Block_byref_array_3 *array = __cself->array; // bound by ref
 __Block_byref_object_4 *object = __cself->object; // bound by ref
           
           // 这里函数执行有一手，用的 ->__forwarding->fmt 去找值
           // 保证不管是堆区还是栈区执行函数，找到的一个值都是堆上的或者栈上的，
           // 就是大家操作的都是同一个地址的数据
           (fmt->__forwarding->fmt) = "FMT val = %d\n";
           printf((fmt->__forwarding->fmt), (val->__forwarding->val));
           (temp->__forwarding->temp) = 30;
           
           ((void (*)(id, SEL, ObjectType _Nonnull))(void *)objc_msgSend)((id)(array->__forwarding->array), sel_registerName("addObject:"), (id _Nonnull)(object->__forwarding->object));
   }
   
   // Block 用的 copy，只有 Block 中使用 __block 变量时才会出现 
   static void __main_block_copy_0(struct __main_block_impl_0*dst, struct __main_block_impl_0*src) {_Block_object_assign((void*)&dst->fmt, (void*)src->fmt, 8/*BLOCK_FIELD_IS_BYREF*/);_Block_object_assign((void*)&dst->val, (void*)src->val, 8/*BLOCK_FIELD_IS_BYREF*/);_Block_object_assign((void*)&dst->temp, (void*)src->temp, 8/*BLOCK_FIELD_IS_BYREF*/);_Block_object_assign((void*)&dst->array, (void*)src->array, 8/*BLOCK_FIELD_IS_BYREF*/);_Block_object_assign((void*)&dst->object, (void*)src->object, 8/*BLOCK_FIELD_IS_BYREF*/);}

   // Block 用的 dispose，只有 Block 中使用 __block 变量时才会出现
   static void __main_block_dispose_0(struct __main_block_impl_0*src) {_Block_object_dispose((void*)src->fmt, 8/*BLOCK_FIELD_IS_BYREF*/);_Block_object_dispose((void*)src->val, 8/*BLOCK_FIELD_IS_BYREF*/);_Block_object_dispose((void*)src->temp, 8/*BLOCK_FIELD_IS_BYREF*/);_Block_object_dispose((void*)src->array, 8/*BLOCK_FIELD_IS_BYREF*/);_Block_object_dispose((void*)src->object, 8/*BLOCK_FIELD_IS_BYREF*/);}
   
   // 不变
   static struct __main_block_desc_0 {
     size_t reserved;
     size_t Block_size;
     void (*copy)(struct __main_block_impl_0*, struct __main_block_impl_0*);
     void (*dispose)(struct __main_block_impl_0*);
   } __main_block_desc_0_DATA = { 0, sizeof(struct __main_block_impl_0), __main_block_copy_0, __main_block_dispose_0};
   
   // 5 个 __block 变量的初始化：
   // fmt
   __attribute__((__blocks__(byref))) __Block_byref_fmt_0 fmt = {(void*)0,(__Block_byref_fmt_0 *)&fmt, 0, sizeof(__Block_byref_fmt_0), "val = %d\n"};
   
   // val
   __attribute__((__blocks__(byref))) __Block_byref_val_1 val = {(void*)0,(__Block_byref_val_1 *)&val, 0, sizeof(__Block_byref_val_1), 10};
   
   // temp
   __attribute__((__blocks__(byref))) __Block_byref_temp_2 temp = {(void*)0,(__Block_byref_temp_2 *)&temp, 0, sizeof(__Block_byref_temp_2), 20};
   
   // array
   __attribute__((__blocks__(byref))) __Block_byref_array_3 array = {(void*)0,(__Block_byref_array_3 *)&array, 33554432, sizeof(__Block_byref_array_3), __Block_byref_id_object_copy_131, __Block_byref_id_object_dispose_131, ((NSMutableArray * _Nonnull (*)(id, SEL))(void *)objc_msgSend)((id)objc_getClass("NSMutableArray"), sel_registerName("array"))};
   
   // object
   __attribute__((__blocks__(byref))) __Block_byref_object_4 object = {(void*)0,(__Block_byref_object_4 *)&object, 33554432, sizeof(__Block_byref_object_4), __Block_byref_id_object_copy_131, __Block_byref_id_object_dispose_131, ((NSObject *(*)(id, SEL))(void *)objc_msgSend)((id)((NSObject *(*)(id, SEL))(void *)objc_msgSend)((id)objc_getClass("NSObject"), sel_registerName("alloc")), sel_registerName("init"))};

   // array 和 object 里面有新东西
   // __Block_byref_id_object_copy_131
   // __Block_byref_id_object_dispose_131
   // 结构体里面定义是两个函数指针:
   // void (*__Block_byref_id_object_copy)(void*, void*);
   // void (*__Block_byref_id_object_dispose)(void*);
   
   // 在 110 行和 113 行找到了定义
   static void __Block_byref_id_object_copy_131(void *dst, void *src) {
    _Block_object_assign((char*)dst + 40, *(void * *) ((char*)src + 40), 131);
   }
   
   static void __Block_byref_id_object_dispose_131(void *src) {
    _Block_object_dispose(*(void * *) ((char*)src + 40), 131);
   }
   
   // 需要到 libclosure74 里面看源码
   // 看到里面调用了 _Block_object_assign 和 _Block_object_dispose
   // 这和上面的 __main_block_copy_0 和 __main_block_dispose_0 里面调用是是一样的函数
   // _Block_object_dispose 和 _Block_object_assign 函数。
   // 已知 Block 截获对象类型和使用 __block 变量时
   // 会添加 __main_block_copy_0 和 __main_block_dispose_0 函数。 
};

// 看到 68 行的定义:
// Runtime copy/destroy helper functions (from Block_private.h)
#ifdef __OBJC_EXPORT_BLOCKS
extern "C" __declspec(dllexport) void _Block_object_assign(void *, const void *, const int);
extern "C" __declspec(dllexport) void _Block_object_dispose(const void *, const int);
extern "C" __declspec(dllexport) void *_NSConcreteGlobalBlock[32];
extern "C" __declspec(dllexport) void *_NSConcreteStackBlock[32];
#else
__OBJC_RW_DLLIMPORT void _Block_object_assign(void *, const void *, const int);
__OBJC_RW_DLLIMPORT void _Block_object_dispose(const void *, const int);
__OBJC_RW_DLLIMPORT void *_NSConcreteGlobalBlock[32];
__OBJC_RW_DLLIMPORT void *_NSConcreteStackBlock[32];
#endif

```
OC 对象类型用 __block 修饰时的情况单独拿出来说一下:
```
// object
__attribute__((__blocks__(byref))) __Block_byref_object_4 object = {(void*)0,(__Block_byref_object_4 *)&object, 33554432, sizeof(__Block_byref_object_4), __Block_byref_id_object_copy_131, __Block_byref_id_object_dispose_131, ((NSObject *(*)(id, SEL))(void *)objc_msgSend)((id)((NSObject *(*)(id, SEL))(void *)objc_msgSend)((id)objc_getClass("NSObject"), sel_registerName("alloc")), sel_registerName("init"))};

// 简化后
__Block_byref_object_4 object = {
(void*)0, // isa
(__Block_byref_object_4 *)&object, // __forwarding
33554432, // __flags
sizeof(__Block_byref_object_4), // __size
__Block_byref_id_object_copy_131, // __Block_byref_id_object_copy
__Block_byref_id_object_dispose_131, // __Block_byref_id_object_dispose
((NSObject *(*)(id, SEL))(void *)objc_msgSend)((id)((NSObject *(*)(id, SEL))(void *)objc_msgSend)((id)objc_getClass("NSObject"), sel_registerName("alloc")), sel_registerName("init")) // obj
}
```
+ __flags = 33554432 即二进制的 10000000000000000000000000 即 1 << 25，BLOCK_HAS_COPY_DISPOSE = (1 << 25), // compiler 译：compiler 含有 copy_dispose 助手【即拥有 copy 和 dispose 函数】




## Block 存储域
&ensp;通过前面的研究可知，Block 转换为 Block 的结构体类型的自动变量，__block 变量转换为 __block 变量的结构体类型的自动变量。所谓结构体类型的自动变量，即**栈上生成的该结构体的实例**。如表:
|  名称  |  实质  |
|  -----  |  -----  |
| Block |  栈上 Block 的结构体实例  |
| __block 变量  | 栈上 __block 变量的结构体实例 |

通过之前的说明可知 **Block 也是 OC 对象**。将 Block 当作 OC 对象来看时，该 Block 的类为 `_NSConcreteStackBlock`。同时还有 `_NSConcreteGlobalBlock`、`_NSConcreteMallocBlock`。 由名称中含有 `stack` 可知，该类的对象 Block 设置在栈上。同样由 `global` 可知，与全局变量一样，设置在程序的数据区域（.data 区）中。`malloc` 设置在由 `malloc` 函数分配的内存块（即堆）中。
|类|设置对象的存储域|
|---|---|
|_NSConcreteStackBlock|栈|
|_NSConcreteGlobalBlock|程序的数据区域(.data 区)|
|_NSConcreteMallocBlock|堆|

应用程序的内存分配
1. 程序区域 .text 区
2. 数据区域 .data 区
3. 堆
4. 栈

**在记述全局变量的地方使用 Block 语法**时，生成的 Block 为 _NSConcreteGlobalBlock 类对象。

**isa 是靠 runtime 动态确定的，不能通过转换代码看出**

如下:
```
void (^blk)(void) = ^{ printf("全局区的 _NSConcreteGlobalBlock Block！\n"); };

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // insert code here...
        NSLog(@"🎉🎉🎉 Hello, World!");
        
        blk();
        
        NSLog(@"❄️❄️❄️ block isa: %@", blk);
    }
}

// 打印:
全局区的 _NSConcreteGlobalBlock Block！
❄️❄️❄️ block isa: <__NSGlobalBlock__: 0x100002068>

// 转换后:

// 命名都是以 __blk 开始的，对应该全局 Block 的名字 blk
// __blk_block_impl_0
struct __blk_block_impl_0 {
  struct __block_impl impl;
  struct __blk_block_desc_0* Desc;
  
  // 构造函数
  __blk_block_impl_0(void *fp, struct __blk_block_desc_0 *desc, int flags=0) {
    // isa 指向了 _NSConcreteGlobalBlock 
    impl.isa = &_NSConcreteGlobalBlock;
    impl.Flags = flags;
    impl.FuncPtr = fp;
    Desc = desc;
  }
};

// __blk_block_func_0
static void __blk_block_func_0(struct __blk_block_impl_0 *__cself) {
 printf("全局区的 _NSConcreteGlobalBlock Block！\n"); 
}

// __blk_block_desc_0
static struct __blk_block_desc_0 {
  size_t reserved;
  size_t Block_size;
} __blk_block_desc_0_DATA = { 0, sizeof(struct __blk_block_impl_0)};

// 创建 __blk_block_impl_0 实例
static __blk_block_impl_0 __global_blk_block_impl_0((void *)__blk_block_func_0, &__blk_block_desc_0_DATA);

// blk 
void (*blk)(void) = ((void (*)())&__global_blk_block_impl_0);

int main(int argc, const char * argv[]) {
    /* @autoreleasepool */ { __AtAutoreleasePool __autoreleasepool; 

        NSLog((NSString *)&__NSConstantStringImpl__var_folders_24_5w9yv8jx63bgfg69gvgclmm40000gn_T_main_b56a4a_mi_0);

        // 调用 blk
        ((void (*)(__block_impl *))((__block_impl *)blk)->FuncPtr)((__block_impl *)blk);
    }

    return 0;
}
```
此 Block 即该 Block 用结构体实例设置在程序的数据区域中。因为在使用全局变量的地方不能使用自动变量，所以不存在对自动变量进行截获。由此 Block 用结构体实例的内容不依赖于执行时的状态，所以整个程序中只需要一个实例。因此将 Block 用结构体实例设置在与全局变量相同的数据区域中即可。

&ensp;只在截获自动变量时，Block 用结构体实例截获的值才会根据执行时的状态变化。

**也就是说，即使在函数内而不在记述广域变量的地方使用 Block 语法时，只要 Block 不截获自动变量，就可以将 Block 用结构体实例设置在程序的数据区域。**
```
// 当前在 main 函数内:
// 不捕获外部自动变量
void (^globalBlock)(void) = ^{
    NSLog(@"❄️❄️❄️ 测试 block isa");
};

globalBlock();
NSLog(@"❄️❄️❄️ block isa: %@", globalBlock);

// 打印:
❄️❄️❄️ 测试 block isa
❄️❄️❄️ block isa: <__NSGlobalBlock__: 0x100002088>
```

**对于没有要截获自动变量的 block，我们不需要依赖于其运行时的状态【捕获的变量】，这样我们就不涉及到 block 的 copy 情况，因此是放在数据区。**

**此外要注意的是，通过 clang 编译出来的 isa 在第二种情况下会显示成 stackblock，这是因为 OC 是一门动态语言，真正的元类还是在运行的情况下确定的，这种情况下可以使用 lldb 调试器查看。**

**虽然通过 clang 转换的源代码通常是 _NSConcreteStackBlock 类对象，但实现上却有不同。总结如下:**

+ 记述全局变量的地方有 Block 语法时
+ Block 语法的表达式中不使用截获的自动变量时

以上情况下，Block 为 `_NSConcreteGlobalBlock` 类对象，即 Block 配置在程序的数据区域中。除此之外 Block 语法生成的 Block 为 _NSConcreteStackBlock 类对象，且设置在栈上。

```
// 不捕获外部自动变量是 global
void (^globalBlock)(void) = ^{
    NSLog(@"❄️❄️❄️ 测试 block isa");
};

int a = 2;
// 右边栈区 block 赋值给左侧 block 时，会被复制到堆区
void (^mallocBlock)(void) = ^{
    NSLog(@"❄️❄️❄️ 测试 block isa a = %d", a);
};

globalBlock();
mallocBlock();

NSLog(@"❄️❄️❄️ globalBlock isa: %@", globalBlock);
NSLog(@"❄️❄️❄️ mallocBlock isa: %@", mallocBlock);
// 栈区 block
NSLog(@"❄️❄️❄️ stackBlock isa: %@", ^{ NSLog(@"❄️❄️❄️ a = %d", a); });

// 打印：
❄️❄️❄️ 测试 block isa
❄️❄️❄️ 测试 block isa a = 2
❄️❄️❄️ globalBlock isa: <__NSGlobalBlock__: 0x100002088>
❄️❄️❄️ mallocBlock isa: <__NSMallocBlock__: 0x100540fa0>
❄️❄️❄️ stackBlock isa: <__NSStackBlock__: 0x7ffeefbff4e0>
```
 配置在全局变量上的 Block ，从变量作用域外也可以通过指针安全的使用，但设置在栈上的 Block，如果其所属的变量作用域结束，该 Block 就被废弃。由于 __Block 变量也配置在栈上，同样的，如果其所属的变量作用域结束，则该 __block 变量也会被废弃。
 
 示例代码:
 ```
 // block 不持有 object2
 void (^blk)(void);
 {
     NSObject *object = [[NSObject alloc] init];
     NSObject * __weak object2 = object;
     // 右边栈区 block 被复制到堆区
     // 弱引用 object2, 出了下面花括号，object 被释放废弃，object2 被置为 nil 
     blk = ^{
         NSLog(@"object2 = %@", object2);
     };
 }
 blk();
 //打印：
 object2 = (null)
 
 // block 持有 object
 void (^blk)(void);
 {
     NSObject *object = [[NSObject alloc] init];
     // NSObject * __weak object2 = object;
     // 出了花括号 object 依然存在，因为它被 blk 强引用
     blk = ^{
         NSLog(@"object = %@", object);
     };
 }
 blk();
 // 打印：
 object = <NSObject: 0x10059cee0>
 ```
 
 &ensp;Blocks 提供了将 Block 和 __block 变量从栈上复制到堆上的方法来解决这个问题。将配置在栈上的 Block 复制到堆上，这样即使 Block 语法记述的变量作用域结束，堆上的 Block 还可以继续存在。

+ 不会有任何一个块一上来就被存在堆区，请牢记这一点！
+ `_NSConcreteMallocBlock` 存在的意义和 `autorelease` 一样，就是为了能延长 block 的作用域
+ 我们将 block 对象和 __blcok 对象从栈区复制到堆区，这样就算栈上的 block 被废弃了，还是可以使用堆上那一个
+ 可以联想我们在 ARC 是如何处理返回值中的 __strong 的，大概同理

**在这里要思考一个问题：在栈上和堆上同时有一个 block 的情况下，我们的赋值，修改，废弃操作应该怎样管理？**

复制到堆上的 Block isa 会指向 _NSConcreteMallocBlock，即 impl.isa = &_NSConcreteMallocBlock;

**__block 变量用结构体成员变量 __forwarding 可以实现无论 __block 变量配置在栈上还是堆上时都能够正确地访问 __block 变量。**

**有时在 __block 变量配置在堆上的状态下，也可以访问栈上的 __block 变量。只要栈上的结构体实例成员变量 __forwarding 指向堆上的结构体实例，那么不管是从栈上的 __block 变量还是从堆上的 __block 变量都能够正确访问。**

// 代码示例：
```
// 这个 a 是位于栈区 __Block_byref_a_0 结构体实例，已经不是 int 类型
__block int a = 2;

// 下面 block 被复制到堆区，a 也同时被复制到 堆区
void (^mallocBlock)(void) = ^{
    // a->__forwarding->a 自增
    // 堆上 a 的 __forwarding 指向自己
    ++a;
    NSLog(@"❄️❄️❄️ 测试 block isa a = %d", a);
};

// 下面的 a 还是在栈区的 __Block_byref_a_0 结构体实例，
// 但是它的 __forwrding 指针是指向上面被复制堆区的 a 的，
// 这样不管是栈区 a 还是 堆区 a，当操作 int a = 2，这个数值 a 时都是同一个。
++a;
```

Blocks 提供的复制方法究竟是什么呢？实际上 ARC  有效时，大多数情形下编译器会恰当的进行判断，自动生成将 Block 从栈复制到堆上的代码。

> 碰到两个问题，都是用中间变量接一下就正常了：
```
// 问题一：
// 用 clang -rewrite-objc 能转换成功
typedef int(^BLK)(int);

BLK func(int rate) {
    // 右边栈区 block 复制到堆区，并被 temp 持有
    BLK temp = ^(int count){ return rate * count; };
    return temp;
}

// 用 clang -rewrite-objc 转换失败，改成上面就会成功，（用中间变量接收一下）
typedef int(^BLK)(int);

BLK func(int rate) {
    // 此时直接返回栈区 block 不行 
    return ^(int count){ return rate * count; };
}

// 失败描述，转换失败，但是执行该函数是正常的
returning block that lives on the local stack
return ^(int count){ return rate * count; };
           ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
64 warnings and 1 error generated.

// 问题二:
BLK __weak blk;
{
    NSObject *object = [[NSObject alloc] init];
    
    // NSObject * __weak object2 = object;
    
    void (^strongBlk)(void) = ^{
        NSLog(@"object = %@", object);
    };
    
    // blk 是一个弱引用变量，用一个 strong 赋值给他，
    // 它不持有该 strong 变量
    blk = strongBlk;
}

// blk();
printf("blk = %p\n", blk);
// 打印正常，出了花括号，block 结构体实例释放了:
blk = 0x0

BLK __weak blk;
{
    NSObject *object = [[NSObject alloc] init];
    // NSObject * __weak object2 = object;
    // void (^strongBlk)(void) = ^{
    // NSLog(@"object = %@", object);
    // };

    // 这里给了警告: 
    // Assigning block literal to a weak variable; object will be released after assignment
    blk = ^{
        NSLog(@"object = %@", object);
    };
    printf("内部 blk = %p\n", blk);
}

// blk();
printf("blk = %p\n", blk);
// 打印：出了花括号，还是打印了 blk 不为 0x0
内部 blk = 0x7ffeefbff538
blk = 0x7ffeefbff538
```
看一下下面这个返回 Block 的函数:
```
typedef int (^blk_t)(int);
blk_t func(int rate) {
    return ^(int count) {
        return rate * count;
    };
}
```
源代码为返回配置在栈上的 Block 的函数。即程序执行中从 **该函数** 返回 **函数调用方** 时变量作用域结束，因此栈上的 Block 也被废弃。虽然有这样的问题，但是该源代码通过对应 ARC 的编译器可转换为如下:
```
blk_t func(int rate) {
blk_t tmp = &__func_block_impl_0(__func_block_func_0, &__func_block_desc_0_DATA, rate);

tmp = objc_retainBlock(tmp);

return objc_autoreleaseReturnValue(tmp);
}
```
另外，因为 ARC 处于有效状态，所以 `blk_t tmp` 实际上与附有 `__strong` 修饰符的 `blk_t __strong tmp` 相同。
在 objc4 找到  `objc_retainBlock` 函数实际上就是 `Block_copy` 函数:
```
// 在 NSObject.mm 文件 31 行
//
// The -fobjc-arc flag causes the compiler to issue calls to objc_{retain/release/autorelease/retain_block}
//

id objc_retainBlock(id x) {
    return (id)_Block_copy(x);
}

// usr/include/Block.h 中找到
// Create a heap based copy of a Block or simply add a reference to an existing one.
// 创建基于堆的 Block 副本，或仅添加对现有 Block 的引用。（已经在堆上的 block 调用 copy 函数，引用计数增加）
// This must be paired with Block_release to recover memory, even when running under Objective-C Garbage Collection.
// 如果在 OC 的垃圾回收机制下使用时必须与 "Block_release" 配对使用。

BLOCK_EXPORT void *_Block_copy(const void *aBlock)
    __OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);

```
即:
```
tmp = _Block_copy(tmp);
return objc_autoreleaseReturnValue(tmp);
```
分析:
```
// 第一步，__func_block_impl_0 结构体实例生成
// 将通过 Block 语法生成的 Block， 
// 即配置在栈上的 Block 用结构体实例赋值给相当于 Block 类型的变量 tmp 中
blk_t tmp = &__func_block_impl_0(__func_block_func_0, &__func_block_desc_0_DATA, rate);

// 第二步，_Block_copy 函数执行
// _Block_copy 函数，将栈上的 Block 复制到堆上。
// 复制后，将堆上的地址作为指针赋值给变量 tmp。
tmp = _Block_copy(tmp);

// 第三步，将堆上的 Block 作为 OC 对象，
// 注册到 autoreleasepool 中，然后返回该对象
return objc_autoreleaseReturnValue(tmp);
```
**将 Block 作为函数返回值返回时，编译器会自动生成复制到堆上的代码。**

前面说大部分情况下编译器会适当的进行判断，不过在此之外的情况下需要**手动**（自己调用 copy 函数）生成代码，将 Block 从栈上复制到 **堆** （_Block_copy 函数的注释已经说了，它是创建基于堆的 block 副本）上，即我们自己主动调用 “copy” 实例方法。

**编译器不能进行判断究竟是什么样的状况呢？**

+ 向方法或函数的参数中传递 Block 时。
但是如果在方法或函数 **中** 适当的复制了传递过来的参数，那么就不必在调用该方法或函数前手动复制了。

以下方法或函数不用手动复制，编译器会给我们自动复制:

+ Cocoa 框架的方法且方法名中含有 usingBlock 等时
+ Grand Central Dispatch 的 API
+ 将 Block 赋值给类的附有 __strong 修饰符的 id 类型或 Block 类型成员变量时【当然这种情况就是最多的，只要赋值一个block变量就会进行复制】

NSArray 的 enumerateObjectsUsingBlock 以及 dispatch_async 函数就不用手动复制。
NSArray 的 initWithObjects 上传递 Block 时需要手动复制。

下面是个🌰：
```
id obj = [Son getBlockArray];
void (^blk)(void) = [obj objectAtIndex:0];
blk();

// 对 block 主动调用 copy 函数，能正常运行 
+ (id)getBlockArray {
    int val = 10;
    return [[NSArray alloc] initWithObjects:[^{NSLog(@"blk0: %d", val);} copy], [^{NSLog(@"blk1: %d", val);} copy], nil];
}

// 如下如果不加 copy 函数，则运行崩溃
+ (id)getBlockArray {
    int val = 10;
    return [[NSArray alloc] initWithObjects:^{NSLog(@"blk0: %d", val);}, ^{NSLog(@"blk1: %d", val);}, nil];
}

// 崩溃原因: 不主动调用 copy，getBlockArray 函数执行结束，栈上的 block 被废弃了
// 编译器对此种情况不能判断是否需要复制。
// 也可以不判断全部情况都复制，但是将 Block 从栈复制到堆是相当消耗 CPU 的。
// 当 Block 在栈上也能使用时，从栈上复制到堆上，就只是浪费 CPU 资源了。
// 此时需要我们判断，自行手动复制。
```
|Block 的类|副本源的配置存储域|复制效果|
|---|---|---|
|_NSConcreteStackBlock|栈|从栈复制到堆|
|_NSConcreteGlobalBlock|程序的数据区域|什么也不做|
|_NSConcreteMallocBlock|堆|引用计数增加|
不管 Block 配置在何处，用 copy 方法复制都不会引起任何问题，在不确定时调用 copy 方法即可。

## __block 变量存储域
使用 __block 变量的 Block 从栈复制到堆上时，__block 变量也会受到影响。
|__block 变量的配置存储域|Block 从栈复制到堆时的影响|
|---|---|
|栈|从栈复制到堆并被 Block 持有|
|堆|被 Block 持有|

若在一个 Block 中使用 __block 变量，当该 Block 从栈复制到堆时，使用的所有 __block 变量也必定配置在栈上，这些 __block 变量也全部被从栈复制到堆。此时，Block 持有 __block 变量，即使在该 Block 已复制到堆的情形下，复制 Block 也对所使用的 __block 变量没有任何影响。

**使用 __block 变量的 Block 持有 __block 变量。如果 Block被废弃，它所持有的 __block 变量也就被释放。**

回顾 __block 变量用结构体成员变量 __forwarding 的原因：**不管 __block 变量配置在栈上还是在堆上，都能够正确的访问该变量。**
通过 Block 的复制，__block 变量也从栈上复制到堆上。此时可同时访问栈上的 __block 变量和堆上的 __block 变量。

源代码如下:
```
__block int val = 0;

// 使用 copy 方法复制了使用 __block 变量的 Block 语法
// Block 和 __block 变量两者均从栈复制到堆 
// 在 Block 语法的表达式中使用初始化后的 __block 变量，做了自增运算
void (^blk)(void) = [^{++val;} copy];

// 在 Block 语法之后使用与 Block 无关的变量，
// 此时的 val 是第一行生成的 __block 变量，
// Block 语法表达式中使用的 val 是 Block 结构体自己的成员变量 val
// 两者之间毫无瓜葛，硬要说有关系的话，大概就是 Block 表达式里面的 val（指针）成员变量
// 在 Block 结构体初始化时初始化列表里面 val 初始化是用的:val(_val->__forwarding) { }

++val;

// 通过 clang 转换，看到两次自增运算均转换为如下形式:

// Block 表达式内部：
// 首先找到 Block 结构体实例的成员变量 val 
__Block_byref_val_0 *val = __cself->val; // bound by ref
// val 是结构体 __Block_byref_val_0 指针
++(val->__forwarding->val);

// 外部：
++(val.__forwarding->val);

blk();
// 且此行打印语句也是用的 val.__forwarding->val
NSLog(@"val = %d", val);
```

在变换 Block 语法的函数中，该变量 val 为复制到堆上的 __block 变量用结构体实例，而使用的与 Block 无关的变量 val，为复制前栈上的 __block 变量用结构体实例。

**超级重要的一句：**
**但是栈上的 __block 变量用结构体实例在 __block 变量从栈复制到堆上时，会将成员变量 __forwarding 的值替换为复制目标堆上的 __block 变量用结构体实例的地址**。

至此，无论是在 Block 语法中、Block 语法外使用 __block 变量，还是 __block 变量配置在栈上或堆上，都可以顺利的访问到同一个 __block 变量。

**所有使用 val 的地方实际都转化为了: val->__forwarding->val（block 内部）或者 val.__forwarding->val（外部，是结构体实例可以直接使用 .）。**

## 截获对象
示例代码:
```
void (^blk)(id);
{
    // id array = [[NSMutableArray alloc] init];
    id array = [NSMutableArray array];
    
    // 注意最后 block 调用了 copy 函数
    // 即使不调用也能正常执行
    // 原书是调用了 copy
    // 实际当右边的栈区赋值给左侧 blk 变量时，
    // 已经发生了 block 复制到堆区
    blk = [^(id obj){
        [array addObject:obj];
        
        NSLog(@"array count = %ld", [array count]);
    } copy];
}

blk([[NSObject alloc] init]);
blk([[NSObject alloc] init]);
blk([[NSObject alloc] init]);
// 打印：
array count = 1
array count = 2
array count = 3
```
意味着赋值给变量 array 的 NSMutableArray 类的对象在该源代码最后 Block 的执行部分超出其变量作用域而存在。

clang 转换后:
`_main_block_impl_0`
```
struct __main_block_impl_0 {
  struct __block_impl impl;
  struct __main_block_desc_0* Desc;
  // id 类型的 array 成员变量，截获外部 array 自动变量
  id array;
  
  // 构造函数 初始化列表 : array(_array)
  __main_block_impl_0(void *fp, struct __main_block_desc_0 *desc, id _array, int flags=0) : array(_array) {
    impl.isa = &_NSConcreteStackBlock;
    impl.Flags = flags;
    impl.FuncPtr = fp;
    Desc = desc;
  }
};
```
`__main_block_func_0`
```
static void __main_block_func_0(struct __main_block_impl_0 *__cself, id obj) {
// 取到 block 结构体实例的成员变量 array
id array = __cself->array; // bound by copy

// 调用 addObject 函数，参数是 id obj 
((void (*)(id, SEL, ObjectType _Nonnull))(void *)objc_msgSend)((id)array, sel_registerName("addObject:"), (id)obj);

// 打印 count
NSLog((NSString *)&__NSConstantStringImpl__var_folders_24_5w9yv8jx63bgfg69gvgclmm40000gn_T_main_b7ba78_mi_1, ((NSUInteger (*)(id, SEL))(void *)objc_msgSend)((id)array, sel_registerName("count")));
}
```

```
// __main_block_copy_0
// 3/*BLOCK_FIELD_IS_OBJECT*/ 这里表示捕获的是对象
// 如果是 block 截获 __block 变量时，
// 会是 8/*BLOCK_FIELD_IS_BYREF*/ 这里表示捕获的是引用

static void __main_block_copy_0(struct __main_block_impl_0*dst, struct __main_block_impl_0*src) {_Block_object_assign((void*)&dst->array, (void*)src->array, 3/*BLOCK_FIELD_IS_OBJECT*/);}

// __main_block_dispose_0
// 3/*BLOCK_FIELD_IS_OBJECT*/ 这里表示捕获的是对象
// 如果是 block 截获 __block 变量时，
// 会是 8/*BLOCK_FIELD_IS_BYREF*/ 这里表示捕获的是引用

static void __main_block_dispose_0(struct __main_block_impl_0*src) {_Block_object_dispose((void*)src->array, 3/*BLOCK_FIELD_IS_OBJECT*/);}

static struct __main_block_desc_0 {
  size_t reserved;
  size_t Block_size;
  void (*copy)(struct __main_block_impl_0*, struct __main_block_impl_0*);
  void (*dispose)(struct __main_block_impl_0*);
} __main_block_desc_0_DATA = { 0, sizeof(struct __main_block_impl_0), __main_block_copy_0, __main_block_dispose_0};
```
main 函数内:
```
void (*blk)(id);
{
    // 创建 array 对象
    id array = ((NSMutableArray * _Nonnull (*)(id, SEL))(void *)objc_msgSend)((id)objc_getClass("NSMutableArray"), sel_registerName("array"));
    
    // block 定义，创建 block 结构体实例时，传入 array
    blk = (void (*)(id))((id (*)(id, SEL))(void *)objc_msgSend)((id)((void (*)(id))&__main_block_impl_0((void *)__main_block_func_0, &__main_block_desc_0_DATA, array, 570425344)), sel_registerName("copy"));
}

// 下面是三次 block 执行，（创建 object 对象）
((void (*)(__block_impl *, id))((__block_impl *)blk)->FuncPtr)((__block_impl *)blk, ((NSObject *(*)(id, SEL))(void *)objc_msgSend)((id)((NSObject *(*)(id, SEL))(void *)objc_msgSend)((id)objc_getClass("NSObject"), sel_registerName("alloc")), sel_registerName("init")));

((void (*)(__block_impl *, id))((__block_impl *)blk)->FuncPtr)((__block_impl *)blk, ((NSObject *(*)(id, SEL))(void *)objc_msgSend)((id)((NSObject *(*)(id, SEL))(void *)objc_msgSend)((id)objc_getClass("NSObject"), sel_registerName("alloc")), sel_registerName("init")));

((void (*)(__block_impl *, id))((__block_impl *)blk)->FuncPtr)((__block_impl *)blk, ((NSObject *(*)(id, SEL))(void *)objc_msgSend)((id)((NSObject *(*)(id, SEL))(void *)objc_msgSend)((id)objc_getClass("NSObject"), sel_registerName("alloc")), sel_registerName("init")));
```
1.3.4 节中，在 OC 中，C 语言结构体不能含有附有 __strong 修饰符的变量。因为编译器不知道应何时进行 C 语言结构体的初始化和废弃操作，不能很好地管理内存。
但是 OC  运行时库能准确的把握 Block 从栈复制到堆以及堆上的 Block 被废弃的时机，因此 Block 用结构体中即使含有附有 __strong 修饰符或者 __weak 修饰符的变量，也可以恰当的进行初始化和废弃。为此需要 __main_block_copy_0 和 __main_block_dispose_0 函数，并把他们放在了 __main_block_desc_0 结构体的成员变量 copy 和 dispose 中。
__main_block_copy_0 函数使用 `_Block_object_assign` 函数将对象类型对象赋值给 Block 用结构体的成员变量 array 中并持有该对象。
```
static void __main_block_copy_0(struct __main_block_impl_0*dst, struct __main_block_impl_0*src) {
_Block_object_assign(
(void*)&dst->array, // 参数 1
(void*)src->array, // 参数 2
3/*BLOCK_FIELD_IS_OBJECT*/); // 参数 3
}

static void __main_block_dispose_0(struct __main_block_impl_0*src) {
_Block_object_dispose(
(void*)src->array, // 参数 1
3/*BLOCK_FIELD_IS_OBJECT*/); // 参数 2
}

// usr/include/Block.h 中
// Used by the compiler. Do not call this function yourself.
BLOCK_EXPORT void _Block_object_assign(void *, const void *, const int)
    __OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);
    
    
    // Create a heap based copy of a Block or simply add a reference to an existing one.
    // This must be paired with Block_release to recover memory, even when running
    // under Objective-C Garbage Collection.
    BLOCK_EXPORT void *_Block_copy(const void *aBlock)
        __OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);

    // Lose the reference, and if heap based and last reference, recover the memory
    BLOCK_EXPORT void _Block_release(const void *aBlock)
        __OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);


    // Used by the compiler. Do not call this function yourself.
    BLOCK_EXPORT void _Block_object_assign(void *, const void *, const int)
        __OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);

    // Used by the compiler. Do not call this function yourself.
    BLOCK_EXPORT void _Block_object_dispose(const void *, const int)
        __OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);

    // Used by the compiler. Do not use these variables yourself.
    BLOCK_EXPORT void * _NSConcreteGlobalBlock[32]
        __OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);
    BLOCK_EXPORT void * _NSConcreteStackBlock[32]
        __OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);
        
        
        #define Block_copy(...) ((__typeof(__VA_ARGS__))_Block_copy((const void *)(__VA_ARGS__)))
        #define Block_release(...) _Block_release((const void *)(__VA_ARGS__))
```
_Block_object_assign 函数调用相当于 retain 实例方法的函数，将对象赋值在对象类型的结构体成员变量中。
__main_block_dispose_0 调用 _Block_object_dispose，释放赋值在 Block 用结构体成员变量 array 中的对象。
_Block_object_dispose 函数调用相当于 release 实例方法的函数，释放赋值在对象类型的结构体成员变量中的对象。

转换代码中 __main_block_desc_0 中的 copy 和 dispose 从没使用过，那什么时候会使用呢？
（这些方法都是编译器自己去调用的，我们不会主动调用它们。）
|函数|调用时机|
|---|---|
|copy 函数|栈上的 Block 复制到堆时|
|dispose 函数|堆上的 Block 被废弃时|

栈上 Block 复制到堆上时的情况:

+ 调用 Block 的 copy 实例方法时
+ Block 作为函数返回值返回时
+ 将 Block 赋值给附有 __strong 修饰符 id 类型的类或 Block 类型成员变量时
+ 在方法名中含有 usingBlock 的 Cocoa 框架方法或 Grand Central Dispatch 的 API 中传递 Block 时

这些情况下，编译器自动的将对象的 Block 作为参数并调用 _Block_copy 函数，这与调用 Block 的 copy 实例方法的效果相同。usingBlock 和 GCD 中传递 Block 时，在该方法或函数内部对传递过来的 Block 调用 Block 的 copy 实例方法或者 _Block_copy 函数。
看似从栈复制到堆上，其实可归结为 _Block_copy 函数被调用时 Block 从栈复制到堆。
相对，释放复制到堆上的 Block 后，谁都不持有 Block 而使其被废弃时调用 dispose 函数，这相当于对象的 dealloc 实例方法。

有了这些构造，通过使用附有 __strong 修饰符的自动变量，Block 中截获的对象就能够超出其作用域而存在。

在使用 __block 变量时，已经用到 copy 和 dispose 函数：
```
static void __main_block_copy_0(struct __main_block_impl_0*dst, struct __main_block_impl_0*src) {
_Block_object_assign(

(void*)&dst->val,
(void*)src->val,
8/*BLOCK_FIELD_IS_BYREF*/);

}

static void __main_block_dispose_0(struct __main_block_impl_0*src) {
_Block_object_dispose(

(void*)src->val,
8/*BLOCK_FIELD_IS_BYREF*/);

}
```
发现最后的参数有所不同:

截获对象时和使用 __block 变量时的不同：

| 对象 | BLOCK_FIELD_IS_OBJECT |
| __block 对象 | BLOCK_FIELD_IS_BYREF |

通过 BLOCK_FIELD_IS_OBJECT  和 BLOCK_FIELD_IS_BYREF 区分 copy 函数和 dispose 函数的对象类型是对象还是 __block 变量。

copy 函数持有截获的对象、dispose 函数释放截获的对象
copy 函数持有所使用的 __block 变量、dispose 函数释放所使用的 __block 变量

**Block中使用的赋值给附有__strong修饰符的自动变量的对象和复制到堆上的__block变量由于被堆上的Block所持有，因而可超出其变量作用域而存在。**

```
// 不调用 copy 也能正常执行，因为右边 block 赋值给右边时，已经赋值到了堆上
        void (^blk)(id);
        {
             id array = [[NSMutableArray alloc] init];
//            id array = [NSMutableArray array];
//            id __weak array2 = array;
            
            blk = ^(id obj){
                [array addObject:obj];
                
                NSLog(@"array count = %ld", [array count]);
            };
        }
        
        blk([[NSObject alloc] init]);
        blk([[NSObject alloc] init]);
        blk([[NSObject alloc] init]);
        
// 示例 2：
blk_t blk;
{
    id array = [[NSMutableArray alloc] init];
    // array = [NSMutableArray array];
    // __unsafe_unretained id array2 = array;
    NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);
    blk = ^(id obj){
        // id __strong array3 = array2;
        [array addObject:obj];
        NSLog(@"array count = %ld", [array count]);
        // NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);
    };

    NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);
}

blk([[NSObject alloc] init]);
blk([[NSObject alloc] init]);
blk([[NSObject alloc] init]);

// 打印：
⛈⛈⛈ array retainCount = 1
⛈⛈⛈ array retainCount = 3
array count = 1
array count = 2
array count = 3
```

## __block 变量和对象
__block 说明符可指定任何类型的自动变量。
```
// 
__block id obj = [[NSObject alloc] init];

// __Block_byref_obj_0
struct __Block_byref_obj_0 {
  void *__isa;
__Block_byref_obj_0 *__forwarding;
 int __flags;
 int __size;
 void (*__Block_byref_id_object_copy)(void*, void*);
 void (*__Block_byref_id_object_dispose)(void*);
 id obj;
};

// _Block_object_assign
static void __Block_byref_id_object_copy_131(void *dst, void *src) {
 _Block_object_assign(
 
 (char*)dst + 40,
 *(void * *) ((char*)src + 40),
 131
 
 );
}

// _Block_object_dispose
static void __Block_byref_id_object_dispose_131(void *src) {
 _Block_object_dispose(
 
 *(void * *) ((char*)src + 40),
 131
 
 );
}

// 
__attribute__((__blocks__(byref))) __Block_byref_obj_0 obj = {

(void*)0,
(__Block_byref_obj_0 *)&obj,
33554432,
sizeof(__Block_byref_obj_0),

__Block_byref_id_object_copy_131,
__Block_byref_id_object_dispose_131,

((NSObject *(*)(id, SEL))(void *)objc_msgSend)((id)((NSObject *(*)(id, SEL))(void *)objc_msgSend)((id)objc_getClass("NSObject"), sel_registerName("alloc")), sel_registerName("init"))

};
```
_Block_object_assign 和 _Block_object_dispose

```
// 当 array 用不同的创建方式时，
//（block 是否调用 copy 都不影响结果）：
// (array2 是否用 __block 不影响结果)
// 1. id array = [[NSMutableArray alloc] init];
blk_t blk;
{
     id array = [[NSMutableArray alloc] init];
    // id array = [NSMutableArray array];
    id __weak array2 = array;

    blk = [^(id obj){
        [array2 addObject:obj];

        NSLog(@"array count = %ld", [array2 count]);
    } copy];
}

blk([[NSObject alloc] init]);
blk([[NSObject alloc] init]);
blk([[NSObject alloc] init]);
// 打印:
array count = 0
array count = 0
array count = 0

// 2. id array = [NSMutableArray array];
blk_t blk;
{
    // id array = [[NSMutableArray alloc] init];
    id array = [NSMutableArray array];
    id __weak array2 = array;

    blk = [^(id obj){
        [array2 addObject:obj];

        NSLog(@"array count = %ld", [array2 count]);
    } copy];
}

blk([[NSObject alloc] init]);
blk([[NSObject alloc] init]);
blk([[NSObject alloc] init]);
// 打印:
array count = 1
array count = 2
array count = 3

// 3. 如果再加一层 @autoreleasepool {}
blk_t blk;
{
    // id array = [[NSMutableArray alloc] init];
    @autoreleasepool {
        
        id array = [NSMutableArray array];
        id __weak array2 = array;
        
        blk = [^(id obj){
            [array2 addObject:obj];
            
            NSLog(@"❄️❄️❄️ array count = %ld", [array2 count]);
        } copy];
        
    } // 出了这个花括号 array 就释放废弃了，同时 array2 也被置为 nil 了，所以打印 0 
    
}

blk([[NSObject alloc] init]);
blk([[NSObject alloc] init]);
blk([[NSObject alloc] init]);
// 打印:
❄️❄️❄️ array count = 0
❄️❄️❄️ array count = 0
❄️❄️❄️ array count = 0

// 4. 用 __unsafe_unretained 修饰
blk_t blk;
{
     id array = [[NSMutableArray alloc] init];
    // id array = [NSMutableArray array];
    id __unsafe_unretained array2 = array;

    blk = ^(id obj){
        [array2 addObject:obj];
        
        NSLog(@"array count = %ld", [array2 count]);
    };
}

blk([[NSObject alloc] init]);
blk([[NSObject alloc] init]);
blk([[NSObject alloc] init]);
// 崩溃，访问悬垂指针

// 5. array 用 [NSMutableArray array] 创建能正常运行
blk_t blk;
{
    // id array = [[NSMutableArray alloc] init];
    id array = [NSMutableArray array];
    id __unsafe_unretained array2 = array;

    blk = ^(id obj){
        [array2 addObject:obj];
        
        NSLog(@"array count = %ld", [array2 count]);
    };
}
// 即使出了花括号， array 还处于自动释放池，并没有被释放

blk([[NSObject alloc] init]);
blk([[NSObject alloc] init]);
blk([[NSObject alloc] init]);
// 打印:
array count = 1
array count = 2
array count = 3

// 6. 报错 Cannot capture __autoreleasing variable in a block
id __autoreleasing array2 = array;

// 7. 把 array 赋值给 __block id array2 则较复杂
 id array = [[NSMutableArray alloc] init];
 // array = [NSMutableArray array];
__block id array2 = array;

// 转换后：
 id array = ((NSMutableArray *(*)(id, SEL))(void *)objc_msgSend)((id)((NSMutableArray *(*)(id, SEL))(void *)objc_msgSend)((id)objc_getClass("NSMutableArray"), sel_registerName("alloc")), sel_registerName("init"));

// 看到 __Block_byref_array2_0 结构体初始化传入了 array 赋值给它的 array2 成员变量
__attribute__((__blocks__(byref))) __Block_byref_array2_0 array2 = 
{
(void*)0,
(__Block_byref_array2_0 *)&array2,
33554432,
sizeof(__Block_byref_array2_0),
__Block_byref_id_object_copy_131,
__Block_byref_id_object_dispose_131,
array
};

// 这个 Block 调用 copy 函数也有必要认真看一下
blk = 
(blk_t)((id (*)(id, SEL))(void *)objc_msgSend)((id)((void (*)(id))&__main_block_impl_0((void *)__main_block_func_0, &__main_block_desc_0_DATA, (__Block_byref_array2_0 *)&array2, 570425344)), sel_registerName("copy"));
```

## Block 循环引用
如果在 Block 中使用附有 strong 修饰符的对象类型自动变量，那么当 block 从栈复制到堆时，该对象为 block 所持有。不复制也会持有的，block 结构体初始化的时候已经将其捕获。
示例：
```
// 1. 
id array = [[NSMutableArray alloc] init];
{
    NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);
    
    ^(id obj) {
        [array addObject:obj];
        NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);
    };
    
    NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);
}
NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);

// 打印：
⛈⛈⛈ array retainCount = 1 // array 持有
⛈⛈⛈ array retainCount = 2 // array 和 栈上 block 同时持有
⛈⛈⛈ array retainCount = 1 // 出了花括号，栈上 block 释放，只剩下 array 持有

// 2.
id array = [[NSMutableArray alloc] init];
{
    NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);
    
    blk = ^(id obj) {
        [array addObject:obj];
        NSLog(@"⛈⛈⛈  Block array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);
    };
    
    NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);
}
NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);

if (blk != nil) {
    blk([[NSObject alloc] init]);
    blk([[NSObject alloc] init]);
    blk([[NSObject alloc] init]);
}
// 打印：
⛈⛈⛈ array retainCount = 1 // array 持有
⛈⛈⛈ array retainCount = 3 // 花括号内，栈上 block 持有、复制到堆的 block 持有、array 持有，总共是 3
⛈⛈⛈ array retainCount = 2 // 这里减 1 是栈上 block 出了花括号释放，同时也释放了 array，所以这里减 1
⛈⛈⛈  Block array retainCount = 2 // 这里 block 执行 3 次打印都是 2，此时 array 持有和堆上的 block blk 持有
⛈⛈⛈  Block array retainCount = 2
⛈⛈⛈  Block array retainCount = 2

// 3.
id array = [[NSMutableArray alloc] init];
{
    NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);
    
    blk = ^(id obj) {
        [array addObject:obj];
        NSLog(@"⛈⛈⛈  Block array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);
    };
    
    NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);
}

NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);

if (blk != nil) {
    blk([[NSObject alloc] init]);
    blk([[NSObject alloc] init]);
    blk([[NSObject alloc] init]);
}

blk = nil;
NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);
// 打印：
⛈⛈⛈ array retainCount = 1
⛈⛈⛈ array retainCount = 3
⛈⛈⛈ array retainCount = 2
⛈⛈⛈  Block array retainCount = 2
⛈⛈⛈  Block array retainCount = 2
⛈⛈⛈  Block array retainCount = 2
⛈⛈⛈ array retainCount = 1 // 只有这里，blk 三次执行完毕后，blk 赋值 空，blk 释放，同时释放 array，所以还剩下 array 持有，retainCount 为 1

// 4.
{
    id array = [[NSMutableArray alloc] init];
    NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);
    
    blk = ^(id obj) {
        [array addObject:obj];
        NSLog(@"⛈⛈⛈  Block array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);
    };
    
    NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);
}

// NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);

if (blk != nil) {
    blk([[NSObject alloc] init]);
    blk([[NSObject alloc] init]);
    blk([[NSObject alloc] init]);
}
// 打印：
⛈⛈⛈ array retainCount = 1 // 对象创建时为 1
⛈⛈⛈ array retainCount = 3 // 栈上 block 持有和复制到堆时堆上 block 持有 
                             // 出了花括号以后，栈上 block 释放，array 局部变量释放
                             // 剩下的 1 是堆上的 block 持有的
                             // 所以下面 block 指向时，打印都是 1
⛈⛈⛈  Block array retainCount = 1 // 出了花括号以后变量 array 释放，还剩下 block blk 自己持有，所以打印 1
⛈⛈⛈  Block array retainCount = 1
⛈⛈⛈  Block array retainCount = 1
```
```
- (id)init {
    self = [super init];
    blk_ = ^{ NSLog(@"self = %@", self);};
    return self;
}

// 依然会捕获 self,对编译器而言，obj_ 只不过是对象用结构体的成员变量。
// blk_ = ^{ NSLog(@"obj_ = %@", self->obj_); };

- (id)init {
    self = [super init];
    blk_ = ^{
        NSLog(@"obj_ = %@", obj_);
        };
        
    return self;    
}

// 除了 __weak self 也可用:
id __weak obj = obj_;
blk_ = ^{ NSLog(@"obj_ = %@", obj); };
```
该源代码中，由于 Block 存在时，持有该 Block 的 Object 对象即赋值在变量 tmp 中的self 必定存在，因此不需要判断变量 tmp 的值是否为 nil。
在 iOS 4 和 OS X 10.6 中，可以用 _unsafe_unretained 代替 __weak 修饰符，此处即可代替，且不必担心悬垂指针。

**由于Block语法赋值在了成员变量 blk_ 中，因此通过 Block 语法生成在栈上的 Block 此时由栈复制到堆上，并持有所使用的 self.**

在为避免循环引用而使用 __weak 修饰符时，虽说可以确认使用附有 __weak 修饰符的变量时，是否为 nil，但更有必要使之生存，以使用赋值给附有 __weak 修饰符变量的对象。（意思就比如上面，block 表达式开始执行时，首先判断 self 是否是 nil，如果不是 nil 才有必要继续往下执行，在往下执行的过程中并且希望 self 一直存在，不要正在使用时，竟被释放了，如果是单线程则无需考虑，但是在多线程开发时一定要考虑到这一点。）

**在 Block 里面加 __strong 修饰 weakSelf 取得 strongSelf，防止 block 结构体实例的 self 成员变量过早释放。Block 从外界所捕获的对象和在 Block 内部使用 __strong 强引用的对象，差别就在于一个是在 定义的时候 就会影响对象的引用计数, 一个是在 Block 运行的时候才强引用对象，且 block 表达式执行完毕还是会 -1**

**__weak 修饰的对象被 Block 引用，不会影响对象的释放，而 __strong 在 Block 内部修饰的对象，会保证，在使用这个对象在 scope 内，这个对象都不会被释放，出了 scope，引用计数就会 -1，且 __strong 主要是用在多线程运用中，如果只使用单线程，则只需要使用 __weak 即可。**

用 __block 变量来避免循环引用，原理是在 Block 内部对捕获的变量赋值为 nil，硬性破除引用环。
```
- (id)init {
    self = [super init];
    __block id tmp = self;
    blk_ = ^{
        NSLog(@"self = %@", tmp);
        tmp = nil;
    };
}
```

**对使用 __block 变量避免循环引用的方法和使用 __weak 修饰符及 __unsafe_unretained 修饰符避免循环引用的方法做比较:**

__block 优点：
+ 通过 __block 变量可控制对象的持有期间。
+ 在不能使用 __weak 修饰符的环境中不使用 __unsafe_unretained 修饰符即可（不必担心访问悬垂指针）
+ 在执行 Block 时可动态决定是否将 nil 或其他对象赋值在 __block  变量中。

__block 缺点:
+ 为避免循环引用必须执行 Block。

## copy/release
ARC  无效时，一般需要手动将 Block 从栈复制到堆，另外，由于 ARC 无效，所以肯定要手动释放复制的 Block。此时可用 copy 实例方法来复制，用 release 实例方法来释放。

```
void (^blk_on_heap)(void) = [blk_on_stack copy];
[blk_on_heap release];
```

只要 Block 有一次 **复制并配置在堆上**，就可通过 **retain 实例方法** 持有。

```
[blk_on_heap retain];
```
但是对于 **配置在栈上的 Block  调用 retain 实例方法则不起作用**。
```
[blk_on_stack retain];
```
该源代码中，虽然对赋值给 blk_on_stack 的栈上的 Block 调用了 retain 实例方法，**但实际上对此源代码不起任何作用**。因此**推荐使用 copy 实例方法来持有 Block**。

另外，由于 Blocks 是 C 语言的扩展，所以在 C 语言中也可以使用 Block 语法。此时使用 “Block_copy 函数” 和 “Block_release 函数” 代替 copy/release 实例方法。使用方法以及引用计数的思考方式与 OC 中的 copy/release 实例方法相同。
```
// 把栈上的 block 复制到堆上
void (^blk_on_heap)(void) = Block_copy(blk_on_stack);
// 释放堆上的 block
Block_release(blk_on_heap);
```
Block_copy 函数就是之前出现过的 _Block_copy 函数，即 OC  运行时库所使用的为 C 语言而准备的函数。释放堆上的 Block 时也同样调用 OC 运行时库的 Block_release 函数。

另外极其重要的一个知识点:
另外极其重要的一个知识点:
另外极其重要的一个知识点:

**ARC 无效时，__Block 说明符被用来避免 Block 中的循环引用，这是由于当 Block 从栈复制到堆时，若 Block 使用的变量为附有 __block 说明符的 id 类型或对象类型的自动变量，不会被 retain；若 Block 使用变量为没有 __block 说明符的 id 类型或对象类型的自动变量，则被 retain。**

由于 ARC 有效时和无效时 __block 说明符的用途有很大区别，因此编写源代码时，必须知道源代码是在 ARC 有效情况下编译还是无效情况下编译。

# Block 部分 完结撒花 🎉🎉🎉 感谢陪伴 🎉🎉🎉

