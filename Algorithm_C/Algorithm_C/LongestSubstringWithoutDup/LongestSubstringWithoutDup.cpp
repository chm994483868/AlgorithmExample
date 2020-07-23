//
//  LongestSubstringWithoutDup.cpp
//  Algorithm_C
//
//  Created by CHM on 2020/7/23.
//  Copyright © 2020 CHM. All rights reserved.
//

#include "LongestSubstringWithoutDup.hpp"

bool hasDuplication(const std::string& str, int position[]);

int longestSubstringWithoutDuplication_1(const std::string& str) {
    int longest = 0;
    int* position = new int[26];
    for (int start = 0; start < str.length(); ++start) {
        for (int end = start; end < str.length(); ++end) {
            int count = end - start + 1;
            const 
        }
    }
}

bool hasDuplication(const std::string& str, int position[]) {
    for (int i = 0; i < 26; ++i) {
        position[i] = -1;
    }
    
    for (int i = 0; i < str.length(); ++i) {
        int indexInPosition = str[i] - 'a';
        if (position[indexInPosition] >= 0) {
            return true;
        }
        
        position[indexInPosition] = indexInPosition;
    }
    
    return false;
}
