//
//  main.m
//  Block
//
//  Created by CHM on 2020/8/6.
//  Copyright © 2020 CHM. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "NSObject+ArcDebugRetainCount.h"

//#import "Son.h"
//#import "Father.h"
//#import "GrandSon.h"

//typedef void(^Blk_T)(int);
//
//// block 定义(匿名)
//int i = 10;
//Blk_T a = ^void (int event) {
//    printf("buttonID: %d event = %d\n", i, event);
//};
//// 函数定义
//void Func(int event) {
//    printf("buttonID: %d event = %d\n", i, event);
//};
//void (*funcPtr)(int) = &Func;

// Block 结构体命名测试
//int test() {
//    @autoreleasepool {
//        void (^block1)(void) = ^{
//            printf("测试 Block1 结构体名字\n");
//        };
//
//        block1();
//
//        void (^block2)(void) = ^{
//            printf("测试 Block2 结构体名字\n");
//        };
//
//        block2();
//    }
//
//    return 0;
//}

//int global_val = 1;
//static int static_global_val = 2;
//
//typedef void(^BLK)(void);
//BLK blk;

//void (^blk)(void) = ^{ printf("全局区的 _NSConcreteGlobalBlock Block！global_val = %d static_global_val = %d \n", global_val, static_global_val); };

//typedef void(^BLK)(void);

//typedef int(^BLK)(int);
//typedef int(^BLK2)(int);
//
//BLK myFuncTEST(BLK2 block) {
//    BLK temp = ^(int count){ return block(10) * count; };
//    return temp;
//
////    return ^(int count){ return rate * count; };
////    return block();
//}

//typedef int (^blk_t)(int);
//blk_t func(int rate) {
//    return ^(int count) {
//        return rate * count;
//    };
//}

//typedef void(^blk_t)(id);

//blk_t blk = ^(id obj){
////    [array addObject:obj];
//
//    NSLog(@"⛈⛈⛈ array count = %@", obj);
//};

//int global_val = 1;
//static int static_global_val = 2;

// blk_t blk;

// void (^blk)(void) = ^{ printf("全局区的 _NSConcreteGlobalBlock Block！\n"); };

//NSLog(@"🔔🔔🔔 %@", ^{ printf("%c\n", a);});

void exampleA() {
    // ARC 和 MRC 下均为栈区 Block
//    char a = 'A';
//    ^{
//        printf("🔔🔔🔔 %c\n", a);
//    }();
    
    NSLog(@"🔔🔔🔔 %@", ^{ printf("🟪🟪🟪");});
}

