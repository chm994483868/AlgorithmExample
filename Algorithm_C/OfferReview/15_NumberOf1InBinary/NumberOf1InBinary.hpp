//
//  NumberOf1InBinary.hpp
//  OfferReview
//
//  Created by CHM on 2020/7/28.
//  Copyright © 2020 CHM. All rights reserved.
//

#ifndef NumberOf1InBinary_hpp
#define NumberOf1InBinary_hpp

#include <stdio.h>

namespace NumberOf1InBinary {

// 15：二进制中1的个数
// 题目：请实现一个函数，输入一个整数，输出该数二进制表示中1的个数。例如
// 把9表示成二进制是1001，有2位是1。因此如果输入9，该函数输出2。
int numberOf1_Solution1(int n);
int numberOf1_Solution2(int n);

// 测试代码
void Test(int number, unsigned int expected);

void Test();

}

#endif /* NumberOf1InBinary_hpp */
