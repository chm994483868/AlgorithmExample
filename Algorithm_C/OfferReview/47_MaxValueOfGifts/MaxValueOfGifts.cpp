//
//  MaxValueOfGifts.cpp
//  OfferReview
//
//  Created by CHM on 2020/7/31.
//  Copyright © 2020 CHM. All rights reserved.
//

#include "MaxValueOfGifts.hpp"

int MaxValueOfGifts::getMaxValue_solution1(const int* values, int rows, int cols) {
    if (values == nullptr || rows <= 0 || cols <= 0) {
        return 0;
    }
    
    int** maxValues = new int*[rows];
    unsigned int i = 0;
    for (; i < rows; ++i) {
        maxValues[i] = new int[cols];
    }
    
    for (i = 0; i < rows; ++i) {
        for (unsigned int j = 0; j < cols; ++j) {
            int up = 0;
            int left = 0;
            
            if (i > 0) {
                up = maxValues[i - 1][j];
            }
            
            if (j > 0) {
                left = maxValues[i][j - 1];
            }
            
            maxValues[i][j] = std::max(up, left) + values[i * cols + j];
        }
    }
    
    int maxValue = maxValues[rows - 1][cols - 1];
    for (i = 0; i < rows; ++i) {
        delete [] maxValues[i];
    }
    delete [] maxValues;

    return maxValue;
}

int MaxValueOfGifts::getMaxValue_solution2(const int* values, int rows, int cols) {
    if (values == nullptr || rows <= 0 || cols <= 0) {
        return 0;
    }
    
    int* maxValues = new int[cols];
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            int left = 0;
            int up = 0;
            
            if (i > 0) {
                up = maxValues[j];
            }
            
            if (j > 0) {
                left = maxValues[j - 1];
            }
            
            maxValues[j] = std::max(left, up) + values[i * cols + j];
        }
    }
    
    int maxValue = maxValues[cols - 1];
    delete [] maxValues;
    
    return maxValue;
}

// 测试代码
void MaxValueOfGifts::test(const char* testName, const int* values, int rows, int cols, int expected) {
    if(getMaxValue_solution1(values, rows, cols) == expected)
        std::cout << testName << ": solution1 passed." << std::endl;
    else
        std::cout << testName << ": solution1 FAILED." << std::endl;

    if(getMaxValue_solution2(values, rows, cols) == expected)
        std::cout << testName << ": solution2 passed." << std::endl;
    else
        std::cout << testName << ": solution2 FAILED." << std::endl;
}

void MaxValueOfGifts::test1() {
    // 三行三列
    int values[][3] = {
        { 1, 2, 3 },
        { 4, 5, 6 },
        { 7, 8, 9 }
    };
    int expected = 29;
    test("test1", (const int*) values, 3, 3, expected);
}

void MaxValueOfGifts::test2() {
    //四行四列
    int values[][4] = {
        { 1, 10, 3, 8 },
        { 12, 2, 9, 6 },
        { 5, 7, 4, 11 },
        { 3, 7, 16, 5 }
    };
    int expected = 53;
    test("test2", (const int*) values, 4, 4, expected);
}

void MaxValueOfGifts::test3() {
    // 一行四列
    int values[][4] = {
        { 1, 10, 3, 8 }
    };
    int expected = 22;
    test("test3", (const int*) values, 1, 4, expected);
}

void MaxValueOfGifts::test4() {
    int values[4][1] = {
        { 1 },
        { 12 },
        { 5 },
        { 3 }
    };
    int expected = 21;
    test("test4", (const int*) values, 4, 1, expected);
}

void MaxValueOfGifts::test5() {
    // 一行一列
    int values[][1] = {
        { 3 }
    };
    int expected = 3;
    test("test5", (const int*) values, 1, 1, expected);
}

void MaxValueOfGifts::test6() {
    // 空指针
    int expected = 0;
    test("test6", nullptr, 0, 0, expected);
}

void MaxValueOfGifts::Test() {
    test1();
    test2();
    test3();
    test4();
    test5();
}
