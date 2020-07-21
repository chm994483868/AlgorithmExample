//
//  RecursiveSummary.cpp
//  Algorithm_C
//
//  Created by CHM on 2020/7/21.
//  Copyright © 2020 CHM. All rights reserved.
//

#include "RecursiveSummary.hpp"

// 06. 快速排序 O(n*logn)
void quickSort(int nums[], int l, int r) {
    if (l >= r) {
        return;
    }
    
    int i = l, j = r, x = nums[l];
    while (i < j) {
        while (i < j && nums[j] >= x) j--;
        if (i < j) nums[i++] = nums[j];
        
        while (i < j && nums[i] < x) i++;
        if (i < j) nums[j--] = nums[i];
    }
    
    nums[i] = x;
    
    // 这里的连续递归，是上下参数的值完全无相互影响的递归
    quickSort(nums, l, i - 1);
    quickSort(nums, i + 1, r);
}
