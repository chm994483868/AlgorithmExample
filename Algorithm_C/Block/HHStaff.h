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
    // Class isa; 补0 偏0 长8 //在 继承的 NSObject 中还有一个 Class isa; 成员变量
    int _age; // 补0 偏8 长12
    int _height; // 补0 偏12 长16
    NSString *_name; // 补0 偏16 长24
}

//
//- (void)doInstanceStaffWork; // 对象方法
//+ (void)doClassStaffWork; // 类方法

@end

NS_ASSUME_NONNULL_END
