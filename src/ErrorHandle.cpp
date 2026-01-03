/**
 * @file ErrorHandle.cpp
 * @brief 增强型错误处理器实现
 * @details 实现类似Clang风格的专业错误诊断输出
 */

#include <ErrorHandle.hpp>

// 错误处理器全局实例
ErrorHandle errorHandle;

/* ============================================================
 *                     私有辅助方法
 * ============================================================ */

/**
 * @brief 设置控制台输出颜色
 * @param color 颜色代码
 */
void ErrorHandle::setColor(ConsoleColor color)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

/**
 * @brief 重置控制台颜色为默认
 */
void ErrorHandle::resetColor()
{
    setColor(COLOR_DEFAULT);
}

/**
 * @brief 获取源代码的指定行
 * @param lineNum 行号(从1开始)
 * @return 该行的源代码内容
 */
wstring ErrorHandle::getSourceLine(size_t lineNum)
{
    if (lineNum == 0) return L"";
    
    wstring line = L"";
    size_t currentLine = 1;
    size_t pos = 0;
    
    // 遍历源程序找到对应行
    while (true) {
        wchar_t ch = readUnicode.getProgmWStr(pos);
        if (ch == L'\0' || ch == L'#') break;
        
        if (currentLine == lineNum) {
            if (ch == L'\n') break;
            line += ch;
        } else if (ch == L'\n') {
            currentLine++;
            if (currentLine > lineNum) break;
        }
        pos++;
    }
    
    return line;
}

/**
 * @brief 生成位置指示器
 * @param col 起始列(从1开始)
 * @param length 高亮长度
 * @return 包含空格和^符号的字符串
 */
wstring ErrorHandle::generatePointer(size_t col, size_t length)
{
    wstring pointer = L"";
    
    // 添加前导空格
    for (size_t i = 1; i < col; ++i) {
        pointer += L' ';
    }
    
    // 添加^符号
    if (length == 0) length = 1;
    for (size_t i = 0; i < length; ++i) {
        pointer += L'^';
    }
    
    return pointer;
}

/**
 * @brief 打印错误级别标签
 * @param level 错误级别
 */
void ErrorHandle::printErrorLevel(ErrorLevel level)
{
    switch (level) {
    case LEVEL_NOTE:
        setColor(COLOR_CYAN);
        wcout << L"note: ";
        break;
    case LEVEL_WARNING:
        setColor(COLOR_YELLOW);
        wcout << L"warning: ";
        warnCnt++;
        break;
    case LEVEL_ERROR:
        setColor(COLOR_RED);
        wcout << L"error: ";
        errCnt++;
        break;
    case LEVEL_FATAL:
        setColor(COLOR_RED);
        wcout << L"fatal error: ";
        errCnt++;
        break;
    }
    resetColor();
}

/**
 * @brief 打印位置信息
 * @param row 行号
 * @param col 列号
 */
void ErrorHandle::printLocation(size_t row, size_t col)
{
    setColor(COLOR_WHITE);
    if (!currentFileName.empty()) {
        wcout << currentFileName << L":";
    }
    wcout << row << L":" << col << L": ";
    resetColor();
}

/**
 * @brief 打印源码片段和位置指示器
 * @param row 行号
 * @param col 列号
 * @param highlightLen 高亮长度
 */
void ErrorHandle::printSourceSnippet(size_t row, size_t col, size_t highlightLen)
{
    wstring sourceLine = getSourceLine(row);
    if (sourceLine.empty()) return;
    
    // 打印行号
    setColor(COLOR_CYAN);
    wcout << L"   " << row << L" | ";
    resetColor();
    
    // 打印源代码行，高亮错误位置
    for (size_t i = 0; i < sourceLine.length(); ++i) {
        if (i + 1 >= col && i + 1 < col + highlightLen) {
            setColor(COLOR_RED);
            wcout << sourceLine[i];
            resetColor();
        } else {
            wcout << sourceLine[i];
        }
    }
    wcout << endl;
    
    // 打印位置指示器
    setColor(COLOR_CYAN);
    wcout << L"     | ";
    setColor(COLOR_GREEN);
    wcout << generatePointer(col, highlightLen) << endl;
    resetColor();
}

/**
 * @brief 格式化并输出完整的错误信息
 */
