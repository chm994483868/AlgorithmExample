//
//  main.m
//  Block
//
//  Created by CHM on 2020/8/6.
//  Copyright © 2020 CHM. All rights reserved.
//

#import <Foundation/Foundation.h>

typedef void(^Blk_T)(int);
// block 定义(匿名)
int i = 10;
Blk_T a = ^void (int event) {
    printf("buttonID: %d event = %d\n", i, event);
};
// 函数定义
void Func(int event) {
    printf("buttonID: %d event = %d\n", i, event);
};
void (*funcPtr)(int) = &Func;


int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // insert code here...
        NSLog(@"🎉🎉🎉 Hello, World!");

    }
    return 0;
}
