//
//  KLeastNumbers.cpp
//  OfferReview
//
//  Created by CHM on 2020/7/31.
//  Copyright © 2020 CHM. All rights reserved.
//

#include "KLeastNumbers.hpp"

void KLeastNumbers::getLeastNumbers_Solution1(int* input, int n, int* output, int k) {
    if (input == nullptr || n <= 0 || output == nullptr || k <= 0 || k > n) {
        return;
    }
    
    int start = 0;
    int end = n - 1;
    int index = partition(input, n, start, end);
    
    while (index != k - 1) {
        if (index > k - 1) {
            end = index - 1;
            index = partition(input, n, start, end);
        } else {
            start = index + 1;
            index = partition(input, n, start, end);
        }
    }
    
    for (unsigned int i = 0; i < k; ++i) {
        output[i] = input[i];
    }
}

void KLeastNumbers::getLeastNumbers_Solution2(const vector<int>& data, intSet& leastNumbers, int k) {
    leastNumbers.clear();
    
    if (k < 1 || data.size() < k) {
        return;
    }
    
    vector<int>::const_iterator iter = data.begin();
    for (; iter != data.end(); ++iter) {
        if ((leastNumbers.size()) < k) {
            leastNumbers.insert(*iter);
        } else {
            setIterator iterGreatest = leastNumbers.begin();
            if (*iter < *(leastNumbers.begin())) {
                leastNumbers.erase(iterGreatest);
                leastNumbers.insert(*iter);
            }
        }
    }
}

// 测试代码
void KLeastNumbers::Test(char* testName, int* data, int n, int* expectedResult, int k) {
    if(testName != nullptr)
        printf("%s begins: \n", testName);

    vector<int> vectorData;
    for(int i = 0; i < n; ++ i)
        vectorData.push_back(data[i]);

    if(expectedResult == nullptr)
        printf("The input is invalid, we don't expect any result.\n");
    else
    {
        printf("Expected result: \n");
        for(int i = 0; i < k; ++ i)
            printf("%d\t", expectedResult[i]);
        printf("\n");
    }

    printf("Result for solution1:\n");
    int* output = new int[k];
    getLeastNumbers_Solution1(data, n, output, k);
    if(expectedResult != nullptr)
    {
        for(int i = 0; i < k; ++ i)
            printf("%d\t", output[i]);
        printf("\n");
    }

    delete[] output;

    printf("Result for solution2:\n");
    intSet leastNumbers;
    getLeastNumbers_Solution2(vectorData, leastNumbers, k);
    printf("The actual output numbers are:\n");
    for(setIterator iter = leastNumbers.begin(); iter != leastNumbers.end(); ++iter)
        printf("%d\t", *iter);
    printf("\n\n");
}

// k小于数组的长度
void KLeastNumbers::Test1() {
    int data[] = {4, 5, 1, 6, 2, 7, 3, 8};
    int expected[] = {1, 2, 3, 4};
    Test("Test1", data, sizeof(data) / sizeof(int), expected, sizeof(expected) / sizeof(int));
}

// k等于数组的长度
void KLeastNumbers::Test2() {
    int data[] = {4, 5, 1, 6, 2, 7, 3, 8};
    int expected[] = {1, 2, 3, 4, 5, 6, 7, 8};
    Test("Test2", data, sizeof(data) / sizeof(int), expected, sizeof(expected) / sizeof(int));
}

// k大于数组的长度
void KLeastNumbers::Test3() {
    int data[] = {4, 5, 1, 6, 2, 7, 3, 8};
    int* expected = nullptr;
    Test("Test3", data, sizeof(data) / sizeof(int), expected, 10);
}

// k等于1
void KLeastNumbers::Test4() {
    int data[] = {4, 5, 1, 6, 2, 7, 3, 8};
    int expected[] = {1};
    Test("Test4", data, sizeof(data) / sizeof(int), expected, sizeof(expected) / sizeof(int));
}

// k等于0
void KLeastNumbers::Test5() {
    int data[] = {4, 5, 1, 6, 2, 7, 3, 8};
    int* expected = nullptr;
    Test("Test5", data, sizeof(data) / sizeof(int), expected, 0);
}

// 数组中有相同的数字
void KLeastNumbers::Test6() {
    int data[] = {4, 5, 1, 6, 2, 7, 2, 8};
    int expected[] = {1, 2};
    Test("Test6", data, sizeof(data) / sizeof(int), expected, sizeof(expected) / sizeof(int));
}

// 输入空指针
void KLeastNumbers::Test7() {
    int* expected = nullptr;
    Test("Test7", nullptr, 0, expected, 0);
}

void KLeastNumbers::Test() {
    Test1();
    Test2();
    Test3();
    Test4();
    Test5();
    Test6();
    Test7();
}
