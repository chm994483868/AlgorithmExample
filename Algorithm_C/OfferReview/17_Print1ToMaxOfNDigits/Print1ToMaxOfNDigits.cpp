//
//  Print1ToMaxOfNDigits.cpp
//  OfferReview
//
//  Created by HM C on 2020/7/28.
//  Copyright © 2020 CHM. All rights reserved.
//

#include "Print1ToMaxOfNDigits.hpp"

void Print1ToMaxOfNDigits::printNumber(char* number) {
    bool isBeginning0 = true;
    unsigned long nLength = strlen(number);
    for (unsigned long i = 0; i < nLength; ++i) {
        if (isBeginning0 && number[i] != '0') {
            isBeginning0 = false;
        }
        
        if (!isBeginning0) {
            printf("%c", number[i]);
        }
    }
    
    printf("\t");
}

bool Print1ToMaxOfNDigits::increment(char* number) {
    bool isOverflow = false;
    int nTakeOver = 0;
    unsigned long nLength = strlen(number);
    for (unsigned long i = nLength - 1; i >= 0; --i) {
        int sum = number[i] - '0' + nTakeOver;
        
        if (i == nLength - 1) {
            ++sum;
        }
        
        if (sum >= 10) {
            if (i == 0) {
                isOverflow = true;
            } else {
                sum -= 10;
                nTakeOver = 1;
                number[i] = sum + '0';
            }
        } else {
            number[i] = sum + '0';
            break;
        }
    }
    
    return isOverflow;
}

void Print1ToMaxOfNDigits::print1ToMaxOfNDigits_1(int n) {
    if (n <= 0) {
        return;
    }
    
    char* number = new char[n + 1];
    memset(number, '0', n);
    number[n] = '\0';
    
    while (!increment(number)) {
        printNumber(number);
    }
    
    delete [] number;
}

// 测试代码
void Print1ToMaxOfNDigits::Test(int n) {
    printf("Test for %d begins:\n", n);

    print1ToMaxOfNDigits_1(n);
//    Print1ToMaxOfNDigits_2(n);

    printf("\nTest for %d ends.\n", n);
}

void Print1ToMaxOfNDigits::Test() {
    Test(1);
    Test(2);
    Test(3);
    Test(0);
    Test(-1);
}
