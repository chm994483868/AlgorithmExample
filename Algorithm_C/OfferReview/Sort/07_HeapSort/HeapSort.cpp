//
//  HeapSort.cpp
//  OfferReview
//
//  Created by CHM on 2020/8/4.
//  Copyright © 2020 CHM. All rights reserved.
//

#include "HeapSort.hpp"

void HeapSort::heapSort(int nums[], int count) {
    if (nums == nullptr || count <= 0) {
        return;
    }
    
    int i = (count - 1 - 1) / 2;
    // 建堆
    for (; i >= 0; --i) {
        maxHeapFixdown(nums, i, count);
    }
    
    // 排序
    for (i = count - 1; i >= 1; --i) {
        swap(&nums[0], &nums[i]);
        maxHeapFixdown(nums, 0, i);
    }
}

void HeapSort::maxHeapFixdown(int nums[], int i, int n) {
    // 1. 左子节点下标
    int j = i * 2 + 1;
    int temp = nums[i];
    
    while (j < n) {
        // 2. 找到左右子节点中的最大值
        if (j + 1 < n && nums[j + 1] > nums[j]) {
            ++j;
        }
        
        // 3. 如果父节点的值大于等于左右子节点的值，则不用交换
        if (nums[j] <= temp) {
            break;
        }
        
        // 4. 把子节点中较大的值赋给父节点
        swap(&nums[i], &nums[j]);
        
        // 5. 更新 i 和 j
        i = j;
        j = i * 2 + 1;
    }
}

// 测试代码
void HeapSort::test(char* testName, int nums[], int count) {
    printf("%s begins: \n", testName);
    printArray("", nums, count);
    heapSort(nums, count);
    printArray("", nums, count);
}

void HeapSort::test1() {
    int nums[] = {4, 6, 1, 2, 9, 10, 20, 1, 1};
    test("test1", nums, 8);
}

void HeapSort::test2() {
    int nums[] = {1, 2, 3, 4, 5, 6, 7, 8};
    test("test2", nums, 8);
}

void HeapSort::test3() {
    int nums[] = {5, 4, 3, 2, 1};
    test("test3", nums, 5);
}

void HeapSort::test4() {
    int nums[] = {-4, 6, -2, 9, 10, -20, 1, -1};
    test("test4", nums, 8);
}

void HeapSort::test5() {
    int nums[2] = {4, 2};
    test("test5", nums, 2);
}

void HeapSort::test6() {
    int nums[] = {};
    test("test5", nums, 0);
}

void HeapSort::Test() {
    test1();
    test2();
    test3();
    test4();
    test5();
    test6();
}