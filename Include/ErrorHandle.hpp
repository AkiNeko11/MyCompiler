/**
 * @file ErrorHandle.hpp
 * @brief 错误处理模块
 * @details 负责编译过程中的错误检测、记录与输出
 */

#ifndef _ERROR_HANDLE_HPP
#define _ERROR_HANDLE_HPP

#include <Lexer.hpp>
#include <Types.hpp>

/**
 * @class ErrorHandle
 * @brief 错误处理器
 * @details 管理编译过程中产生的所有错误信息，支持多种错误格式的输出
 */
class ErrorHandle
{
private:
    unsigned int errCnt;          // 错误计数
    wstring errMsg[ERR_CNT];      // 错误信息模板表

    // 输出上一个词法单元的位置信息
    void printPreWord(const wchar_t msg[], const size_t preWordRow, const size_t preWordCol);
    // 输出当前词法单元的位置信息
    void printCurWord(const wchar_t msg[], const size_t rowPos, const size_t colPos);
    
public:
    // 报告错误(无额外信息)
    void error(const unsigned int n, const size_t preWordRow, const size_t preWordCol, 
               const size_t rowPos, const size_t colPos);
    // 报告错误(带一个额外信息)
    void error(const unsigned int n, const wchar_t*, const size_t preWordRow, 
               const size_t preWordCol, const size_t rowPos, const size_t colPos);
    // 报告错误(带两个额外信息)
    void error(const unsigned int n, const wchar_t*, const wchar_t*, const size_t preWordRow, 
               const size_t preWordCol, const size_t rowPos, const size_t colPos);
    
    void InitErrorHandle();                       // 初始化错误处理器
    unsigned int GetError() { return errCnt; };   // 获取错误数量
    void over();                                  // 输出编译结束信息
};

extern ErrorHandle errorHandle;

#endif
