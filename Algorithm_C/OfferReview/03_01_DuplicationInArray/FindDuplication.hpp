//
//  FindDuplication.hpp
//  OfferReview
//
//  Created by CHM on 2020/7/27.
//  Copyright © 2020 CHM. All rights reserved.
//

#ifndef FindDuplication_hpp
#define FindDuplication_hpp

#include <stdio.h>
#include <cstdio>
#include <string>

using namespace std;

namespace FindDuplication {

// 3.(-)：找出数组中重复的数字
// 题目：在一个长度为 n 的数组里的所有数字都在 0 到 n - 1 的范围内。数组中某些数字是重复的，但不知道有几个数字重复了，
// 也不知道每个数字重复了几次。请找出数组中任意一个重复的数字。例如，如果输入长度为7的数组 {2, 3, 1, 0, 2, 5, 3}，
// 那么对应的输出是重复的数字 2 或者 3。
// 分析：由于数组中数字的范围是 0 ~ (n - 1)，可以把每个数字都放在数组中和自己的值相等的下标处
// 例如上面的从数组左边开始，首先 2 和 1 交换，然后 1 和 3 交换，然后 3 和 0 交换，这时候 下标 0 处放的数值是 0 了，
// 1)：{0, 1, 2, 3, 2, 5, 3}
// 然后往右边移动, 1 处存的 1，往后移动 2 处存放的是 2， 往后移动 3 处存放的是 3， 往后移动 4 处存放的 2，和 2 处存放的 2 重复了
// 返回 2 即为重复的数字

/// 找出数组中重复的数字，返回值：true 输入有效，并且数组中存在重复的数字 false 输入无效，或者数组中没有重复的数字
/// @param numbers 数组
/// @param length 数组长度
/// @param duplication （输出）数组中的一个重复的数字
bool duplicate(int numbers[], int length, int *duplication);

// 测试代码
bool contains(int array[], int length, int number);
void test(char* testName, int numbers[], int lengthNumbers, int expected[], int expectedExpected, bool validArgument);
// 重复的数字是数组中最小的数字
void test1();
// 重复的数字是数组中最大的数字
void test2();
// 数组中存在多个重复的数字
void test3();
// 没有重复的数字
void test4();
// 没有重复的数字
void test5();
// 无效的输入
void test6();

void Test();

}

#endif /* FindDuplication_hpp */
