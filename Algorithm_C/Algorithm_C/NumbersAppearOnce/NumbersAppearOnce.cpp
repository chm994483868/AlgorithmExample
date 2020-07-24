//
//  NumbersAppearOnce.cpp
//  Algorithm_C
//
//  Created by CHM on 2020/7/24.
//  Copyright © 2020 CHM. All rights reserved.
//

#include "NumbersAppearOnce.hpp"

unsigned int findFirstBitIs1(int num);
bool isBit1(int num, unsigned int indexBit);

void findNumsAppearOnce(int data[], int length, int* num1, int* num2) {
    if (data == nullptr || length < 2) {
        return;
    }
    
    int resultExclusiveOR = 0;
    for (int i = 0; i < length; ++i) {
        resultExclusiveOR ^= data[i];
    }
}
