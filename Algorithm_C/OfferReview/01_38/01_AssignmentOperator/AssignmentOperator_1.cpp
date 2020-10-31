//
//  AssignmentOperator_1.cpp
//  OfferReview
//
//  Created by HM C on 2020/10/31.
//  Copyright © 2020 CHM. All rights reserved.
//

#include "AssignmentOperator_1.hpp"

AssignmentOperator_1::CMyString::CMyString(const char* pData) {
    if (pData == nullptr) {
        m_pData = new char[1];
        m_pData[0] = '\0';
    } else {
        unsigned long length = strlen(pData);
        m_pData = new char[length + 1];
        strcpy(m_pData, pData);
    }
}

AssignmentOperator_1::CMyString::CMyString(const CMyString& str) {
    unsigned long length = strlen(str.m_pData);
    m_pData = new char[length + 1];
    strcpy(m_pData, str.m_pData);
}

AssignmentOperator_1::CMyString::~CMyString() {
    delete [] m_pData;
}

AssignmentOperator_1::CMyString& AssignmentOperator_1::CMyString::operator=(const CMyString& str) {
    
    // 一：初级解法
    if ()
}
