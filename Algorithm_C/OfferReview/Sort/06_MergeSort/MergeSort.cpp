//
//  MergeSort.cpp
//  OfferReview
//
//  Created by CHM on 2020/8/4.
//  Copyright © 2020 CHM. All rights reserved.
//

#include "MergeSort.hpp"

// 将有两个有序数列 a[first...mid] 和 a[mid...last] 合并
void MergeSort::mergeSort(int nums[], int first, int last, int temp[]) {
    if (first >= last) {
        return;
    }
    
    int mid = ((last - first) >> 1 ) + first;
    
    mergeSort(nums, first, mid, temp);
    mergeSort(nums, mid + 1, last, temp); // 递归拆
    
    mergeArray(nums, first, mid, last, temp); // 递归结束开始合并
}

void MergeSort::mergeArray(int nums[], int first, int mid, int last, int temp[]) {
    int i = first, j = mid + 1;
    int m = mid, n = last;
    int k = 0;
    
    while (i <= m && j <= n) {
        if (nums[i] <= nums[j]) {
            temp[k++] = nums[i++];
        } else {
            temp[k++] = nums[j++];
        }
    }
    
    while (i <= m) {
        temp[k++] = nums[i++];
    }
    
    while (j <= n) {
        temp[k++] = nums[j++];
    }
    
    for (i = 0; i < k; ++i) {
        nums[first + i] = temp[i];
    }
}

// 测试代码
void MergeSort::test(char* testName, int nums[], int count) {
    printf("%s begins: \n", testName);
    printArray("", nums, count);
    
    int* temp = new int[count];
    mergeSort(nums, 0, count - 1, temp);
    delete [] temp;
    
    printArray("", nums, count);
}

void MergeSort::test1() {
    int nums[] = {4, 6, 1, 2, 9, 10, 20, 1, 1};
    test("test1", nums, 8);
}

void MergeSort::test2() {
    int nums[] = {1, 2, 3, 4, 5, 6, 7, 8};
    test("test2", nums, 8);
}

void MergeSort::test3() {
    int nums[] = {5, 4, 3, 2, 1};
    test("test3", nums, 5);
}

void MergeSort::test4() {
    int nums[] = {-4, 6, -2, 9, 10, -20, 1, -1};
    test("test4", nums, 8);
}

void MergeSort::test5() {
    int nums[2] = {4, 2};
    test("test5", nums, 2);
}

void MergeSort::test6() {
    int nums[] = {};
    test("test5", nums, 0);
}

void MergeSort::Test() {
    test1();
    test2();
    test3();
    test4();
    test5();
    test6();
}
