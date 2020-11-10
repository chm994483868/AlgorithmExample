//
//  ReverseWordsInSentence.cpp
//  OfferReview
//
//  Created by CHM on 2020/11/10.
//  Copyright © 2020 CHM. All rights reserved.
//

#include "ReverseWordsInSentence.hpp"

// 翻转字符串，准备两个指针，一个从字符串头部开始，一个从尾部开始，
// 左边指针递增，右边指针递减，交换两个指针指向的字符串
void ReverseWordsInSentence::reverse(char* pBegin, char* pEnd) {
    if (pBegin == nullptr || pEnd == nullptr) {
        return;
    }
    
    // 交换字符
    while (pBegin < pEnd) {
        char temp = *pBegin;
        *pBegin = *pEnd;
        *pEnd = temp;
        
        ++pBegin;
        --pEnd;
    }
}

char* ReverseWordsInSentence::reverseSentence(char* pData) {
    if (pData == nullptr) {
        return nullptr;
    }
    
    char* pBegin = pData;
    char* pEnd = pData;
    while (*pEnd != '\0') {
        ++pEnd;
    }
    --pEnd;
    
    // 翻转整个句子
    reverse(pBegin, pEnd);
    
    // 翻转句子中的每个单词
    pBegin = pEnd = pData;
    while (<#condition#>) {
        <#statements#>
    }
}
