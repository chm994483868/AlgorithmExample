//
//  HHStaff.h
//  Block
//
//  Created by HM C on 2020/8/13.
//  Copyright © 2020 CHM. All rights reserved.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface HHStaff : NSObject {
    NSString *name;
}

- (void)doInstanceStaffWork; // 对象方法
+ (void)doClassStaffWork; // 类方法

@end

NS_ASSUME_NONNULL_END
