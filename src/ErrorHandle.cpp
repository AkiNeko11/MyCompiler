/**
 * @file ErrorHandle.cpp
 * @brief 错误处理器实现
 * @details 实现编译过程中的错误检测、记录与格式化输出
 */

#include <ErrorHandle.hpp>

/**
 * @brief 初始化错误处理器
 * @details 重置错误计数，初始化所有错误信息模板
 */
void ErrorHandle::InitErrorHandle()
{
    errCnt = 0;
    
    // 缺失类错误
    errMsg[MISSING] = L"Missing %s";
    errMsg[MISSING_DETAILED] = L"Missing %s. Details: %s";
    
    // 未声明错误
    errMsg[UNDECLARED_IDENT] = L"Undeclared identifier '%s'";
    errMsg[UNDECLARED_PROC] = L"Undeclared procedure name '%s'";
    
    // 重复声明错误
    errMsg[REDECLEARED_IDENT] = L"Redecleared identifier '%s'";
    errMsg[REDECLEARED_PROC] = L"Redecleared procedure name '%s'";
    
    // 非法定义错误
    errMsg[ILLEGAL_DEFINE] = L"Illegal %s definition ";
    errMsg[ILLEGAL_WORD] = L"Illegal word %s";
    errMsg[ILLEGAL_RVALUE_ASSIGN] = L"Cannot assign a rvalue";
    
    // 期望类错误
    errMsg[EXPECT] = L"Expecting %s";
    errMsg[EXPECT_STH_FIND_ANTH] = L"Expecting %s but %s was found";
    
    // 冗余错误
    errMsg[REDUNDENT] = L"Redundent %s";
    
    // 其他语义错误
    errMsg[INCOMPATIBLE_VAR_LIST] = L"The real variable list is incompatible with formal variable list";
    errMsg[UNDEFINED_PROC] = L"Calling undefined procedure '%s'";

    // 语法错误
    errMsg[SYNTAX_ERROR] = L"Syntax Error: %s. Expected: %s.";
    errMsg[INVALID_SYNTAX] = L"Invalid syntax near '%s'. Details: %s";
    errMsg[UNEXPECTED_TOKEN] = L"Unexpected token '%s'. Expected: %s";
}

/**
 * @brief 输出上一词法单元位置的错误信息
 * @param msg 格式化后的错误信息
 * @param preWordRow 上一词法单元行号
 * @param preWordCol 上一词法单元列号
 */
void ErrorHandle::printPreWord(const wchar_t msg[], const size_t preWordRow, const size_t preWordCol)
{
    wcout << L"(" << preWordRow << "," << preWordCol << L")"
          << L" Error: " << msg << endl;
}

/**
 * @brief 输出当前词法单元位置的错误信息
 * @param msg 格式化后的错误信息
 * @param rowPos 当前行号
 * @param colPos 当前列号
 */
void ErrorHandle::printCurWord(const wchar_t msg[], const size_t rowPos, const size_t colPos)
{
    wcout << L"(" << rowPos << "," << colPos << L")"
          << L" Error: " << msg << endl;
}

/**
 * @brief 报告错误(无额外参数)
 * @param n 错误类型编码
 * @param preWordRow 上一词法单元行号
 * @param preWordCol 上一词法单元列号
 * @param rowPos 当前行号
 * @param colPos 当前列号
 */
void ErrorHandle::error(const unsigned int n, const size_t preWordRow, 
                        const size_t preWordCol, const size_t rowPos, const size_t colPos)
{
    wchar_t msg[200] = L"";
    swprintf_s(msg, sizeof(msg) / sizeof(wchar_t), errMsg[n].c_str());
    errCnt++;
    
    // 根据错误类型选择输出位置
    if (n == REDUNDENT || n == MISSING || n == UNDECLARED_PROC)
        printPreWord(msg, preWordRow, preWordCol);
    else
        printCurWord(msg, rowPos, colPos);
}

/**
 * @brief 报告错误(带一个额外参数)
 * @param n 错误类型编码
 * @param extra 额外的错误信息参数
 * @param preWordRow 上一词法单元行号
 * @param preWordCol 上一词法单元列号
 * @param rowPos 当前行号
 * @param colPos 当前列号
 */
void ErrorHandle::error(const unsigned int n, const wchar_t* extra,
                        const size_t preWordRow, const size_t preWordCol, 
                        const size_t rowPos, const size_t colPos)
{
    wchar_t msg[200] = L"";
    swprintf_s(msg, sizeof(msg) / sizeof(wchar_t), errMsg[n].c_str(), extra);
    errCnt++;
    
    if (n == REDUNDENT || n == MISSING || n == UNDECLARED_PROC)
        printPreWord(msg, preWordRow, preWordCol);
    else
        printCurWord(msg, rowPos, colPos);
}

/**
 * @brief 报告错误(带两个额外参数)
 * @param n 错误类型编码
 * @param extra1 第一个额外参数
 * @param extra2 第二个额外参数
 * @param preWordRow 上一词法单元行号
 * @param preWordCol 上一词法单元列号
 * @param rowPos 当前行号
 * @param colPos 当前列号
 */
void ErrorHandle::error(const unsigned int n, const wchar_t* extra1, const wchar_t* extra2,
                        const size_t preWordRow, const size_t preWordCol, 
                        const size_t rowPos, const size_t colPos)
{
    wchar_t msg[200] = L"";
    swprintf_s(msg, sizeof(msg) / sizeof(wchar_t), errMsg[n].c_str(), extra1, extra2);
    errCnt++;
    
    if (n == REDUNDENT || n == MISSING || n == UNDECLARED_PROC)
        printPreWord(msg, preWordRow, preWordCol);
    else
        printCurWord(msg, rowPos, colPos);
}

/**
 * @brief 输出编译结束信息
 * @details 根据错误数量输出编译成功或失败的信息
 */
void ErrorHandle::over()
{
    if (errCnt == 0) {
        wcout << L"No error. Congratulations!" << endl;
        wcout << L"______________________________Compile compelete!________________________________\n"
              << endl;
    } else {
        wcout << L"Totol: " << errCnt << L" errors" << endl;
        wcout << L"_______________________________Compile failed!_________________________________\n"
              << endl;
    }
}

// 错误处理器全局实例
ErrorHandle errorHandle;
