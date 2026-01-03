/**
 * @file Lexer.hpp
 * @brief 词法分析器模块
 * @details 实现对源程序的词法分析，识别各类词法单元
 */

#ifndef _LEXER_HPP
#define _LEXER_HPP

#include <Types.hpp>
#include <ErrorHandle.hpp>
using namespace std;

/**
 * @class Lexer
 * @brief 词法分析器
 * @details 逐字符扫描源程序，识别并返回词法单元(Token)
 */
class Lexer
{
private:
    wchar_t ch;                   // 当前读入的字符
    unsigned long tokenType;      // 当前识别的词法单元类型
    wstring strToken;             // 当前词法单元的字符串值
    size_t nowPtr;                // 当前字符在源程序中的位置
    size_t rowPos;                // 当前行号
    size_t colPos;                // 当前列号
    size_t preWordRow;            // 上一合法词法单元的结束行号
    size_t preWordCol;            // 上一合法词法单元的结束列号

    unordered_map<unsigned long, wstring> sym_map;  // 词法单元类型到字符串的映射

    // 保留字表
    wstring resv_table[RSV_WORD_MAX] = {
        L"odd", L"begin", L"end", L"if", L"then", L"while", L"do", L"call",
        L"const", L"var", L"procedure", L"write", L"read", L"program", L"else"
    };

    // 运算符表
    wchar_t opr_table[OPR_MAX] = {
        L'+', L'-', L'*', L'/', L'=', L'<', L'>', L'(', L')', L',', L';'
    };

    bool IsDigit();       // 判断当前字符是否为数字
    bool IsLetter();      // 判断当前字符是否为字母
    bool IsBoundary();    // 判断当前字符是否为界符
    int IsOperator();     // 判断当前字符是否为运算符
    void GetBC();         // 跳过空白字符
    void GetChar();       // 读取下一个字符
    void Retract();       // 回退一个字符
    int Reserve();        // 查找保留字表
    void Concat();        // 将当前字符追加到strToken

public:
    void GetWord();                                   // 获取下一个词法单元
    void InitLexer();                                 // 初始化词法分析器
    wchar_t GetCh();                                  // 获取当前字符
    size_t GetPreWordCol() { return preWordCol; };    // 获取上一词法单元列号
    size_t GetPreWordRow() { return preWordRow; };    // 获取上一词法单元行号
    size_t GetColPos() { return colPos; };            // 获取当前列号
    size_t GetRowPos() { return rowPos; };            // 获取当前行号
    wstring GetStrToken() { return strToken; };       // 获取当前词法单元字符串
    unsigned long GetTokenType() { return tokenType; }; // 获取当前词法单元类型
};

extern Lexer lexer;

#endif
