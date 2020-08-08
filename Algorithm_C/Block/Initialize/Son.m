//
//  Son.m
//  Block
//
//  Created by CHM on 2020/8/6.
//  Copyright © 2020 CHM. All rights reserved.
//

#import "Son.h"

@implementation Son

//+ (void)initialize {
//    NSLog(@"Son --> self == %@, functionString == %s", [self class], __FUNCTION__);
//}

- (void)eat {
    NSLog(@"Son --> self == %@, functionString == %s", [self class], __FUNCTION__);
}

- (void)run {
//    [super run];

//    NSLog(@"Son --> self == %@, functionString == %s", [self class], __FUNCTION__);
}

+ (void)ClassRun {
//    [super initialize];
//    [super initialize];
//    [super initialize];
    [self initialize];
}

+ (id)getBlockArray {
    int val = 10;
    return [[NSArray alloc] initWithObjects:[^{NSLog(@"blk0: %d", val);} copy], [^{NSLog(@"blk1: %d", val);} copy], nil];
}

@end