void exampleB_addBlockToArray(NSMutableArray *array) {
    char b = 'B';
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

void exampleC_addBlockToArray(NSMutableArray *array) {
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

typedef void(^dBlock)();
dBlock exampleD_getBlock() {
    char d = 'D';
    return [^{
        printf("🔔🔔🔔 %c\n", d);
    } copy];
}

void exampleD() {
    NSLog(@"🔔🔔🔔 %@", exampleD_getBlock());
    exampleD_getBlock()();
}

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
    NSLog(@"🔔🔔🔔 %@", exampleE_getBlock());
    eBlock block = exampleE_getBlock();
    NSLog(@"🔔🔔🔔 %@", block);
    block();
}

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // insert code here...
        NSLog(@"🎉🎉🎉 Hello, World!");
        
//        exampleA();
//        exampleB();
//        exampleC();
//        exampleD();
//        exampleE();
        
//        int temp = myFuncTEST(^int(int temp) {
//            return temp * 2;
//        })(2);
//
////        int temp = func(10)(30);
//        printf("temp = %d\n", temp);
        
//        blk();
        
//        BLK __weak blk;
//        {
//            NSObject *object = [[NSObject alloc] init];
//            // NSObject * __weak object2 = object;
//             void (^strongBlk)(void) = ^{
//             NSLog(@"object = %@", object);
//             };
//
////            blk = ^{
////                NSLog(@"object = %@", object);
////            };
//
////            printf("内部 blk = %p\n", blk);
//
//            blk = strongBlk;
//        }
//
//        // blk();
//        printf("blk = %p\n", blk);
        
//        const char* fmt = "val = %d\n";
//        int val = 10;
//        int temp = 20;
//        static int static_int = 55;
//        NSMutableArray *array = [NSMutableArray array];
//        NSObject *object = [[NSObject alloc] init];
//
//        void (^blk)(void) = ^{
////            fmt = "FMT val = %d\n";
//            printf(fmt, val);
//            printf("temp = %d", temp);
//            printf("static = %d", static_int);
//            [array addObject:object];
//        };
//
////        void (^blk2)(void) = ^{
//////            val = 50;
////            printf(fmt, val);
////        };
//
////        blk2();
//        val = 60;
//        blk();
        
//        {
//            int a = 10;
//            int* val = &a;
//
//            NSMutableArray *array = [NSMutableArray array];
//            NSObject *obj = [[NSObject alloc] init];
//            [array addObject:obj];
//
//            __weak NSMutableArray  *array2 = array;
//
//            blk = ^{
////                *val = 50;
////                printf("val = %d\n", *val);
//                NSLog(@"%@", array2);
//            };
//
////            a = 30;
//        }
//
//        blk();
        
//        // 示例 1：
//        int dmy = 256;
//        int val = 10;
//        int* valPtr = &val;
//        const char* fmt = "val = %d\n";
//
//        void (^blk)(void) = ^{
//            printf(fmt, val);
//            printf("valPtr = %d\n", *valPtr);
//        };
//
//        val = 2;
//        fmt = "These values were changed. val = %d\n";
//
//        blk();
        
//        // 运行结果：val = 10

//         示例 2：
//        int dmy = 256;
//        int temp = 10;
//        int* val = &temp;
//
//        printf("🎉🎉 val 初始值：= %d\n", *val);
//
//        const char* fmt = "🎉 Block 内部：val = %d\n";
//        void (^blk)(void) = ^{
//            *val = 22;
//            printf(fmt, *val);
//        };
//
//        *val = 20; // 修改 val
//        fmt = "These values were changed. val = %d\n";
//
//        blk();
//
//        printf("🎉🎉 val = %d\n", *val); // block 执行时把 *val 修改为 22
        
//         运行结果：val = 20

//        // 示例 3：
//        int dmy = 256;
//        __block int val = 10;
//        const char* fmt = "val = %d\n";
//        void (^blk)(void) = ^{
//            printf(fmt, val);
//        };
//
//        val = 2;
//        fmt = "These values were changed. val = %d\n";
//
//        blk();
//        // 运行结果：val = 2
        
//        void (^blk)(void) = ^ {
//            printf("Block 内部打印\n");
//        };
//
//        blk();
        
//        Son *son = [[Son alloc] init];
//        [son run];
        
//        [Son ClassRun];
        
//        GrandSon* grandSon = [[GrandSon alloc] init];
//        [grandSon run];
        
//        [GrandSon TEST];
        
//        [son eat];
        
//        Father *father = [[Father alloc] init];
//        [father run];
        
//        __block int m_nVal = 222;
//        __block NSMutableArray *m_pArray = [NSMutableArray array];
//        NSLog(@"m_pArray %@", [m_pArray class]);
//
//        static int static_val = 3;
//        void (^blk)(void) = ^{
//            global_val *= 2;
//            static_global_val *= 2;
//            static_val *= 3;
//        };
//
//        static_val = 12;
//
//        blk();
//
//        static_val = 111;
//
//        printf("static_val = %d, global_val = %d, static_global_val = %d\n", static_val, global_val, static_global_val);
        
//        id obj = [Son getBlockArray];
//        void (^blk)(void) = [obj objectAtIndex:0];
//        blk();
        
//        __block int val = 0;
//        void (^blk)(void) = [^{++val;} copy];
//        ++val;
//        blk();
//        NSLog(@"val = %d", val);
        
//        blk_t blk;
//
//        id array = [[NSMutableArray alloc] init];
//        {
//            NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);
//
//            blk = ^(id obj) {
//                [array addObject:obj];
//                NSLog(@"⛈⛈⛈  Block array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);
//            };
//
//            NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);
//        }
//
//        NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);
//
//        if (blk != nil) {
//            blk([[NSObject alloc] init]);
//            blk([[NSObject alloc] init]);
//            blk([[NSObject alloc] init]);
//        }
//
//        blk = nil;
//        NSLog(@"⛈⛈⛈ array retainCount = %lu", (unsigned long)[array arcDebugRetainCount]);
        
//        id object = [[NSObject alloc] init];
//        __weak id weakObj = object;
//        NSLog(@"⛈⛈⛈ object retainCount = %lu", (unsigned long)[object arcDebugRetainCount]);
////        blk = ^(){
////            NSLog(@"⛈⛈⛈ object = %@", weakObj);
////
////            if (weakObj == nil) {
////                return ;
////            }
////
////            dispatch_async(dispatch_queue_create(DISPATCH_QUEUE_PRIORITY_DEFAULT, NULL), ^{
////
//////                for (int i = 0; i < 1000000; ++i) {
//////                    // 模拟耗时任务
//////                }
////
////                NSLog(@"⛈⛈⛈ 耗时的任务 结束! object = %@", weakObj);
////            });
////        };
//
//
//                    dispatch_async(dispatch_queue_create(DISPATCH_QUEUE_PRIORITY_DEFAULT, NULL), ^{
//
//        //                for (int i = 0; i < 1000000; ++i) {
//        //                    // 模拟耗时任务
//        //                }
//
//                        NSLog(@"⛈⛈⛈ 耗时的任务 结束! object = %@", weakObj);
//                    });
//
//        NSLog(@"⛈⛈⛈ after block object retainCount = %lu", (unsigned long)[object arcDebugRetainCount]);
        
//        {
//            id array = [[NSMutableArray alloc] init];
//            blk = ^(id obj){
//                [array addObject:obj];
//
//                NSLog(@"⛈⛈⛈ array count = %@", array);
//            };
//        }
//
//        blk([[NSObject alloc] init]);
//        blk([[NSObject alloc] init]);
//        blk([[NSObject alloc] init]);
        
////        {
////            id array = [[NSMutableArray alloc] init];
//           blk = [^(id obj){
////                [array addObject:obj];
//
////                NSLog(@"⛈⛈⛈ array count = %@", array);
//               NSLog(@"⛈⛈⛈ obj = %@", obj);
//            } copy];
//
//
////        }
//
//        blk([[NSObject alloc] init]);
//        blk([[NSObject alloc] init]);
//        blk([[NSObject alloc] init]);
//
////        blk();
        
//        int a = 10;
//        __block int* b = &a;
//        void (^blk)(void) = ^{
//            NSLog(@"⛈⛈⛈ block 内部 b 修改前: b = %d", *b);
//            *b = 20; // 修改后外部 b 也是 20
//        };
//
//        NSLog(@"⛈⛈⛈ b = %d", *b);
//        a = 30;
//        NSLog(@"⛈⛈⛈ b = %d", *b);
//
//        blk();
//
//        NSLog(@"⛈⛈⛈ b = %d", *b);
//        NSLog(@"⛈⛈⛈ a = %d", a);
        
//        static int static_val = 3;
//        void (^blk)(void) = ^{
//           global_val *= 2;
//           static_global_val *= 2;
//           static_val *= 3;
//        };
//
//        static_val = 12;
//        blk();
//
//        // static_val = 111;
//        printf("static_val = %d, global_val = %d, static_global_val = %d\n", static_val, global_val, static_global_val);
        
//        // 不捕获外部自动变量是 global
//        void (^globalBlock)(void) = ^{
//            NSLog(@"❄️❄️❄️ 测试 block isa");
//        };
//
//        // 这个 a 是位于栈区 __Block_byref_a_0 结构体实例，已经不是 int 类型
//        __block int a = 2;
//
//        // 下面 block 被复制到堆区，a 也同时被复制到 堆区
//        void (^mallocBlock)(void) = ^{
//            // a->__forwarding->a 自增
//            // 堆上 a 的 __forwarding 指向自己
//            ++a;
//            NSLog(@"❄️❄️❄️ 测试 block isa a = %d", a);
//        };
//
//        // 下面的 a 还是在栈区的 __Block_byref_a_0 结构体实例，
//        // 但是它的 __forwrding 指针是指向上面被复制堆区的 a 的，
//        // 这样不管是栈区 a 还是 堆区 a，当操作 int a = 2，这个数值 a 时都是同一个。
//        ++a;
//
////        globalBlock();
////        mallocBlock();
////
////        NSLog(@"❄️❄️❄️ globalBlock isa: %@", globalBlock);
////        NSLog(@"❄️❄️❄️ mallocBlock isa: %@", mallocBlock);
////        // 栈区 block,
////        NSLog(@"❄️❄️❄️ stackBlock isa: %@", ^{ NSLog(@"❄️❄️❄️ a = %d", a); });
        
//        blk_t blk;
//        {
//            // id array = [[NSMutableArray alloc] init];
//            @autoreleasepool {
//
//                id array = [NSMutableArray array];
//                id __weak array2 = array;
//
//                blk = [^(id obj){
//                    [array2 addObject:obj];
//
//                    NSLog(@"❄️❄️❄️ array count = %ld", [array2 count]);
//                } copy];
//
//            }
//
//            ^(id obj) {
//                NSLog(@"%@", obj);
//            };
//
//            ^(void) {
//                NSLog(@"%d", 12);
//            };
//
//
//        }
//
//        blk([[NSObject alloc] init]);
//        blk([[NSObject alloc] init]);
//        blk([[NSObject alloc] init]);
        
//        NSObject *is_object = [[NSObject alloc] init];
//        void (^is_block)() = ^{ NSLog(@"is_block 参数"); };
//        __block NSObject *is_byref = [[NSObject alloc] init];
//        NSObject *tt = [[NSObject alloc] init];
//        NSObject *pp = [[NSObject alloc] init];
//
//        __block __unsafe_unretained NSObject *is_weak = tt;
//        __unsafe_unretained NSObject *is_only_weak = pp;
//
//        NSLog(@"⛈⛈⛈ is_byref retainCount = %lu ---%p---%p", (unsigned long)[is_byref arcDebugRetainCount], is_byref, &is_byref); // 堆区 栈区
//
//        void (^aBlock)() = ^{
//            NSLog(@"⛈⛈⛈ is_object retainCount = %lu ---%p---%p", (unsigned long)[is_object arcDebugRetainCount], is_object, &is_object);
//            is_block();
//
//            NSLog(@"⛈⛈⛈ is_byref retainCount = %lu ---%p---%p", (unsigned long)[is_byref arcDebugRetainCount], is_byref, &is_byref);
//            NSLog(@"⛈⛈⛈ is_weak retainCount = %lu ---%p---%p", (unsigned long)[is_weak arcDebugRetainCount], is_weak, &is_weak);
//            NSLog(@"⛈⛈⛈ is_only_weak retainCount = %lu ---%p---%p", (unsigned long)[is_only_weak arcDebugRetainCount], is_only_weak, &is_only_weak);
//        };
//
//        aBlock(); // 堆区 堆区
//        void (^bBlock)() = [aBlock copy];
//        bBlock(); // 堆区 堆区
//        aBlock(); // 堆区 堆区
//        NSLog(@"⛈⛈⛈ is_byref retainCount = %lu ---%p---%p", (unsigned long)[is_byref arcDebugRetainCount], is_byref, &is_byref); // 堆区 堆区
        
//        // 不捕获外部自动变量是 global
//        void (^globalBlock)(void) = ^{
//            NSLog(@"❄️❄️❄️ 测试 block isa");
//        };
//
//        int a = 2;
//        // 右边栈区 block 赋值给左侧 block 时，会被复制到堆区
//        void (^mallocBlock)(void) = ^{
//            NSLog(@"❄️❄️❄️ 测试 block isa a = %d", a);
//        };
//
//        globalBlock();
//        mallocBlock();
//
//        NSLog(@"❄️❄️❄️ globalBlock isa: %@", globalBlock);
//        NSLog(@"❄️❄️❄️ mallocBlock isa: %@", mallocBlock);
//        // 栈区 block
//        NSLog(@"❄️❄️❄️ stackBlock isa: %@", ^{ NSLog(@"❄️❄️❄️ a = %d", a); });
        
    }
    
    return 0;
}
