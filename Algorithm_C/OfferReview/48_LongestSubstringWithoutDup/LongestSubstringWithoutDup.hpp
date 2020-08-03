//
//  LongestSubstringWithoutDup.hpp
//  OfferReview
//
//  Created by CHM on 2020/7/31.
//  Copyright © 2020 CHM. All rights reserved.
//

#ifndef LongestSubstringWithoutDup_hpp
#define LongestSubstringWithoutDup_hpp

#include <stdio.h>
#include <string>
#include <iostream>

namespace LongestSubstringWithoutDup {

//// 48：最长不含重复字符的子字符串
//// 题目：请从字符串中找出一个最长的不包含重复字符的子字符串，计算该最长子
//// 字符串的长度。假设字符串中只包含从'a'到'z'的字符。
//// 方法一：蛮力法
//bool hasDuplication(const std::string& str, int position[]);
//int longestSubstringWithoutDuplication_1(const std::string& str);
//
////// 方法二：动态规划
////int longestSubstringWithoutDuplication_2(const std::string& str);
//
//// 测试代码
//void testSolution1(const std::string& input, int expected);
//void testSolution2(const std::string& input, int expected);
//void test(const std::string& input, int expected);
//void test1();
//void test2();
//void test3();
//void test4();
//void test5();
//void test6();
//void test7();
//void test8();
//void test9();
//void test10();
//
//void Test();

void swap(int* num1, int* num2) {
    int temp = *num1;
    *num1 = *num2;
    *num2 = temp;
}

// 冒泡排序
void bubbleSort(int numbers[], int length) {
    if (numbers == nullptr || length <= 0) {
        return;
    }
    
    int k = length - 1;
    for (int i = 0; i < length - 1; ++i) {
        bool noExchange = true;
        int n = 0;
        for (int j = 0; j < k; ++j) {
            if (numbers[j] > numbers[j + 1]) {
                swap(&numbers[j], &numbers[j + 1]);
                noExchange = false;
                n = j;
            }
        }
        
        if (noExchange) {
            break;
        }
        k = n;
    }
}

// 快速排序
void quickSort(int numbers[], int length, int start, int end) {
    if (numbers == nullptr || length <= 0 || start < 0 || end >= length) {
        return;
    }
    
    if (start >= end) {
        return;
    }
    
    int i = start, j = end, temp = numbers[i];
    while (i < j) {
        while (i < j && numbers[j] >= temp) {
            --j;
        }
        
        if (i < j) {
            numbers[i++] = numbers[j];
        }
        
        while (i < j && numbers[i] < temp) {
            ++i;
        }
        
        if (i < j) {
            numbers[j--] = numbers[i];
        }
    }
    
    numbers[i] = temp;
    quickSort(numbers, length, start, i - 1);
    quickSort(numbers, length, i + 1, end);
}
    
}

#endif /* LongestSubstringWithoutDup_hpp */
