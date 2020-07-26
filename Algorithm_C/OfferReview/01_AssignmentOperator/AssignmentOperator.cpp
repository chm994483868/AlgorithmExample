//
//  AssignmentOperator.cpp
//  OfferReview
//
//  Created by HM C on 2020/7/26.
//  Copyright © 2020 CHM. All rights reserved.
//

#include "AssignmentOperator.hpp"

namespace AssignmentOperator {

// 1: 赋值运算符函数
// 题目：如下为类型CMyString的声明，请为该类型添加赋值运算符函数。

class CMyString {
public:
    CMyString(char* pData = nullptr); // 构造函数，pData 参数默认为 nullptr
    CMyString(const CMyString& str); // 复制构造函数，该类型的引用传值
    ~CMyString(void); // 析构函数
    
    CMyString& operator = (const CMyString& str);
    
private:
    char* m_pData;
};

CMyString::CMyString(char* pData) {
    
}

}


