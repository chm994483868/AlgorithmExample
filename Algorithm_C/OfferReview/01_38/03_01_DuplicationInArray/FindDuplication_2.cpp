//
//  FindDuplication_2.cpp
//  OfferReview
//
//  Created by CHM on 2021/1/26.
//  Copyright © 2021 CHM. All rights reserved.
//

#include "FindDuplication_2.hpp"

bool FindDuplication_2::duplicate(int nums[], int count, int* duplication) {
    if (nums == nullptr || count <= 0) {
        return false;
    }
    
    int i = 0;
    for (; i < count; ++i) {
        if (nums[i] < 0 || nums[i] > count - 1) {
            return false;
        }
    }
    
    for (i = 0; i < count; ++i) {
        while (nums[i] != i) {
            if (nums[i] == nums[nums[i]]) {
                *duplication = nums[i];
                return true;
            }
            
            int temp = nums[i];
            nums[i] = nums[nums[i]];
            nums[nums[i]] = temp;
        }
    }
    
    return false;
}
