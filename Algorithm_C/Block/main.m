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
        // 
    
    }
    return 0;
}
