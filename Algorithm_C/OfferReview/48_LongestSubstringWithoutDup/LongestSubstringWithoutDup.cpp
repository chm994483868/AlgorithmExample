//
//  LongestSubstringWithoutDup.cpp
//  OfferReview
//
//  Created by CHM on 2020/7/31.
//  Copyright © 2020 CHM. All rights reserved.
//

#include "LongestSubstringWithoutDup.hpp"

//int LongestSubstringWithoutDup::longestSubstringWithoutDuplication_2(const std::string& str) {
//    
//    int curLength = 0;
//    int maxLength = 0;
//    
//    int* position = new int[26];
//    unsigned int i = 0;
//    for (; i < 26; ++i) {
//        position[i] = -1;
//    }
//    
//    for (i = 0; i < str.length(); ++i) {
//        int prevIndex = position[str[i] - 'a'];
//        if (prevIndex < 0 || i - prevIndex > curLength) {
//            ++curLength;
//        } else {
//            if (curLength > maxLength) {
//                maxLength = curLength;
//            }
//            
//            curLength = i - prevIndex;
//        }
//        position[str[i] - 'a'] = i;
//    }
//    
//    if (curLength > maxLength) {
//        maxLength = curLength;
//    }
//    
//    delete [] position;
//    return maxLength;
//}
//
//// 测试代码
//void LongestSubstringWithoutDup::testSolution1(const std::string& input, int expected) {
//    int output = longestSubstringWithoutDuplication_1(input);
//    if(output == expected)
//        std::cout << "Solution 1 passed, with input: " << input << std::endl;
//    else
//        std::cout << "Solution 1 FAILED, with input: " << input << std::endl;
//}
//
////void LongestSubstringWithoutDup::testSolution2(const std::string& input, int expected) {
////    int output = longestSubstringWithoutDuplication_2(input);
////    if(output == expected)
////        std::cout << "Solution 2 passed, with input: " << input << std::endl;
////    else
////        std::cout << "Solution 2 FAILED, with input: " << input << std::endl;
////}
//
//void LongestSubstringWithoutDup::test(const std::string& input, int expected) {
//    testSolution1(input, expected);
////    testSolution2(input, expected);
//}
//
//void LongestSubstringWithoutDup::test1() {
//    const std::string input = "abcacfrar";
//    int expected = 4;
//    test(input, expected);
//}
//
//void LongestSubstringWithoutDup::test2() {
//    const std::string input = "acfrarabc";
//    int expected = 4;
//    test(input, expected);
//}
//
//void LongestSubstringWithoutDup::test3() {
//    const std::string input = "arabcacfr";
//    int expected = 4;
//    test(input, expected);
//}
//
//void LongestSubstringWithoutDup::test4() {
//    const std::string input = "aaaa";
//    int expected = 1;
//    test(input, expected);
//}
//
//void LongestSubstringWithoutDup::test5() {
//    const std::string input = "abcdefg";
//       int expected = 7;
//       test(input, expected);
//}
//
//void LongestSubstringWithoutDup::test6() {
//    const std::string input = "aaabbbccc";
//    int expected = 2;
//    test(input, expected);
//}
//
//void LongestSubstringWithoutDup::test7() {
//    const std::string input = "abcdcba";
//    int expected = 4;
//    test(input, expected);
//}
//
//void LongestSubstringWithoutDup::test8() {
//    const std::string input = "abcdaef";
//    int expected = 6;
//    test(input, expected);
//}
//
//void LongestSubstringWithoutDup::test9() {
//    const std::string input = "a";
//    int expected = 1;
//    test(input, expected);
//}
//
//void LongestSubstringWithoutDup::test10() {
//    const std::string input = "";
//    int expected = 0;
//    test(input, expected);
//}
//
//void LongestSubstringWithoutDup::Test() {
//    test1();
//    test2();
//    test3();
//    test4();
//    test5();
//    test6();
//    test7();
//    test8();
//    test9();
//    test10();
//}
