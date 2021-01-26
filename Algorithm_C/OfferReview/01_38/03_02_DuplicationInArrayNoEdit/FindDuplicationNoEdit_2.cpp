//
//  FindDuplicationNoEdit_2.cpp
//  OfferReview
//
//  Created by CHM on 2021/1/26.
//  Copyright © 2021 CHM. All rights reserved.
//

#include "FindDuplicationNoEdit_2.hpp"

int FindDuplicationNoEdit_2::countRange(const int* nums, int count, int start, int end) {
    if (nums == nullptr) {
        return 0;
    }
    
    int result = 0;
    for (int i = 0; i < count; ++i) {
        if (nums[i] >= start && nums[i] <= end) {
            ++result;
        }
    }
    
    return result;
}

int FindDuplicationNoEdit_2::getDuplication(const int* nums, int length) {
    if (nums == nullptr || length <= 0) {
        return -1;
    }
    
    int start = 1;
    int end = length - 1;
    
    while (start <= end) {
        int mid = ((end - start) >> 1) + start;
        int count = countRange(nums, length, start, mid);
        
        if (start == end) {
            if (count > 1) {
                return start;
            } else {
                break;
            }
        }
        
        if (count > (mid - start + 1)) {
            end = mid;
        } else {
            start = mid + 1;
        }
    }
    
    return -1;
}
