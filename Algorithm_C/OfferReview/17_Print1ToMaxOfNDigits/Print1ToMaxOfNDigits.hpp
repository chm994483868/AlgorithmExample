//
//  Print1ToMaxOfNDigits.hpp
//  OfferReview
//
//  Created by HM C on 2020/7/28.
//  Copyright © 2020 CHM. All rights reserved.
//

#ifndef Print1ToMaxOfNDigits_hpp
#define Print1ToMaxOfNDigits_hpp

#include <stdio.h>
#include <memory>

namespace Print1ToMaxOfNDigits {

// 17：打印1到最大的n位数
// 题目：输入数字 n，按顺序打印出从 1 最大的 n 位十进制数。比如输入 3，则
// 打印出 1、2、3 一直到最大的3位数即 999。
void printNumber(char* number);
bool increment(char* number);
void print1ToMaxOfNDigits_1(int n);

// 测试代码
void Test(int n);

void Test();

}

#endif /* Print1ToMaxOfNDigits_hpp */
