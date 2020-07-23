//
//  UglyNumber.cpp
//  Algorithm_C
//
//  Created by HM C on 2020/7/23.
//  Copyright © 2020 CHM. All rights reserved.
//

#include "UglyNumber.hpp"

bool isUgly(int number) {
    while (number % 2 == 0) {
        number /= 2;
    }
    
    while (number % 3 == 0) {
        number /= 3;
    }
    
    while (number % 5 == 0) {
        number /= 5;
    }
    
    return (number == 1) ? true : false;
}

int getUglyNumber_Solution1(int index) {
    if (index <= 0) {
        return 0;
    }
    
    int number = 0;
    int uglyFound = 0;
    while (uglyFound < index) {
        ++number;
        
        if (isUgly(number)) {
            ++uglyFound;
        }
    }
    
    return number;
}

int min(int number1, int number2, int number3);

int getUglyNumber_Solution2(int index) {
    if (index <= 0) {
        return 0;
    }
    
    int *pUglyNumbers = new int[index];
    pUglyNumbers[0] = 1;
    int nextUglyIndex = 1;
    
    int* pMultiply2 = pUglyNumbers;
    int* pMultiply3 = pUglyNumbers;
    int* pMultiply5 = pUglyNumbers;
    
    while (nextUglyIndex < index) {
        int min_mark = min(*pMultiply2 * 2, *pMultiply3 * 3, *pMultiply5 * 5);
        pUglyNumbers[nextUglyIndex] = min_mark;
        
        while (*pMultiply2 * 2 <= pUglyNumbers[nextUglyIndex]) {
            ++pMultiply2;
        }
        
        while (*pMultiply3 * 3 <= pUglyNumbers[nextUglyIndex]) {
            ++pMultiply3;
        }
        
        while (*pMultiply5 * 5 <= pUglyNumbers[nextUglyIndex]) {
            ++pMultiply5;
        }
        
        ++nextUglyIndex;
    }
    
    int ugly = pUglyNumbers[nextUglyIndex - 1];
    delete [] pUglyNumbers;
    return ugly;
}

int min(int number1, int number2, int number3) {
    int min = (number1 < number2) ? number1: number2;
    min = (min < number3) ? min: number3;
    return min;
}
