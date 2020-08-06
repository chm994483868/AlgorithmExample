//
//  Son.h
//  Block
//
//  Created by CHM on 2020/8/6.
//  Copyright © 2020 CHM. All rights reserved.
//

#import "Father.h"

NS_ASSUME_NONNULL_BEGIN

@interface Son : Father

- (void)eat;
- (void)run;
+ (void)ClassRun;

@end

NS_ASSUME_NONNULL_END
