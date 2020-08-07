//
//  main.m
//  Block
//
//  Created by CHM on 2020/8/6.
//  Copyright © 2020 CHM. All rights reserved.
//

#import <Foundation/Foundation.h>

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

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // insert code here...
        NSLog(@"🎉🎉🎉 Hello, World!");
        
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
        
//        static_val = 111;
//        printf("static_val = %d, global_val = %d, static_global_val = %d\n", static_val, global_val, static_global_val);
    }
    
    return 0;
}
