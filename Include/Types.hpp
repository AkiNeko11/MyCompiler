/**
 * @file Types.hpp
 * @brief 编译器通用类型定义与常量声明
 * @details 包含词法分析、语法分析、P-Code生成所需的类型定义、
 *          宏定义、错误码以及全局工具类
 */

#ifndef _TYPES_HPP
#define _TYPES_HPP

#include <locale>
#include <string>
#include <cstddef>
#include <fcntl.h>
#include <fstream>
#include <io.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdint.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <ctime>
#include <unordered_map>
#include <vector>
#include <windows.h>
#include <ostream>

using namespace std;

/* ============================================================
 *                    基本常量定义
 * ============================================================ */
const int RSV_WORD_MAX = 15;    // 保留字总数
const int OPR_MAX = 11;         // 运算符总数
const int ERR_CNT = 70;         // 错误类型总数

/* ============================================================
 *                    错误类型编码
 * ============================================================ */
#define EXPECT_STH_FIND_ANTH 0      // 期望某符号但发现另一符号
#define EXPECT 1                     // 缺少期望的符号
#define EXPECT_NUMEBR_AFTER_BECOMES 2 // 赋值号后应为数字
#define ILLEGAL_DEFINE 3             // 非法定义
#define ILLEGAL_WORD 4               // 非法单词
#define ILLEGAL_RVALUE_ASSIGN 5      // 非法右值赋值
#define MISSING 6                    // 缺少符号
#define REDUNDENT 7                  // 多余符号
#define UNDECLARED_IDENT 8           // 未声明的标识符
#define UNDECLARED_PROC 9            // 未声明的过程
#define REDECLEARED_IDENT 10         // 重复声明的标识符
#define REDECLEARED_PROC 11          // 重复声明的过程
#define INCOMPATIBLE_VAR_LIST 12     // 参数列表不匹配
#define UNDEFINED_PROC 13            // 未定义的过程
#define UNDEFINED_PROG 14            // 未定义的程序
#define SYNTAX_ERROR 15              // 语法错误
#define MISSING_DETAILED 16          // 缺少详细信息
#define INVALID_SYNTAX 17            // 无效语法
#define UNEXPECTED_TOKEN 18          // 意外的词法单元

/* ============================================================
 *                 词法单元类型编码 (位掩码)
 * ============================================================
 * 使用位掩码可以方便地进行集合运算，如判断token是否属于某个集合
 * 例如: if (tokenType & (PLUS | MINUS)) 判断是否为加减运算符
 * ============================================================ */

#define NUL 0x0               // 空符号

// 关系运算符 <lop>
#define EQL 0x1               // =
#define NEQ 0x2               // <>
#define LSS 0x4               // <
#define LEQ 0x8               // <=
#define GRT 0x10              // >
#define GEQ 0x20              // >=

// 算术运算符 <aop> <mop>
#define PLUS 0x40             // +
#define MINUS 0x80            // -
#define MULTI 0x100           // *
#define DIVIS 0x200           // /

// 标识符与数字
#define IDENT 0x400           // 标识符 <id>
#define NUMBER 0x800          // 数字常量 <integer>

// 界符
#define LPAREN 0x1000         // 左括号 (
#define RPAREN 0x2000         // 右括号 )
#define COMMA 0x4000          // 逗号 ,
#define SEMICOLON 0x8000      // 分号 ;
#define ASSIGN 0x10000        // 赋值符 :=

// 保留字
#define ODD_SYM 0x20000       // odd 奇偶判断
#define BEGIN_SYM 0x40000     // begin
#define END_SYM 0x80000       // end
#define IF_SYM 0x100000       // if
#define THEN_SYM 0x200000     // then
#define WHILE_SYM 0x400000    // while
#define DO_SYM 0x800000       // do
#define CALL_SYM 0x1000000    // call
#define CONST_SYM 0x2000000   // const
#define VAR_SYM 0x4000000     // var
#define PROC_SYM 0x8000000    // procedure
#define WRITE_SYM 0x10000000  // write
#define READ_SYM 0x20000000   // read
#define PROGM_SYM 0x40000000  // program
#define ELSE_SYM 0x80000000   // else

/* ============================================================
 *                   P-Code虚拟机相关常量
 * ============================================================ */
#define P_CODE_CNT 10         // P-Code指令种类数
#define UNIT_SIZE 4           // 单个存储单元字节数
#define ACT_PRE_REC_SIZE 3    // 活动记录预留空间(RA+DL+Display指针)

// P-Code运算操作码 (用于OPR指令的a字段)
#define OPR_RETURN 0          // 过程返回
#define OPR_NEGTIVE 1         // 取负
#define OPR_ADD 2             // 加法
#define OPR_SUB 3             // 减法
#define OPR_MULTI 4           // 乘法
#define OPR_DIVIS 5           // 除法
#define OPR_ODD 6             // 奇偶判断
#define OPR_EQL 7             // 等于比较
#define OPR_NEQ 8             // 不等比较
#define OPR_LSS 9             // 小于比较
#define OPR_GEQ 10            // 大于等于
#define OPR_GRT 11            // 大于比较
#define OPR_LEQ 12            // 小于等于
#define OPR_PRINT 13          // 输出(不换行)
#define OPR_PRINTLN 14        // 输出(换行)

/* ============================================================
 *                      全局变量声明
 * ============================================================ */
extern size_t glo_offset;     // 全局偏移量，用于计算变量地址

#ifndef UNICODE
#define UNICODE
#endif

/* ============================================================
 *                    工具函数声明
 * ============================================================ */

/**
 * @brief 宽字符串转整数
 * @param numStr 待转换的数字字符串
 * @return 转换后的整数，失败返回0
 */
int w_str2int(wstring numStr);

/**
 * @brief 整数转宽字符串
 * @param num 待转换的整数
 * @return 转换后的宽字符串
 */
wstring int2w_str(int num);

/* ============================================================
 *              Unicode文件读取器 (带缓冲区)
 * ============================================================ */

// 缓冲区大小常量
const size_t BUFFER_SIZE = 128;  // 字符缓冲区大小

/**
 * @class ReadUnicode
 * @brief 带缓冲区的UTF-8源文件读取器
 * @details 采用缓冲区按需加载机制，不一次性读取整个文件
 *          当请求的字符位置超出当前缓冲区时，自动加载下一块数据
 */
class ReadUnicode
{
private:
    ifstream file;                      // 文件输入流
    bool isFileOpen;                    // 文件是否已打开
    bool reachedEnd;                    // 是否已读到文件末尾
    
    wchar_t buffer[BUFFER_SIZE];        // 字符缓冲区
    size_t bufferStartPos;              // 缓冲区首字符对应的全局位置
    size_t bufferLength;                // 缓冲区当前有效字符数
    size_t totalCharsLoaded;            // 已加载的总字符数
    
    // 内部辅助方法
    int calcUtf8Length(unsigned char byte);     // 计算UTF-8字符长度
    bool readOneChar(wchar_t& outChar);         // 从文件读取一个字符
    bool loadNextBuffer();                      // 加载下一块缓冲区

public:
    ReadUnicode();
    ~ReadUnicode();
    
    void InitReadUnicode();                     // 初始化/重置读取器
    void readFile2USC2(string filename);        // 打开文件准备读取
    wchar_t getProgmWStr(const size_t pos);     // 获取指定位置的字符
    bool isEmpty();                             // 判断是否为空
    size_t getLoadedCount();                    // 获取已加载字符数
};

extern ReadUnicode readUnicode;

#endif
