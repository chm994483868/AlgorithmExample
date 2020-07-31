//
//  TranslateNumbersToStrings.cpp
//  OfferReview
//
//  Created by CHM on 2020/7/31.
//  Copyright © 2020 CHM. All rights reserved.
//

#include "TranslateNumbersToStrings.hpp"

int TranslateNumbersToStrings::getTranslationCount(const string& number) {
    unsigned long length = number.length();
    int* counts = new int[length];
    int count = 0;

    for (int i = length - 1; i >= 0; --i) {
        count = 0;
        if (i < length - 1) {
            count = counts[i + 1];
        } else {
            count = 1;
        }

        if (i < length - 1) {
            int digit1 = number[i] - '0';
            int digit2 = number[i + 1] - '0';
            int converted = digit1 * 10 + digit2;
            if (converted >= 10 && converted <= 25) {
                if (i < length - 2) {
                    count += counts[i + 2];
                } else {
                    count += 1;
                }
            }
        }

        counts[i] = count;
    }

    count = counts[0];
    delete [] counts;

    return count;
}

int TranslateNumbersToStrings::getTranslationCount(int number) {
    if (number < 0) {
        return 0;
    }
    
    string numberString = to_string(number);
    return getTranslationCount(numberString);
}

// 测试代码
void TranslateNumbersToStrings::Test(const string& testName, int number, int expected) {
    if(getTranslationCount(number) == expected)
        cout << testName << " passed." << endl;
    else
        cout << testName << " FAILED." << endl;
}

void TranslateNumbersToStrings::Test1() {
    int number = 0;
    int expected = 1;
    Test("Test1", number, expected);
}

void TranslateNumbersToStrings::Test2() {
    int number = 10;
    int expected = 2;
    Test("Test2", number, expected);
}

void TranslateNumbersToStrings::Test3() {
    int number = 125;
    int expected = 3;
    Test("Test3", number, expected);
}

void TranslateNumbersToStrings::Test4() {
    int number = 126;
    int expected = 2;
    Test("Test4", number, expected);
}

void TranslateNumbersToStrings::Test5() {
    int number = 426;
    int expected = 1;
    Test("Test5", number, expected);
}

void TranslateNumbersToStrings::Test6() {
    int number = 100;
    int expected = 2;
    Test("Test6", number, expected);
}

void TranslateNumbersToStrings::Test7() {
    int number = 101;
    int expected = 2;
    Test("Test7", number, expected);
}

void TranslateNumbersToStrings::Test8() {
    int number = 12258;
    int expected = 5;
    Test("Test8", number, expected);
}

void TranslateNumbersToStrings::Test9() {
    int number = -100;
    int expected = 0;
    Test("Test9", number, expected);
}

void TranslateNumbersToStrings::Test() {
    Test1();
    Test2();
    Test3();
    Test4();
    Test5();
    Test6();
    Test7();
    Test8();
    Test9();
}
