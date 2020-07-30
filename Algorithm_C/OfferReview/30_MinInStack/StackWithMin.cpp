//
//  StackWithMin.cpp
//  OfferReview
//
//  Created by CHM on 2020/7/30.
//  Copyright © 2020 CHM. All rights reserved.
//

#include "StackWithMin.hpp"

template <typename T>
T& StackWithMin::StackWithMin<T>::top() {
    return m_data.top();
}

template <typename T>
const T& StackWithMin::StackWithMin<T>::top() const {
    return m_data.top();
}

template <typename T>
void StackWithMin::StackWithMin<T>::push(const T& value) {
    m_data.push(value);
    
    if (m_min.empty() || value < m_min.top()) {
        m_min.push(value);
    } else {
        m_min.push(m_min.top());
    }
}

template <typename T>
void StackWithMin::StackWithMin<T>::pop() {
    assert(m_data.size() > 0 && m_min.size() > 0);
    
    m_data.pop();
    m_min.pop();
}

template <typename T>
const T& StackWithMin::StackWithMin<T>::min() const {
    assert(m_data.size() > 0 && m_min.size() > 0);
    
    return m_min.top();
}

template <typename T>
bool StackWithMin::StackWithMin<T>::empty() const {
    return m_data.empty();
}

template <typename T>
size_t StackWithMin::StackWithMin<T>::size() const {
    return m_data.size();
}

// 测试代码
void StackWithMin::Test(const char* testName, const StackWithMin<int>& stack, int expected) {
    if(testName != nullptr)
        printf("%s begins: ", testName);

    if(stack.min() == expected)
        printf("Passed.\n");
    else
        printf("Failed.\n");
}

void StackWithMin::Test() {
    StackWithMin<int> stack;

    stack.push(3);
    Test("Test1", stack, 3);

    stack.push(4);
    Test("Test2", stack, 3);

    stack.push(2);
    Test("Test3", stack, 2);

    stack.push(3);
    Test("Test4", stack, 2);

    stack.pop();
    Test("Test5", stack, 2);

    stack.pop();
    Test("Test6", stack, 3);

    stack.pop();
    Test("Test7", stack, 3);

    stack.push(0);
    Test("Test8", stack, 0);
}
