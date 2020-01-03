//
//  QuickSort.swift
//  SortingAlgorithm
//
//  Created by CHM on 2020/1/3.
//  Copyright © 2020 HM C. All rights reserved.
//

import Foundation

/// 快速排序
/// - Parameters:
///   - nums: 待排序序列
///   - leftIndex: 待排序序列左边的 index
///   - rightIndex: 待排序序列右边的 index
func quickSort<T>(_ nums: inout [T], _ leftIndex: Int, _ rightIndex: Int) where T: Comparable {
    var i = leftIndex, j = rightIndex
    // 取中间的一个值作为一个支点
    let pivot = nums[(leftIndex + rightIndex) / 2]
    while i <= j {
        // 向左移动，直到找到大于支点的元素
        while nums[i] < pivot { i += 1 }
        // 向右移动，直到找到小于支点的元素
        while nums[j] > pivot { j -= 1 }
        // 交换两个元素，让左边的大于支点，右边的小于支点
        if i <= j {
            // 如果 i == j，交换个啥
            if i != j {
                let temp = nums[i]
                nums[i] = nums[j]
                nums[j] = temp
            }
            // 交换完以后，i 前移 1 位，j 后移 1 位
            i += 1
            j -= 1
        }
    }
    
    // 递归左边，进行快速排序
    if leftIndex < j {
        quickSort(&nums, leftIndex, j)
    }
    
    // 递归右边，进行快速排序
    if i < rightIndex {
        quickSort(&nums, i, rightIndex)
    }
}
