//
//  main.m
//  Block
//
//  Created by CHM on 2020/8/6.
//  Copyright © 2020 CHM. All rights reserved.
//

#import <Foundation/Foundation.h>
//#import <objc/runtime.h>
//#import <malloc/malloc.h>

//#include <stddef.h>

//#import "HHStaff.h"

///// Defines a property attribute
//typedef struct {
//    const char * _Nonnull name;           /**< The name of the attribute */
//    const char * _Nonnull value;          /**< The value of the attribute (usually empty) */
//} objc_property_attribute_t;

//// 添加一个 nonatomic、copy 修饰符的 NSString
//void hh_class_addProperty(Class targetClass, const char* propertyName) {
//    objc_property_attribute_t type = {"T", [[NSString stringWithFormat:@"@\"%@\"", NSStringFromClass([NSString class])] UTF8String]};
//    objc_property_attribute_t ownership0 = {"C", ""}; // C = copy
//    objc_property_attribute_t ownership = {"N", ""}; // N = nonatomic
//    objc_property_attribute_t backingivar = {"V", [NSString stringWithFormat:@"_%@", [NSString stringWithCString:propertyName encoding:NSUTF8StringEncoding]].UTF8String};
//    objc_property_attribute_t attrs[] = {type, ownership0, ownership, backingivar};
//
//    class_addProperty(targetClass, propertyName, attrs, 4); // 4: attrs 元素的个数
//}
//
//// 打印属性
//void hh_printerProperty(Class targetClass) {
//    unsigned int outCount, i;
//    objc_property_t *properties = class_copyPropertyList(targetClass, &outCount);
//    for (i = 0; i < outCount; ++i) {
//        objc_property_t property = properties[i];
//        fprintf(stdout, "%s %s\n", property_getName(property), property_getAttributes(property));
//    }
//}
//
//// 打印成员变量
//void hh_printerIvar(Class targetClass) {
//    unsigned int count = 0;
//    Ivar *ivars = class_copyIvarList(targetClass, &count);
//    for (unsigned int i = 0; i < count; ++i) {
//        Ivar const ivar = ivars[i];
//        const char *cName = ivar_getName(ivar);
//        NSString *ivarName = [NSString stringWithUTF8String:cName];
//        NSLog(@"ivarName: %@", ivarName);
//    }
//
//    free(ivars);
//    printf("ivar count = %u\n", count);
//}

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
        
//        Class cls = NSClassFromString(@"HHStaff");
//        NSLog(@"👗👗👗 %p %@ %p", cls, cls, &cls);
        
//        👗👗👗 0x100002838 HHStaff 0x7ffeefbff528
    }
    
    return 0;
}
