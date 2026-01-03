/**
 * @file Parser.hpp
 * @brief 语法分析器模块
 * @details 实现递归下降的语法分析，完成语法检查与中间代码生成
 */

#ifndef PARSER_HPP
#define PARSER_HPP

#include <Types.hpp>
#include <ErrorHandle.hpp>
#include <SymTable.hpp>
#include <Lexer.hpp>
#include <PCode.hpp>

/**
 * @class Parser
 * @brief 递归下降语法分析器
 * @details 基于PL/0文法进行语法分析，同时生成P-Code中间代码
 */
class Parser
{
private:
    /* ====== FIRST集定义 ====== */
    unsigned long firstProg = PROGM_SYM;                    // 程序的FIRST集
    unsigned long firstCondecl = CONST_SYM;                 // 常量声明的FIRST集
    unsigned long firstConst = IDENT;                       // 常量定义的FIRST集
    unsigned long firstVardecl = VAR_SYM;                   // 变量声明的FIRST集
    unsigned long firstProc = PROC_SYM;                     // 过程声明的FIRST集
    unsigned long firstBody = BEGIN_SYM;                    // 复合语句的FIRST集
    unsigned long firstStatement = IDENT | IF_SYM | WHILE_SYM | CALL_SYM | 
                                   firstBody | READ_SYM | WRITE_SYM;  // 语句的FIRST集
    unsigned long firstFactor = IDENT | NUMBER | LPAREN;    // 因子的FIRST集
    unsigned long firstTerm = firstFactor;                  // 项的FIRST集
    unsigned long firstExp = firstTerm | PLUS | MINUS;      // 表达式的FIRST集
    unsigned long firstLexp = firstExp | ODD_SYM;           // 条件表达式的FIRST集
    unsigned long firstLop = EQL | NEQ | LSS | LEQ | GRT | GEQ;  // 关系运算符的FIRST集
    unsigned long firstBlock = firstCondecl | firstVardecl | firstProc | firstBody;  // 分程序的FIRST集

    /* ====== FOLLOW集定义 ====== */
    unsigned long followProg = 0;                           // 程序的FOLLOW集
    unsigned long followBlock = SEMICOLON | NUL;            // 分程序的FOLLOW集
    unsigned long followCondecl = firstVardecl | firstProc | firstBody;  // 常量声明的FOLLOW集
    unsigned long followConst = COMMA | SEMICOLON;          // 常量定义的FOLLOW集
    unsigned long followVardecl = firstProc | firstBody;    // 变量声明的FOLLOW集
    unsigned long followProc = firstBody | SEMICOLON;       // 过程声明的FOLLOW集
    unsigned long followStatement = SEMICOLON | END_SYM | ELSE_SYM;  // 语句的FOLLOW集
    unsigned long followBody = SEMICOLON | followStatement; // 复合语句的FOLLOW集
    unsigned long followLexp = THEN_SYM | DO_SYM;           // 条件表达式的FOLLOW集
    unsigned long followExp = firstLop | COMMA | RPAREN | followStatement | followLexp;  // 表达式的FOLLOW集
    unsigned long followTerm = followExp | PLUS | MINUS;    // 项的FOLLOW集
    unsigned long followFactor = followTerm | MULTI | DIVIS;  // 因子的FOLLOW集
    unsigned long followLop = followExp | followFactor;     // 关系运算符的FOLLOW集
    unsigned long followId = COMMA | SEMICOLON | LPAREN | RPAREN | followFactor;  // 标识符的FOLLOW集

public:
    void block();       // 分程序处理
    void proc();        // 过程声明处理
    void statement();   // 语句处理
    void constA();      // 常量定义处理
    void condecl();     // 常量声明处理
    void vardecl();     // 变量声明处理
    void term();        // 项处理
    void factor();      // 因子处理
    void prog();        // 程序处理
    void body();        // 复合语句处理
    void lexp();        // 条件表达式处理
    void exp();         // 表达式处理
    void analyze();     // 启动语法分析

    // 错误报告
    void reportError(unsigned int errorType, const wchar_t* expected, const wchar_t* context);
    // 词法单元匹配与错误恢复
    int judge(const unsigned long s1, const unsigned long s2, const unsigned int n, const wchar_t* extra);
    int judge(const unsigned long s1, const unsigned long s2, const unsigned int n, 
              const wchar_t* extra1, const wchar_t* extra2);
};

extern Parser parser;

#endif
