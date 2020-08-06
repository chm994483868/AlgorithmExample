//
//  Father.m
//  Block
//
//  Created by CHM on 2020/8/6.
//  Copyright © 2020 CHM. All rights reserved.
//

#import "Father.h"

@implementation Father

+ (void)initialize {
//    NSLog(@"会被调用几次～～ functionString == %s", __FUNCTION__);
//    if (self == [Father class]) {
//        // 这里只会执行一次，就是在 Father 类初始化的时候执行一次，其它再进入该方法时，self 都是 Father 的子类
        NSLog(@"Father --> self == %@, functionString == %s", [self class], __FUNCTION__);
//    }
}

//- (void)run {
//    NSLog(@"Father --> self == %@, functionString == %s", [self class], __FUNCTION__);
//}

@end
