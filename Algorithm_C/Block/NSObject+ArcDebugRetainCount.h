//
//  NSObject+ArcDebugRetainCount.h
//  Block
//
//  Created by CHM on 2020/8/9.
//  Copyright © 2020 CHM. All rights reserved.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface NSObject (ArcDebugRetainCount)

- (NSUInteger)arcDebugRetainCount;

@end

NS_ASSUME_NONNULL_END
