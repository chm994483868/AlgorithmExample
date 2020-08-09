//
//  NSObject+ArcDebugRetainCount.m
//  Block
//
//  Created by CHM on 2020/8/9.
//  Copyright © 2020 CHM. All rights reserved.
//

#import "NSObject+ArcDebugRetainCount.h"

@implementation NSObject (ArcDebugRetainCount)

- (NSUInteger)arcDebugRetainCount {
    return [self retainCount];
}

@end
