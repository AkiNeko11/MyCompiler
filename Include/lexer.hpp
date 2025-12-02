#ifndef LEXER_HPP
#define LEXER_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

// Token类型定义
#define NUL 0x0           // 空
#define EQL 0x1           // =
#define NEQ 0x2           // <>
#define LSS 0x4           // <
#define LEQ 0x8           // <=
#define GRT 0x10          // >
#define GEQ 0x20          // >=
#define PLUS 0x40         // +
#define MINUS 0x80        // -
#define MULTI 0x100       // *
#define DIVIS 0x200       // /
#define IDENT 0x400       // 标识符
#define NUMBER 0x800      // 数值
#define LPAREN 0x1000     // (
#define RPAREN 0x2000     // )
#define COMMA 0x4000      // ,
#define SEMICOLON 0x8000  // ;
#define ASSIGN 0x10000    // :=
#define ODD_SYM 0x20000   // odd
#define BEGIN_SYM 0x40000
#define END_SYM 0x80000
#define IF_SYM 0x100000
#define THEN_SYM 0x200000
#define WHILE_SYM 0x400000
#define DO_SYM 0x800000
#define CALL_SYM 0x1000000
#define CONST_SYM 0x2000000
#define VAR_SYM 0x4000000
#define PROC_SYM 0x8000000
#define WRITE_SYM 0x10000000
#define READ_SYM 0x20000000
#define PROGM_SYM 0x40000000
#define ELSE_SYM 0x80000000

class Lexer
{
public:
    Lexer(fstream &file);
    char GetChar();
    char GetBC();
    string Concat();
    bool IsLetter();
    bool IsDigit();
    int Reserved();
    void Retract();
    int InsertId();
    int InsertConst();
    void ProcError();
    void start();
    
    // 新增：供语法分析器使用的方法
    unsigned long GetToken();  // 获取当前token类型
    string GetStrToken();      // 获取当前token字符串
    int GetLine();             // 获取当前行号
    int GetColumn();           // 获取当前列号
    void NextToken();          // 读取下一个token
    
    vector<string> reserved;
    vector<string> id;
    vector<string> constTable;

private:
    fstream &file;
    int line;   
    int column;
    int code;
    int value;
    string strtoken;
    char ch;
    unsigned long currentToken;  // 当前token类型
    bool tokenReady;            // token是否已准备好
};

#endif

