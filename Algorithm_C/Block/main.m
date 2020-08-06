//
//  main.m
//  Block
//
//  Created by CHM on 2020/8/6.
//  Copyright © 2020 CHM. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "Son.h"
#import "Father.h"
#import "GrandSon.h"

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


int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // insert code here...
        NSLog(@"🎉🎉🎉 Hello, World!");
        
//        // 示例 1：
//        int dmy = 256;
//        int val = 10;
//        const char* fmt = "val = %d\n";
//        void (^blk)(void) = ^{
//            printf(fmt, val);
//        };
//
//        val = 2;
//        fmt = "These values were changed. val = %d\n";
//
//        blk();
//        // 运行结果：val = 10

        // 示例 2：
//        int dmy = 256;
//        int temp = 10;
//        int* val = &temp;
//
//        printf("🎉🎉 val 初始值：= %d\n", *val);
//
//        const char* fmt = "🎉 Block 内部：val = %d\n";
//        void (^blk)(void) = ^{
//            printf(fmt, *val);
////            int temp2 = 30;
////            val = &temp2;
//            *val = 22;
//        };
//
//        *val = 20; // 修改 val
//        fmt = "These values were changed. val = %d\n";
//
//        blk();
//
//        printf("🎉🎉 val = %d\n", *val); // block 执行时把 *val 修改为 22
        
        // 运行结果：val = 20

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
        
        [GrandSon TEST];
        
//        [son eat];
        
//        Father *father = [[Father alloc] init];
//        [father run];
        
    }
    
    return 0;
}
