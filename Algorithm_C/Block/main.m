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
//    char d = 'D';
//    return [^{
//        printf("🔔🔔🔔 %c\n", d);
//    } copy];
    
    return ^{
        printf("🔔🔔🔔 %c\n", 111);
    };
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
    NSLog(@"one 🔔🔔🔔 %@", exampleE_getBlock());
    eBlock block = exampleE_getBlock();
    NSLog(@"two 🔔🔔🔔 %@", block);
    block();
}

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // insert code here...
        NSLog(@"🎉🎉🎉 Hello, World!");
    
        
    }
    
    return 0;
}
