//
//  main.m
//  Block
//
//  Created by CHM on 2020/8/6.
//  Copyright © 2020 CHM. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <objc/runtime.h>
#import <malloc/malloc.h>

#include <stddef.h>

#import "HHStaff.h"

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // insert code here...
//        HHStaff *staff = [[HHStaff alloc] init];
        
////        NSObject *staff = [[NSObject alloc] init];
//        NSLog(@"🧚‍♂️🧚‍♂️🧚‍♂️ class_getInstanceSize => %zd", class_getInstanceSize([staff class]));
//        NSLog(@"🧚‍♂️🧚‍♂️🧚‍♂️ malloc_size => %zd", malloc_size(CFBridgingRetain(staff)));
//        NSLog(@"🧚‍♂️🧚‍♂️🧚‍♂️ sizeof => %zd", sizeof(staff));
        
//        NSObject *staff = [[NSObject alloc] init];
//        NSLog(@"👗👗👗 实例对象地址: %p", staff);
//        Class class1 = object_getClass(staff);
//        NSLog(@"👗👗👗 类对象: %p %@", class1, class1); // HHStaff
//        class1 = object_getClass(class1);
//        NSLog(@"👗👗👗 元类对象: %p %@", class1, class1); // HHStaff 的元类
//        class1 = object_getClass(class1);
//        NSLog(@"👗👗👗 根元类对象: %p %@", class1, class1); // 根元类
//        class1 = object_getClass(class1);
//        NSLog(@"👗👗👗 根根元类对象: %p %@", class1, class1); // 根元类的 isa 指向自己，所以还是同一个地址
//        class1 = object_getClass(class1);
//        NSLog(@"👗👗👗 根根根元类对象: %p %@", class1, class1); // 继续向下，就一值重复了，因为 isa 一直指向的就是自己
//        NSLog(@"👗👗👗 ... 一直同上面重复下去");
     
//        HHStaff *staff = [[HHStaff alloc] init];
//        Class class1 = [staff superclass];
//        NSLog(@"👗👗👗 一父类 - %p %@  其父类的类对象-%p", class1, class1, [HHStaff class]);
//        class1 = [class1 superclass];
//        NSLog(@"👗👗👗 二父类 - %p %@  其父类的类对象-%p", class1, class1, [NSObject class]);
//        class1 = [class1 superclass];
//        NSLog(@"👗👗👗 三父类 - %p %@", class1, class1);
//        class1 = [class1 superclass];
//        NSLog(@"👗👗👗 四父类 - %p %@", class1, class1);
//        class1 = [class1 superclass];
//        NSLog(@"👗👗👗 五父类 - %p %@", class1, class1);
        
//        HHStaff *objc = [[HHStaff alloc] init];
//
//        NSLog(@"👗👗👗 >>>>>> 待验证属性 >>>>>>");
//        Class class1 = object_getClass([HHStaff class]);
//        NSLog(@"👗👗👗一 HHStaff的元类对象 - %p %@", class1, class1);
//        NSLog(@"👗👗👗一 NSObject的类对象 - %p %@", [NSObject class], [NSObject class]);
//
//        class1 = object_getClass([NSObject class]);
//        NSLog(@"👗👗👗一 NSObject的(根)元类对象 - %p %@\n", class1, class1);
//
//        NSLog(@"👗👗👗 >>>>>> objc 相关属性>>>>>>");
//        class1 = object_getClass([objc class]);  // 元类
//        class1 = [class1 superclass];
//        NSLog(@"👗👗👗一 元类对象所属父类 - %p %@", class1, class1);
//
//        class1 = [class1 superclass];
//        NSLog(@"👗👗👗二 父类 - %p %@", class1, class1);
//        class1 = [class1 superclass];
//        NSLog(@"👗👗👗三 父类 - %p %@", class1, class1);
//        class1 = [class1 superclass];
//        NSLog(@"👗👗👗四 父类 - %p %@", class1, class1);
//        class1 = [class1 superclass];
//        NSLog(@"👗👗👗五 父类 - %p %@", class1, class1);
    }
    
    return 0;
}
