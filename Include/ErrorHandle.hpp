/**
 * @file ErrorHandle.hpp
 * @brief 增强型错误处理模块
 * @details 提供类似Clang风格的专业错误诊断，支持彩色输出、
 *          源码显示、位置指示和修复建议
 */

#ifndef _ERROR_HANDLE_HPP
#define _ERROR_HANDLE_HPP

#include <Lexer.hpp>
#include <Types.hpp>

/* ============ 控制台颜色定义 ============ */
enum ConsoleColor {
    COLOR_DEFAULT = 7,      // 默认白色
    COLOR_RED = 12,         // 红色(错误)
    COLOR_YELLOW = 14,      // 黄色(警告)
    COLOR_GREEN = 10,       // 绿色(成功)
    COLOR_CYAN = 11,        // 青色(信息)
    COLOR_MAGENTA = 13,     // 紫色(注释)
    COLOR_WHITE = 15        // 亮白色(高亮)
};

/* ============ 错误级别定义 ============ */
enum ErrorLevel {
    LEVEL_NOTE,             // 注释/提示
    LEVEL_WARNING,          // 警告
    LEVEL_ERROR,            // 错误
    LEVEL_FATAL             // 致命错误
};

/**
 * @class ErrorHandle
 * @brief 增强型错误处理器
 * @details 管理编译过程中产生的所有错误信息，提供专业的诊断输出
 */
class ErrorHandle
{
private:
    unsigned int errCnt;              // 错误计数
    unsigned int warnCnt;             // 警告计数
    wstring errMsg[ERR_CNT];          // 错误信息模板表
    wstring currentFileName;          // 当前编译的文件名
    
    // 控制台颜色控制
    void setColor(ConsoleColor color);
    void resetColor();
    
    // 获取源代码行
    wstring getSourceLine(size_t lineNum);
    
    // 生成位置指示器 (^^^^)
    wstring generatePointer(size_t col, size_t length = 1);
    
    // 打印带颜色的错误级别标签
    void printErrorLevel(ErrorLevel level);
    
    // 打印位置信息
    void printLocation(size_t row, size_t col);
    
    // 打印源码片段和指示器
    void printSourceSnippet(size_t row, size_t col, size_t highlightLen = 1);
    
    // 格式化并输出错误信息
    void printFormattedError(ErrorLevel level, const wchar_t* msg, 
                             size_t row, size_t col, size_t highlightLen = 1,
                             const wchar_t* suggestion = nullptr);

public:
    void InitErrorHandle();
    void SetFileName(const wstring& filename);
    
    // 标准错误报告接口(保持向后兼容)
    void error(const unsigned int n, const size_t preWordRow, const size_t preWordCol, 
               const size_t rowPos, const size_t colPos);
    void error(const unsigned int n, const wchar_t*, const size_t preWordRow, 
               const size_t preWordCol, const size_t rowPos, const size_t colPos);
    void error(const unsigned int n, const wchar_t*, const wchar_t*, const size_t preWordRow, 
               const size_t preWordCol, const size_t rowPos, const size_t colPos);
    
    // 增强型错误报告接口
    void reportError(ErrorLevel level, const wstring& msg, size_t row, size_t col,
                     size_t highlightLen = 1, const wstring& suggestion = L"");
    
    // 警告报告
    void warning(const wstring& msg, size_t row, size_t col);
    
    // 统计信息
    unsigned int GetErrorCount() { return errCnt; }
    unsigned int GetWarningCount() { return warnCnt; }
    unsigned int GetError() { return errCnt; }  // 兼容旧接口
    
    // 输出编译结束信息
    void over();
    
    // 打印编译摘要
    void printSummary();
};

extern ErrorHandle errorHandle;

#endif
