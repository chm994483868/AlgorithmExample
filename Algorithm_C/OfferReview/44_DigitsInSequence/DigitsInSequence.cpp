//
//  DigitsInSequence.cpp
//  OfferReview
//
//  Created by CHM on 2020/7/31.
//  Copyright © 2020 CHM. All rights reserved.
//

#include "DigitsInSequence.hpp"

int DigitsInSequence::countOfIntegers(int digits) {
    if (digits == 1) {
        return 10;
    }
    
    int count = (int)pow(10, digits - 1);
    return 9 * count;
}

int DigitsInSequence::digitAtIndex(int index, int digits) {
    int number = beginNumber(digits) + index / digits;
    int indexFromRight = digits - index % digits;
    for (int i = 1; i < indexFromRight; ++i) {
        number /= 10;
    }
    
    return number % 10;
}

int DigitsInSequence::beginNumber(int digits) {
    if (digits == 1) {
        return 0;
    }
    
    return (int)pow(10, digits - 1);
}

int DigitsInSequence::digitAtIndex(int index) {
    if (index < 0) {
        return -1;
    }
    
    int digits = 1;
    while (true) {
        int number = countOfIntegers(digits);
        if (index < number * digits) {
            return digitAtIndex(index, digits);
        }
        
        index -= number * digits;
        ++digits;
    }
    
    return -1;
}

// 测试代码
void DigitsInSequence::test(const char* testName, int inputIndex, int expectedOutput) {
    if(digitAtIndex(inputIndex) == expectedOutput)
        cout << testName << " passed." << endl;
    else
        cout << testName << " FAILED." << endl;
}

void DigitsInSequence::Test() {
    test("Test1", 0, 0);
    test("Test2", 1, 1);
    test("Test3", 9, 9);
    test("Test4", 10, 1);
    test("Test5", 189, 9);  // 数字99的最后一位，9
    test("Test6", 190, 1);  // 数字100的第一位，1
    test("Test7", 1000, 3); // 数字370的第一位，3
    test("Test8", 1001, 7); // 数字370的第二位，7
    test("Test9", 1002, 0); // 数字370的第三位，0
}
