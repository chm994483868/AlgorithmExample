//
//  RecursiveSummary.cpp
//  Algorithm_C
//
//  Created by CHM on 2020/7/21.
//  Copyright © 2020 CHM. All rights reserved.
//

#include "RecursiveSummary.hpp"

// 递归相关题目总结

// 06. 快速排序 O(n*logn)
void quickSort(int nums[], int left, int right) {
    if (nums == nullptr) {
        return;
    }
    
    if (left >= right) {
        return;
    }
    
    int i = left, j = right, x = nums[left];
    
    while (i < j) {
        while (i < j && nums[j] >= x) {
            --j;
        }
        
        if (i < j) {
            nums[i++] = nums[j];
        }
        
        while (i < j && nums[i] < x) {
            ++i;
        }
        
        if (i < j) {
            nums[j--] = nums[i];
        }
    }
    
    nums[i] = x;
    
    quickSort(nums, left, i - 1);
    quickSort(nums, i + 1, right);
}
