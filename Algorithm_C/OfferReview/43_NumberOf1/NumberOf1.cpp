//
//  NumberOf1.cpp
//  OfferReview
//
//  Created by CHM on 2020/7/31.
//  Copyright © 2020 CHM. All rights reserved.
//

#include "NumberOf1.hpp"

int NumberOf1::numberOf1(unsigned int n) {
    int number = 0;
    while (n) {
        if (n % 10 == 1) {
            ++number;
        }
        
        n /= 10;
    }
    
    return number;
}

int NumberOf1::numberOf1Between1AndN_Solution1(unsigned int n) {
    int number = 0;
    for (unsigned int i = 1; i <= n; ++i) {
        number += numberOf1(i);
    }
    
    return number;
}

int NumberOf1::numberOf1(const char* strN) {
    if (strN == nullptr || *strN < '0' || *strN > '9' || *strN == '\0') {
        return 0;
    }
    
    int first = *strN - '0';
    unsigned int length = static_cast<unsigned int>(strlen(strN));
    
    if (length == 1 && first == 0) {
        return 0;
    }
    
    if (length == 1 && first > 0) {
        return 1;
    }
    
    int numFirstDigit = 0;
    if (first > 1) {
        numFirstDigit = powerBase10(length - 1);
    } else if (first == 1) {
        numFirstDigit = atoi(strN + 1) + 1;
    }
    
    int numOtherDigits = first * (length - 1) * powerBase10(length - 2);
    
    // 递归
    int numRecursive = numberOf1(strN + 1);
    
    return numFirstDigit + numOtherDigits + numRecursive;
}

// 10 的次方
int NumberOf1::powerBase10(unsigned int n) {
    int result = 1;
    for (unsigned int i = 0; i < n; ++i) {
        result *= 10;
    }
    return result;
}

int NumberOf1::numberOf1Between1AndN_Solution2(int n) {
    if (n <= 0) {
        return 0;
    }
    
    char strN[50];
    sprintf(strN, "%d", n);
    
    return numberOf1(strN);
}

// 测试代码
void NumberOf1::Test(const char* testName, int n, int expected) {
    if(testName != nullptr)
        printf("%s begins: \n", testName);
    
    if(numberOf1Between1AndN_Solution1(n) == expected)
        printf("Solution1 passed.\n");
    else
        printf("Solution1 failed.\n");
    
    if(numberOf1Between1AndN_Solution2(n) == expected)
        printf("Solution2 passed.\n");
    else
        printf("Solution2 failed.\n");

    printf("\n");
}

void NumberOf1::Test() {
    Test("Test1", 1, 1);
    Test("Test2", 5, 1);
    Test("Test3", 10, 2);
    Test("Test4", 55, 16);
    Test("Test5", 99, 20);
    Test("Test6", 10000, 4001);
    Test("Test7", 21345, 18821);
    Test("Test8", 0, 0);
}
