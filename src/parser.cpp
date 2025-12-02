#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdio.h>
#include <cstring>

#include "parser.hpp"
#include "lexer.hpp"

using namespace std;

// 全局Parser实例
Parser* parser = nullptr;

// 错误类型定义
#define MISSING 1           // 缺少符号
#define ILLEGAL_DEFINE 2    // 非法定义
#define EXPECT_STH_FIND_ANTH 3  // 期望某符号但找到另一个

// Parser类实现
Parser::Parser(fstream &file) : lexer(file)
{
    // 初始化时读取第一个token
    lexer.NextToken();
}

void Parser::reportError(unsigned int errorType, const char* expected, const char* context)
{
    int line = lexer.GetLine();
    int col = lexer.GetColumn();
    printf("Syntax Error [Line %d, Column %d]: ", line, col);
    
    switch(errorType) {
        case MISSING:
            printf("Missing '%s'", expected);
            break;
        case ILLEGAL_DEFINE:
            printf("Illegal definition, expected %s", expected);
            break;
        case EXPECT_STH_FIND_ANTH:
            printf("Expected '%s' but found '%s'", expected, context);
            break;
        default:
            printf("Unknown error type");
            break;
    }
    if(context && strlen(context) > 0 && errorType != EXPECT_STH_FIND_ANTH) {
        printf(" (context: %s)", context);
    }
    printf("\n");
}

// 错误恢复函数：若当前符号在s1中，则返回1；若不在s1中，则报错，接着循环查找下一个在s1 ∪ s2中的符号
int Parser::judge(const unsigned long s1, const unsigned long s2, const unsigned int n, const char* extra)
{
    unsigned long current = lexer.GetToken();
    if (!(current & s1)) // 当前符号不在s1中
    {
        reportError(n, extra, "");
        unsigned long s3 = s1 | s2; // 把s2补充进s1
        
        // 循环找到下一个合法的符号
        while (!(lexer.GetToken() & s3)) 
        {
            if (lexer.GetToken() == NUL)
            {
                // NUL可能是非法字符，继续读取下一个token
                lexer.NextToken();
                // 如果连续遇到NUL且strtoken为空，可能是文件结束
                if (lexer.GetToken() == NUL && lexer.GetStrToken().empty()) {
                    return 2; // 文件结束
                }
                continue;
            }
            lexer.NextToken(); // 继续词法分析
            // 检查是否文件结束
            if (lexer.GetToken() == NUL && lexer.GetStrToken().empty()) {
                return 2; // 文件结束
            }
        }
        
        if (lexer.GetToken() & s1)
        {
            return 1;
        }
        else if (lexer.GetToken() & s2)
        {
            return -1; // 匹配到s2
        }
        else
            return 0;
    }
    else
    {
        return 1; // 匹配到s1
    }
}

int Parser::judge(const unsigned long s1, const unsigned long s2, const unsigned int n, const char* extra1, const char* extra2)
{
    unsigned long current = lexer.GetToken();
    if (!(current & s1)) // 当前符号不在s1中
    {
        reportError(n, extra1, extra2);
        unsigned long s3 = s1 | s2; // 把s2补充进s1
        
        // 循环找到下一个合法的符号
        while (!(lexer.GetToken() & s3)) 
        {
            if (lexer.GetToken() == NUL)
            {
                // NUL可能是非法字符，继续读取下一个token
                lexer.NextToken();
                // 如果连续遇到NUL且strtoken为空，可能是文件结束
                if (lexer.GetToken() == NUL && lexer.GetStrToken().empty()) {
                    return 2; // 文件结束
                }
                continue;
            }
            lexer.NextToken(); // 继续词法分析
            // 检查是否文件结束
            if (lexer.GetToken() == NUL && lexer.GetStrToken().empty()) {
                return 2; // 文件结束
            }
        }
        
        if (lexer.GetToken() & s1)
            return 1;
        else if (lexer.GetToken() & s2)
        {
            return -1; // 匹配到s2
        }
        else
            return 0;
    }
    else
        return 1;
}

