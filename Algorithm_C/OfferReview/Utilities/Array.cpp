//
//  Array.cpp
//  OfferReview
//
//  Created by HM C on 2020/7/30.
//  Copyright © 2020 CHM. All rights reserved.
//

#include "Array.hpp"

void Partition::swap(int* num1, int* num2) {
    int temp = *num1;
    *num1 = *num2;
    *num2 = temp;
}

int Partition::randomInRange(int min, int max) {
    int random = rand() % (max - min + 1) + min;
    return random;
}

int Partition::partition(int data[], int length, int start, int end) {
    if (data == nullptr || length <= 0 || start < 0 || end >= length) {
        throw std::exception(); // 参数错误
    }
    
    int index = randomInRange(start, end);
    swap(&data[index], &data[end]);
    
    int small = start - 1;
    for (index = start; index < end; ++index) {
        if (data[index] < data[end]) {
            ++small;
            
            if (small != index) {
                swap(&data[small], &data[index]);
            }
        }
    }
    
    ++small;
    if (small != end) {
        swap(&data[small], &data[end]);
    }
    
    return small;
}
