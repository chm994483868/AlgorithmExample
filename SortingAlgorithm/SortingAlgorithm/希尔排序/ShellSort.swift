//
//  ShellSort.swift
//  SortingAlgorithm
//
//  Created by CHM on 2020/1/2.
//  Copyright © 2020 HM C. All rights reserved.
//

import Foundation

func shellSort<T>(_ nums: inout [T]) where T: Comparable {
    var group = Int(floor(Double(nums.count / 2))) // 每次折半，记录每个分组的长度
    while group > 0 {
        group = Int(floor(Double(group / 2))) // 折半
        
        // 下面完全照搬插入排序
        for i in group..<nums.count { // 从每个分组的起始处开始
            var j = i - group // j 表示前一个元素
            
            while j >= 0 && nums[j] > nums[j + group]  {
                let temp = nums[j]
                nums[j] = nums[j + group]
                nums[j + group] = temp
                
                j -= group
            }
        }
    }
}

func shellSort1<T>(_ nums1:[T]) -> [T] where T: Comparable {
    var nums = nums1
    var group = Int(floor(Double(nums.count / 2))) // 每次折半，记录每个分组的长度
    while group > 0 {
        group = Int(floor(Double(group / 2))) // 折半
        
        // 下面完全照搬插入排序
        for i in group..<nums.count { // 从每个分组的起始处开始
            var j = i - group // j 表示前一个元素
            
            while j >= 0 && nums[j] > nums[j + group]  {
                let temp = nums[j]
                nums[j] = nums[j + group]
                nums[j + group] = temp
                
                j -= group
            }
        }
    }
    
    return nums
}

extension Array where Element: Comparable {
    mutating func shellSort() {
        var group = Int(floor(Double(count / 2))) // 每次折半，记录每个分组的长度
        while group > 0 {
            group = Int(floor(Double(group / 2))) // 折半
            
            // 下面完全照搬插入排序
            for i in group..<count { // 从每个分组的起始处开始
                var j = i - group // j 表示前一个元素
                
                while j >= 0 && self[j] > self[j + group]  {
                    let temp = self[j]
                    self[j] = self[j + group]
                    self[j + group] = temp
                    
                    j -= group
                }
            }
        }
    }
}
