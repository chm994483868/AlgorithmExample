//
//  AssignmentOperator_3.cpp
//  OfferReview
//
//  Created by CHM on 2021/1/26.
//  Copyright © 2021 CHM. All rights reserved.
//

#include "AssignmentOperator_3.hpp"

AssignmentOperator_3::CMyString::CMyString(const char* pData) {
    if (pData == nullptr) {
        m_pData = new char[1];
        m_pData[0] = '\0';
    } else {
        unsigned long length = strlen(pData);
        m_pData = new char[length + 1];
        strcpy(m_pData, pData);
    }
}

AssignmentOperator_3::CMyString::CMyString(const CMyString& str) {
    unsigned long length = strlen(str.m_pData);
    m_pData = new char[length + 1];
    strcpy(m_pData, str.m_pData);
}

AssignmentOperator_3::CMyString::~CMyString() {
    delete [] m_pData;
}

AssignmentOperator_3::CMyString& AssignmentOperator_3::CMyString::operator = (const CMyString& str) {
//    if (this == &str) {
//        return *this;
//    }
//
//    delete [] m_pData;
//    m_pData = nullptr;
//
//    m_pData = new char[strlen(str.m_pData) + 1];
//    strcpy(m_pData, str.m_pData);
    
    if (this != &str) {
        CMyString strTemp = CMyString(str);
        char* pTemp = strTemp.m_pData;
        strTemp.m_pData = m_pData;
        m_pData = pTemp;
    }
    
    return *this;
}


