//
//  main.m
//  Block
//
//  Created by CHM on 2020/8/6.
//  Copyright © 2020 CHM. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "HHManager.h"
#import <objc/runtime.h>

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // insert code here...
        // NSLog(@"🎉🎉🎉 Hello, World!");
        
        HHStaff *staffA = [[HHStaff alloc] init];
        HHStaff *staffB = [[HHStaff alloc] init];
        
        NSLog(@"♻️♻️♻️ 实例对象: %p - %p", staffA, staffB);
        
        Class staffClassA = [staffA class];
        Class staffClassB = objc_getClass(object_getClassName(staffB));
        Class staffClassB2 = object_getClass(staffB);
        Class staffClassC = [HHStaff class];
        
        NSLog(@"♻️♻️♻️ 类  对象: %p - %p - %p - %p", staffClassA, staffClassB, staffClassB2, staffClassC);
        
        Class staffMetaClassA = object_getClass(staffClassA);
        Class staffMetaClassB = object_getClass(staffClassB);
        
        NSLog(@"♻️♻️♻️ 元类对象: %p - %p", staffMetaClassA, staffMetaClassB);
    }
    
    return 0;
}