// <prog> → program <id>；<block>
void Parser::prog()
{
    printf("Starting program analysis...\n");
    
    // 匹配 program
    if (lexer.GetToken() == PROGM_SYM)
    {
        lexer.NextToken();
    }
    else
    {
        judge(firstProg, 0, MISSING, "program");
        return;
    }
    
    // 匹配 <id>
    if (lexer.GetToken() == IDENT)
    {
        printf("Program name: %s\n", lexer.GetStrToken().c_str());
        lexer.NextToken();
    }
    else
    {
        judge(firstConst, 0, MISSING, "identifier");
        return;
    }
    
    // 匹配 ;
    if (lexer.GetToken() == SEMICOLON)
    {
        lexer.NextToken();
    }
    else
    {
        judge(SEMICOLON, 0, MISSING, ";");
        return;
    }
    
    // 匹配 <block>
    block();
    
    printf("Program analysis completed\n");
}

// <block> → [<condecl>][<vardecl>][<proc>]<body>
void Parser::block()
{
    printf("Starting block analysis...\n");
    
    // 可选的常量声明
    if (lexer.GetToken() == CONST_SYM)
    {
        condecl();
    }
    
    // 可选的变量声明
    if (lexer.GetToken() == VAR_SYM)
    {
        vardecl();
    }
    
    // 可选的过程声明
    while (lexer.GetToken() == PROC_SYM)
    {
        proc();
    }
    
    // 必须的复合语句
    body();

    printf("Block analysis completed\n");
}

// <condecl> → const <const>{,<const>};
void Parser::condecl()
{
    printf("Starting constant declaration analysis...\n");
    
    // 匹配 const
    if (lexer.GetToken() == CONST_SYM)
    {
        lexer.NextToken();
    }
    else
    {
        judge(firstCondecl, 0, MISSING, "const");
        return;
    }
    
    // 至少一个常量定义
    constA();
    
    // 可能有多个常量定义
    while (lexer.GetToken() == COMMA)   //,分割多个常量定义
    {
        lexer.NextToken();
        constA();
    }
    
    // 匹配 ;
    if (lexer.GetToken() == SEMICOLON)
    {
        lexer.NextToken();
    }
    else
    {
        judge(SEMICOLON, 0, MISSING, ";");
    }
    
    printf("Constant declaration analysis completed\n");
}

// <const> → <id>:=<integer>
void Parser::constA()
{
    // 匹配 <id>
    if (lexer.GetToken() == IDENT)
    {
        string idName = lexer.GetStrToken();
        printf("Constant name: %s\n", idName.c_str());
        lexer.NextToken();
    }
    else
    {
        judge(firstConst, 0, MISSING, "identifier");
        return;
    }
    
    // 匹配 :=
    if (lexer.GetToken() == ASSIGN)
    {
        lexer.NextToken();
    }
    else if (lexer.GetToken() == EQL)
    {
        judge(ASSIGN, 0, EXPECT_STH_FIND_ANTH, ":=", "=");      //:=是合法的，=是非法的
        lexer.NextToken();
    }
    else
    {
        judge(ASSIGN, 0, MISSING, ":=");                        // 缺少:=，报错
        return;
    }
    
    // 匹配 <integer>
    if (lexer.GetToken() == NUMBER)
    {
        printf("Constant value: %s\n", lexer.GetStrToken().c_str());
        lexer.NextToken();
    }
    else
    {
        judge(NUMBER, 0, MISSING, "integer");
    }
}

// <vardecl> → var <id>{,<id>};
void Parser::vardecl()
{
    printf("Starting variable declaration analysis...\n");
    
    // 匹配 var
    if (lexer.GetToken() == VAR_SYM)
    {
        lexer.NextToken();
    }
    else
    {
        judge(firstVardecl, 0, MISSING, "var");
        return;
    }
    
    // 至少一个标识符
    if (lexer.GetToken() == IDENT)
    {
        printf("Variable name: %s\n", lexer.GetStrToken().c_str());
        lexer.NextToken();
    }
    else
    {
        judge(IDENT, 0, MISSING, "identifier");
        return;
    }
    
    // 可能有多个标识符
    while (lexer.GetToken() == COMMA)       //,分割多个变量声明
    {
        lexer.NextToken();
        if (lexer.GetToken() == IDENT)
        {
            printf("Variable name: %s\n", lexer.GetStrToken().c_str());
            lexer.NextToken();
        }
        else
        {
            judge(IDENT, 0, MISSING, "identifier");
            return;
        }
    }
    
    // 匹配 ;
    if (lexer.GetToken() == SEMICOLON)
    {
        lexer.NextToken();
    }
    else
    {
        judge(SEMICOLON, 0, MISSING, ";");
    }
    
    printf("Variable declaration analysis completed\n");
}

