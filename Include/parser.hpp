#ifndef PARSER_HPP
#define PARSER_HPP

#include <fstream>
#include "lexer.hpp"

// 对block、proc、statement、condition、expression、term、factor进行分析
class Parser
{
private:
    Lexer lexer;  // 词法分析器
    
    // first集
    unsigned long firstProg = PROGM_SYM;
    unsigned long firstCondecl = CONST_SYM;
    unsigned long firstConst = IDENT;
    unsigned long firstVardecl = VAR_SYM;
    unsigned long firstProc = PROC_SYM;
    unsigned long firstBody = BEGIN_SYM;
    unsigned long firstStatement = IDENT | IF_SYM | WHILE_SYM | CALL_SYM | firstBody | READ_SYM | WRITE_SYM;
    unsigned long firstFactor = IDENT | NUMBER | LPAREN;
    unsigned long firstTerm = firstFactor;
    unsigned long firstExp = firstTerm | PLUS | MINUS;
    unsigned long firstLexp = firstExp | ODD_SYM;
    unsigned long firstLop = EQL | NEQ | LSS | LEQ | GRT | GEQ;
    unsigned long firstBlock = firstCondecl | firstVardecl | firstProc | firstBody;

    // follow集
    unsigned long followProg = 0;
    unsigned long followBlock = SEMICOLON | NUL;
    unsigned long followCondecl = firstVardecl | firstProc | firstBody;
    unsigned long followConst = COMMA | SEMICOLON;
    unsigned long followVardecl = firstProc | firstBody;
    unsigned long followProc = firstBody | SEMICOLON; 
    unsigned long followStatement = SEMICOLON | END_SYM | ELSE_SYM;
    unsigned long followBody = SEMICOLON | followStatement;
    unsigned long followLexp = THEN_SYM | DO_SYM;
    unsigned long followExp = firstLop | COMMA | RPAREN | followStatement | followLexp; 
    unsigned long followTerm = followExp | PLUS | MINUS;
    unsigned long followFactor = followTerm | MULTI | DIVIS;
    unsigned long followLop = followExp | followFactor;
    unsigned long followId = COMMA | SEMICOLON | LPAREN | RPAREN | followFactor;
public:
    Parser(std::fstream &file);
    void block();
    void proc();
    void statement();
    void constA();
    void condecl();
    void vardecl();
    void term();
    void factor();
    void prog();
    void body();
    void lexp();
    void exp();
    void analyze();
    void reportError(unsigned int errorType, const char* expected, const char* context);
    int judge(const unsigned long s1, const unsigned long s2, const unsigned int n, const char* extra);
    int judge(const unsigned long s1, const unsigned long s2, const unsigned int n, const char* extra1, const char* extra2);
};

#endif