void ErrorHandle::printFormattedError(ErrorLevel level, const wchar_t* msg, 
                                       size_t row, size_t col, size_t highlightLen,
                                       const wchar_t* suggestion)
{
    // 打印位置信息
    printLocation(row, col);
    
    // 打印错误级别
    printErrorLevel(level);
    
    // 打印错误消息
    setColor(COLOR_WHITE);
    wcout << msg << endl;
    resetColor();
    
    // 打印源码片段
    printSourceSnippet(row, col, highlightLen);
    
    // 打印修复建议
    if (suggestion != nullptr && wcslen(suggestion) > 0) {
        setColor(COLOR_CYAN);
        wcout << L"     | ";
        setColor(COLOR_GREEN);
        wcout << L"hint: " << suggestion << endl;
        resetColor();
    }
    
    wcout << endl;
}

/* ============================================================
 *                     公共接口方法
 * ============================================================ */

/**
 * @brief 初始化错误处理器
 */
void ErrorHandle::InitErrorHandle()
{
    errCnt = 0;
    warnCnt = 0;
    currentFileName = L"";
    
    // 初始化错误信息模板
    errMsg[MISSING] = L"missing %s";
    errMsg[UNDECLARED_IDENT] = L"use of undeclared identifier '%s'";
    errMsg[UNDECLARED_PROC] = L"use of undeclared procedure '%s'";
    errMsg[ILLEGAL_DEFINE] = L"invalid %s";
    errMsg[ILLEGAL_WORD] = L"invalid token %s";
    errMsg[ILLEGAL_RVALUE_ASSIGN] = L"expression is not assignable";
    errMsg[EXPECT] = L"expected %s";
    errMsg[EXPECT_STH_FIND_ANTH] = L"expected %s, but found %s";
    errMsg[REDUNDENT] = L"extraneous %s";
    errMsg[INCOMPATIBLE_VAR_LIST] = L"argument count mismatch";
    errMsg[UNDEFINED_PROC] = L"call to undefined procedure '%s'";
    errMsg[SYNTAX_ERROR] = L"%s; expected %s";
}

/**
 * @brief 设置当前文件名
 * @param filename 文件名
 */
void ErrorHandle::SetFileName(const wstring& filename)
{
    currentFileName = filename;
}

/**
 * @brief 报告错误(无额外参数) - 兼容旧接口
 */
void ErrorHandle::error(const unsigned int n, const size_t preWordRow, 
                        const size_t preWordCol, const size_t rowPos, const size_t colPos)
{
    wchar_t msg[256] = L"";
    swprintf_s(msg, sizeof(msg) / sizeof(wchar_t), errMsg[n].c_str());
    
    size_t row = rowPos, col = colPos;
    if (n == ILLEGAL_RVALUE_ASSIGN || n == INCOMPATIBLE_VAR_LIST) {
        row = preWordRow;
        col = preWordCol;
    }
    
    printFormattedError(LEVEL_ERROR, msg, row, col);
}

/**
 * @brief 报告错误(带一个额外参数) - 兼容旧接口
 */
void ErrorHandle::error(const unsigned int n, const wchar_t* extra,
                        const size_t preWordRow, const size_t preWordCol, 
                        const size_t rowPos, const size_t colPos)
{
    wchar_t msg[256] = L"";
    swprintf_s(msg, sizeof(msg) / sizeof(wchar_t), errMsg[n].c_str(), extra);
    
    size_t row = rowPos, col = colPos;
    size_t highlightLen = wcslen(extra);
    
    // MISSING 应该指向当前位置（表示缺失的符号应该在这之前出现）
    if (n == MISSING) {
        // 对于 MISSING 错误，使用前一行/前一位置，指向应该插入符号的地方
        // 如果当前位置在行首（col==1），说明跨行了，应该用前一个词的行尾
        if (colPos == 1 || rowPos != preWordRow) {
            row = preWordRow;
            // 指向前一个词之后的位置（前一个词结束处）
            col = preWordCol + 1;  // 前一词后一个位置
        } else {
            // 同一行，指向当前词之前
            row = rowPos;
            col = colPos > 1 ? colPos - 1 : colPos;
        }
        highlightLen = 1;  // 缺失符号只高亮1个位置
    }
    else
    {
        row = rowPos;
        col = colPos;
    }
    
    // 生成修复建议
    const wchar_t* suggestion = nullptr;
    if (n == MISSING) {
        static wchar_t suggestionBuf[256];
        swprintf_s(suggestionBuf, 256, L"Expected '%s' here", extra);
        suggestion = suggestionBuf;
    }
    else if(n == UNDECLARED_IDENT||UNDECLARED_PROC)
    {
        static wchar_t suggestionBuf[256];
        swprintf_s(suggestionBuf, 256, L"Declare '%s' first", extra);
        suggestion = suggestionBuf;
    }
    else if(n == ILLEGAL_DEFINE||n == ILLEGAL_WORD)
    {
        static wchar_t suggestionBuf[256];
        swprintf_s(suggestionBuf, 256, L"Please check the '%s'", extra);
        suggestion = suggestionBuf;
    }
    else if(n == EXPECT)
    {
        static wchar_t suggestionBuf[256];
        swprintf_s(suggestionBuf, 256, L"Expected '%s' here", extra);
        suggestion = suggestionBuf;
    }
    else if(n == REDUNDENT)
    {
        static wchar_t suggestionBuf[256];
        swprintf_s(suggestionBuf, 256, L"Remove '%s' here", extra);
        suggestion = suggestionBuf;
    }
    else if(n == UNDEFINED_PROC)
    {
        static wchar_t suggestionBuf[256];
        swprintf_s(suggestionBuf, 256, L"Define '%s' first", extra);
        suggestion = suggestionBuf;
    }
    printFormattedError(LEVEL_ERROR, msg, row, col, highlightLen > 0 ? highlightLen : 1, suggestion);
}