// <proc> → procedure <id>（[<id>{,<id>}]）;<block>{;<proc>}
void Parser::proc()
{
    printf("Starting procedure declaration analysis...\n");
    
    // 匹配 procedure
    if (lexer.GetToken() == PROC_SYM)
    {
        lexer.NextToken();
    }
    else
    {
        judge(firstProc, 0, MISSING, "procedure");
        return;
    }
    
    // 匹配 <id>
    if (lexer.GetToken() == IDENT)
    {
        printf("Procedure name: %s\n", lexer.GetStrToken().c_str());
        lexer.NextToken();
    }
    else
    {
        judge(IDENT, 0, MISSING, "identifier");
        return;
    }
    
    // 匹配 (
    if (lexer.GetToken() == LPAREN)
    {
        lexer.NextToken();
        
        // 可选的参数列表
        if (lexer.GetToken() == IDENT)
        {
            printf("Parameter: %s\n", lexer.GetStrToken().c_str());
            lexer.NextToken();
            
            // 可能有多个参数
            while (lexer.GetToken() == COMMA)       //,分割多个参数
            {
                lexer.NextToken();
                if (lexer.GetToken() == IDENT)
                {
                    printf("Parameter: %s\n", lexer.GetStrToken().c_str());
                    lexer.NextToken();
                }
                else
                {
                    judge(IDENT, 0, MISSING, "identifier");
                    return;
                }
            }
        }
        
        // 匹配 )
        if (lexer.GetToken() == RPAREN)
        {
            lexer.NextToken();
        }
        else
        {
            judge(RPAREN, 0, MISSING, ")");
            return;
        }
    }
    else
    {
        judge(LPAREN, 0, MISSING, "(");
        return;
    }
    
    // 匹配 ;
    if (lexer.GetToken() == SEMICOLON)
    {
        lexer.NextToken();
    }
    else
    {
        judge(SEMICOLON, 0, MISSING, ";");
        return;
    }
    
    // 匹配 <block>
    block();

    // 根据BNF: <proc> → procedure <id>（[<id>{,<id>}]）;<block>{;<proc>}
    // 可能有多个过程声明，用分号分隔
    
    while (lexer.GetToken() == SEMICOLON)
    {
        lexer.NextToken();

        // 检查下一个是否是 procedure
        if (lexer.GetToken() == PROC_SYM)
        {
            // 递归调用 proc() 处理下一个过程
            proc();
        }
        else
        {
            judge(PROC_SYM, 0, MISSING, "procedure");
            return;
        }
    }

    printf("Procedure declaration analysis completed\n");
}

// <body> → begin <statement>{;<statement>}end
void Parser::body()
{
    printf("Starting compound statement analysis...\n");
    
    // 匹配 begin
    if (lexer.GetToken() == BEGIN_SYM)
    {
        lexer.NextToken();
    }
    else
    {
        judge(firstBody, 0, MISSING, "begin");
        return;
    }
    
    // 至少一个语句
    statement();
    
    // 可能有多个语句
    while (lexer.GetToken() == SEMICOLON)
    {
        lexer.NextToken();
        // 检查是否是 statement
        if (lexer.GetToken() & firstStatement)
        {
            //递归调用 statement()处理下一个语句
            statement();
        }
        else
        {
            judge(firstStatement, 0, MISSING, "statement");
            return;
        }
        
    }
    
    // 匹配 end
    if (lexer.GetToken() == END_SYM)
    {
        lexer.NextToken();
    }
    else
    {
        judge(END_SYM, 0, MISSING, "end");
    }
    
    printf("Compound statement analysis completed\n");
}

