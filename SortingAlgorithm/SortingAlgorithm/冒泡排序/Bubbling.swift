//
//  Bubbling.swift
//  SortingAlgorithm
//
//  Created by HM C on 2019/12/25.
//  Copyright © 2019 HM C. All rights reserved.
//

import Foundation

func bubbling<T>(_ nums: inout [T]) where T: Comparable {
    for i in 0..<(nums.count - 1) {
        var isChange = false
        for j in 0..<(nums.count - i - 1) {
            if nums[j] > nums[j + 1] {
                let temporary = nums[j]
                nums[j] = nums[j + 1]
                nums[j + 1] = temporary
                
                isChange = true
            }
        }
        
        if !isChange { break }
    }
}

func bubbling1<T>(_ nums1: [T]) -> [T] where T: Comparable {
    var nums = nums1
    for i in 0..<(nums.count - 1) {
        var isChange = false
        for j in 0..<(nums.count - i - 1) {
            if nums[j] > nums[j + 1] {
                let temporary = nums[j]
                nums[j] = nums[j + 1]
                nums[j + 1] = temporary
                
                isChange = true
            }
        }
        
        if !isChange { break }
    }
    
    return nums
}

public extension Array where Element: Comparable {
     mutating func bubbling() {
        for i in 0..<(count - 1) {
            var isChange = false
            for j in 0..<(count - i - 1) {
                if self[j] > self[j + 1] {
                    let temporary = self[j]
                    self[j] = self[j + 1]
                    self[j + 1] = temporary
                    
                    isChange = true
                }
            }
            
            if !isChange { break }
        }
    }
}