/**
 * @brief 报告错误(带两个额外参数) - 兼容旧接口
 */
void ErrorHandle::error(const unsigned int n, const wchar_t* extra1, const wchar_t* extra2,
                        const size_t preWordRow, const size_t preWordCol, 
                        const size_t rowPos, const size_t colPos)
{
    wchar_t msg[256] = L"";
    swprintf_s(msg, sizeof(msg) / sizeof(wchar_t), errMsg[n].c_str(), extra1, extra2);
    size_t highlightLen = wcslen(extra2);
    size_t row = rowPos, col = colPos;

    // 生成修复建议
    const wchar_t* suggestion = nullptr;
    if (n == EXPECT_STH_FIND_ANTH) {
        static wchar_t suggestionBuf[256];
        swprintf_s(suggestionBuf, 256, L"Did you mean '%s' instead of '%s'?", extra1, extra2);
        suggestion = suggestionBuf;
    }
    else if(n == SYNTAX_ERROR)
    {
        static wchar_t suggestionBuf[256];
        swprintf_s(suggestionBuf, 256, L"Please check the syntax: '%s'", extra1);
        suggestion = suggestionBuf;
    }
    
    printFormattedError(LEVEL_ERROR, msg, row, col, highlightLen > 0 ? highlightLen : 1, suggestion);
}

/**
 * @brief 增强型错误报告
 * @param level 错误级别
 * @param msg 错误消息
 * @param row 行号
 * @param col 列号
 * @param highlightLen 高亮长度
 * @param suggestion 修复建议
 */
void ErrorHandle::reportError(ErrorLevel level, const wstring& msg, size_t row, size_t col,
                               size_t highlightLen, const wstring& suggestion)
{
    printFormattedError(level, msg.c_str(), row, col, highlightLen,
                        suggestion.empty() ? nullptr : suggestion.c_str());
}

/**
 * @brief 报告警告
 * @param msg 警告消息
 * @param row 行号
 * @param col 列号
 */
void ErrorHandle::warning(const wstring& msg, size_t row, size_t col)
{
    printFormattedError(LEVEL_WARNING, msg.c_str(), row, col);
}

/**
 * @brief 打印编译摘要
 */
void ErrorHandle::printSummary()
{
    wcout << L"─────────────────────────────────────────────────────────" << endl;
    
    if (errCnt == 0 && warnCnt == 0) {
        setColor(COLOR_GREEN);
        wcout << L"✓ ";
        resetColor();
        wcout << L"Build succeeded with no errors or warnings." << endl;
    } else {
        // 统计信息
        if (errCnt > 0) {
            setColor(COLOR_RED);
            wcout << L"✗ ";
            resetColor();
            wcout << errCnt << L" error(s)";
        }
        if (warnCnt > 0) {
            if (errCnt > 0) wcout << L", ";
            setColor(COLOR_YELLOW);
            wcout << L"⚠ ";
            resetColor();
            wcout << warnCnt << L" warning(s)";
        }
        wcout << L" generated." << endl;
    }
    
    wcout << L"─────────────────────────────────────────────────────────" << endl;
}

/**
 * @brief 输出编译结束信息
 */
void ErrorHandle::over()
{
    wcout << endl;
    printSummary();
    
    if (errCnt == 0) {
        setColor(COLOR_GREEN);
        wcout << L"Compilation successful!" << endl;
        resetColor();
    } else {
        setColor(COLOR_RED);
        wcout << L"Compilation failed." << endl;
        resetColor();
    }
    wcout << endl;
}