// <statement> → <id> := <exp>
//               |if <lexp> then <statement>[else <statement>]
//               |while <lexp> do <statement>
//               |call <id>（[<exp>{,<exp>}]）
//               |<body>
//               |read (<id>{，<id>})
//               |write (<exp>{,<exp>})
void Parser::statement()
{
    unsigned long token = lexer.GetToken();
    
    if (token == IDENT)
    {
        // <id> := <exp>
        printf("Assignment statement: %s\n", lexer.GetStrToken().c_str());
        lexer.NextToken();
        
        if (lexer.GetToken() == ASSIGN)
        {
            lexer.NextToken();
            exp();
        }
        else if (lexer.GetToken() == EQL)
        {
            judge(ASSIGN, 0, EXPECT_STH_FIND_ANTH, ":=", "=");      //:=是合法的，=是非法的
            lexer.NextToken();
            exp();
        }
        else
        {
            judge(ASSIGN, 0, MISSING, ":=");
        }
    }
    else if (token == IF_SYM)
    {
        // if <lexp> then <statement>[else <statement>]
        printf("If statement\n");
        lexer.NextToken();
        lexp();
        
        if (lexer.GetToken() == THEN_SYM)
        {
            lexer.NextToken();
            statement();
            
            if (lexer.GetToken() == ELSE_SYM)
            {
                lexer.NextToken();
                statement();
            }
        }
        else
        {
            judge(THEN_SYM, 0, MISSING, "then");
            return;   // 缺少then，报错
        }
    }
    else if (token == WHILE_SYM)
    {
        // while <lexp> do <statement>
        printf("While statement\n");
        lexer.NextToken();
        lexp();
        
        if (lexer.GetToken() == DO_SYM)
        {
            lexer.NextToken();
            statement();
        }
        else
        {
            judge(DO_SYM, 0, MISSING, "do");
            return;   // 缺少do，报错
        }
    }
    else if (token == CALL_SYM)
    {
        // call <id>（[<exp>{,<exp>}]）
        printf("Call statement\n");
        lexer.NextToken();
        
        if (lexer.GetToken() == IDENT)
        {
            printf("Calling procedure: %s\n", lexer.GetStrToken().c_str());
            lexer.NextToken();
        }
        else
        {
            judge(IDENT, 0, MISSING, "identifier");
            return;
        }
        
        if (lexer.GetToken() == LPAREN)
        {
            lexer.NextToken();
            
            // 可选的参数列表
            if (lexer.GetToken() & firstExp)
            {
                exp();
                
                while (lexer.GetToken() == COMMA)
                {
                    lexer.NextToken();
                    if(lexer.GetToken() & firstExp)
                    {
                        exp();
                    }
                    else
                    {
                        judge(firstExp, 0, MISSING, "expression");
                    }
                }
            }
            
            if (lexer.GetToken() == RPAREN)
            {
                lexer.NextToken();
            }
            else
            {
                judge(RPAREN, 0, MISSING, ")");
            }
        }
        else
        {
            judge(LPAREN, 0, MISSING, "(");
        }
    }
    else if (token == BEGIN_SYM)
    {
        // <body>
        body();
    }
    else if (token == READ_SYM)
    {
        // read (<id>{，<id>})
        printf("Read statement\n");
        lexer.NextToken();
        
        if (lexer.GetToken() == LPAREN)
        {
            lexer.NextToken();
            
            if (lexer.GetToken() == IDENT)
            {
                printf("Reading variable: %s\n", lexer.GetStrToken().c_str());
                lexer.NextToken();
            }
            else
            {
                judge(IDENT, 0, MISSING, "identifier");
                return;
            }
            
            while (lexer.GetToken() == COMMA)
            {
                lexer.NextToken();
                if (lexer.GetToken() == IDENT)
                {
                    printf("Reading variable: %s\n", lexer.GetStrToken().c_str());
                    lexer.NextToken();
                }
                else
                {
                    judge(IDENT, 0, MISSING, "identifier");
                    return;
                }
            }
            
            if (lexer.GetToken() == RPAREN)
            {
                lexer.NextToken();
            }
            else
            {
                judge(RPAREN, 0, MISSING, ")");
            }
        }
        else
        {
            judge(LPAREN, 0, MISSING, "(");
        }
    }
    else if (token == WRITE_SYM)
    {
        // write (<exp>{,<exp>})
        printf("Write statement\n");
        lexer.NextToken();
        
        if (lexer.GetToken() == LPAREN)
        {
            lexer.NextToken();
            
            if (lexer.GetToken() & firstExp)
            {
                exp();
                
                while (lexer.GetToken() == COMMA)
                {
                    lexer.NextToken();
                    if(lexer.GetToken() & firstExp)
                    {
                        exp();
                    }
                    else
                    {
                        judge(firstExp, 0, MISSING, "expression");
                    }
                }
            }
            else
            {
                judge(firstExp, 0, MISSING, "expression");
            }
            
            if (lexer.GetToken() == RPAREN)
            {
                lexer.NextToken();
            }
            else
            {
                judge(RPAREN, 0, MISSING, ")");
            }
        }
        else
        {
            judge(LPAREN, 0, MISSING, "(");
        }
    }
    else
    {
        judge(firstStatement, 0, ILLEGAL_DEFINE, "statement");
    }
}

