//
//  Son.m
//  Block
//
//  Created by CHM on 2020/8/6.
//  Copyright © 2020 CHM. All rights reserved.
//

#import "Son.h"

@implementation Son

+ (void)initialize {
    NSLog(@"Son --> self == %@, functionString == %s", [self class], __FUNCTION__);
}

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
}

@end
