//
//  InsertionSort.swift
//  SortingAlgorithm
//
//  Created by CHM on 2019/12/31.
//  Copyright © 2019 HM C. All rights reserved.
//

import Foundation

func insertionSort<T>(_ nums: inout [T]) where T: Comparable {
    var preindex = 0
    var current: T
    
    for i in 1..<nums.count {
        preindex = i - 1 // 当前 index 的前一个 index，即已排完序部分的 endIndex
        // 必须记录下这个元素，不然会被覆盖,(如果大于拍完序部分的最大值，则原封不动，如果不是则要移动已排完序部分，为当前元素找位置)
        current = nums[i]
        // 逆序遍历已经排序好的数组
        // 如果当前元素小于排序好的元素，就把排序好的元素往后移动一个位置
        while preindex >= 0 && current < nums[preindex] {
            // 元素向后移动
            nums[preindex + 1] = nums[preindex]
            preindex -= 1
        }
        
        // 找到合适的位置，把当前的元素插入
        nums[preindex + 1] = current
    }
}

func insertionSort1<T>(_ nums1: [T]) -> [T] where T: Comparable {
    var nums = nums1
    
    var preindex = 0
    var current: T
    
    for i in 1..<nums.count {
        preindex = i - 1 // 当前 index 的前一个 index，即已排完序部分的 endIndex
        // 必须记录下这个元素，不然会被覆盖,(如果大于拍完序部分的最大值，则原封不动，如果不是则要移动已排完序部分，为当前元素找位置)
        current = nums[i]
        // 逆序遍历已经排序好的数组
        // 如果当前元素小于排序好的元素，就把排序好的元素往后移动一个位置
        while preindex >= 0 && current < nums[preindex] {
            // 元素向后移动
            nums[preindex + 1] = nums[preindex]
            preindex -= 1
        }
        
        // 找到合适的位置，把当前的元素插入
        nums[preindex + 1] = current
    }
    
    return nums
}

public extension Array where Element: Comparable {
    mutating func insertionSort() {
        var preindex = 0
        var current: Element
        
        for i in 1..<self.count {
            preindex = i - 1 // 当前 index 的前一个 index，即已排完序部分的 endIndex
            // 必须记录下这个元素，不然会被覆盖,(如果大于拍完序部分的最大值，则原封不动，如果不是则要移动已排完序部分，为当前元素找位置)
            current = self[i]
            // 逆序遍历已经排序好的数组
            // 如果当前元素小于排序好的元素，就把排序好的元素往后移动一个位置
            while preindex >= 0 && current < self[preindex] {
                // 元素向后移动
                self[preindex + 1] = self[preindex]
                preindex -= 1
            }
            
            // 找到合适的位置，把当前的元素插入
            self[preindex + 1] = current
        }
    }
}
