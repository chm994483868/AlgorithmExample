//
//  Father+Test2.m
//  Block
//
//  Created by CHM on 2020/8/10.
//  Copyright © 2020 CHM. All rights reserved.
//

#import "Father+Test2.h"

@implementation Father (Test2)

+ (void)load {
    NSLog(@"💭💭💭 Father (Test2) --> self == %@, functionString == %s", [self class], __FUNCTION__);
}

@end