// <lexp> → <exp> <lop> <exp>|odd <exp>
void Parser::lexp()
{
    if (lexer.GetToken() == ODD_SYM)
    {
        printf("Odd expression\n");
        lexer.NextToken();
        exp();
    }
    else if (lexer.GetToken() & firstExp)
    {
        exp();
        
        if (lexer.GetToken() & firstLop)
        {
            unsigned long lop = lexer.GetToken();
            printf("Relational operator: ");
            if (lop & EQL) printf("=");
            else if (lop & NEQ) printf("<>");
            else if (lop & LSS) printf("<");
            else if (lop & LEQ) printf("<=");
            else if (lop & GRT) printf(">");
            else if (lop & GEQ) printf(">=");
            printf("\n");
            lexer.NextToken();
            exp();
        }
        else
        {
            judge(firstLop, 0, MISSING, "relational operator");
        }
    }
    else
    {
        judge(firstLexp, 0, MISSING, "condition expression");
    }
}

// <exp> → [+|-]<term>{<aop><term>}
void Parser::exp()
{
    // 可选的 + 或 -
    if (lexer.GetToken() == PLUS)
    {
        lexer.NextToken();
    }
    else if (lexer.GetToken() == MINUS)
    {
        printf("Negative sign\n");
        lexer.NextToken();
    }
    
    // 至少一个term
    term();
    
    // 可能有多个 <aop><term>
    while (lexer.GetToken() == PLUS || lexer.GetToken() == MINUS)
    {
        if (lexer.GetToken() == PLUS)
        {
            printf("Addition operator\n");
        }
        else
        {
            printf("Subtraction operator\n");
        }
        lexer.NextToken();
        term();
    }
}

// <term> → <factor>{<mop><factor>}
void Parser::term()
{
    // 至少一个factor
    factor();
    
    // 可能有多个 <mop><factor>
    while (lexer.GetToken() == MULTI || lexer.GetToken() == DIVIS)
    {
        if (lexer.GetToken() == MULTI)
        {
            printf("Multiplication operator\n");
        }
        else
        {
            printf("Division operator\n");
        }
        lexer.NextToken();
        factor();
    }
}

// <factor>→<id>|<integer>|(<exp>)
void Parser::factor()
{
    unsigned long token = lexer.GetToken();
    
    if (token == IDENT)
    {
        printf("Identifier factor: %s\n", lexer.GetStrToken().c_str());
        lexer.NextToken();
    }
    else if (token == NUMBER)
    {
        printf("Number factor: %s\n", lexer.GetStrToken().c_str());
        lexer.NextToken();
    }
    else if (token == LPAREN)
    {
        lexer.NextToken();
        exp();
        if (lexer.GetToken() == RPAREN)
        {
            lexer.NextToken();
        }
        else
        {
            judge(RPAREN, 0, MISSING, ")");
        }
    }
    else
    {
        judge(firstFactor, 0, MISSING, "factor");
    }
}

// 分析入口
void Parser::analyze()
{
    prog();
}

// 测试主函数
int main(int argc, char* argv[])
{
    string filename = "test/test.txt";
    if (argc > 1) {
        filename = argv[1];
    }
    
    fstream file(filename);
    if (!file.is_open()) {
        cout << "Error: Cannot open file " << filename << endl;
        return 1;
    }
    
    Parser parser(file);
    parser.analyze();
    
    file.close();
    return 0;
}
