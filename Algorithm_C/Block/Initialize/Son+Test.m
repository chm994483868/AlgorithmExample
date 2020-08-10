//
//  Son+Test.m
//  Block
//
//  Created by CHM on 2020/8/6.
//  Copyright © 2020 CHM. All rights reserved.
//

#import "Son+Test.h"

@implementation Son (Test)

//+ (void)initialize {
//    NSLog(@"Son (Test) Son --> ⬜️ self == %@, functionString == %s", [self class], __FUNCTION__);
//}

+ (void)load {
    NSLog(@"💭💭💭 Son (Test) --> self == %@, functionString == %s", [self class], __FUNCTION__);
}

@end
