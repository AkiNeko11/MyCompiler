#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdio.h>
#include "lexer.hpp"

using namespace std;

Lexer::Lexer(fstream &file):file(file),line(1),column(0),code(0),value(0),strtoken(""),ch('\n'),currentToken(NUL),tokenReady(false)
{
    this->reserved.push_back("odd");
    this->reserved.push_back("begin");
    this->reserved.push_back("end");
    this->reserved.push_back("if");
    this->reserved.push_back("then");
    this->reserved.push_back("while");
    this->reserved.push_back("do");
    this->reserved.push_back("call");
    this->reserved.push_back("const");
    this->reserved.push_back("var");
    this->reserved.push_back("procedure");
    this->reserved.push_back("write");
    this->reserved.push_back("read");
    this->reserved.push_back("program");
    this->reserved.push_back("else");
}



char Lexer::GetChar() 
{
    // 子程序过程，将下一输入字符读到ch中，搜索指示器前移一个字符位置
    this->ch=this->file.get();
    
    // 更新行号和列号
    if(this->ch == '\n') 
    {
        this->line++;
        this->column = 0;  // 换行后列号重置为0
    } 
    else if(ch != EOF && ch != -1) 
    {
        this->column++;  // 非换行符，列号+1
    }
    
    return this->ch;
}

char Lexer::GetBC() 
{
    // 子程序过程，检查字符ch是否为空白符号，如果是，则调用GetChar函数读取下一个字符，否则返回ch
    if(this->ch == EOF || this->ch == -1) return this->ch;
    
    char ch2 = this->ch;
    // 使用 isspace() 函数判断所有空白字符（包括空格、制表符、换行符等）
    while(isspace(static_cast<unsigned char>(ch2)) && !this-> file.eof()) 
    {
        ch2 = this->GetChar();  // 调用GetChar，自动更新行号和列号
        if(this->file.eof() || ch2 == EOF) 
        {
            return EOF;
        }
    }
    return ch2;
}

string Lexer::Concat() 
{
    // 子程序过程，将字符ch添加到字符串strToken中
    this->strtoken += this->ch;
    return this->strtoken;
}

bool Lexer::IsLetter() 
{
    // 布尔函数过程，检查字符ch是否为字母
    if (this->ch >= 'a' && this->ch <= 'z') 
    {
        return true;
    }
    return false;
}

bool Lexer::IsDigit() 
{
    // 布尔函数过程，检查字符ch是否为数字
    if (this->ch >= '0' && this->ch <= '9') 
    {
        return true;
    }
    return false;
}

int Lexer::Reserved() 
{
    // 整型函数过程，对strToken中的字符串查找保留字表，若它是一个保留字则返回它的编码，否则返回-1值
    for (int i = 0; i < this->reserved.size(); i++) 
    {
        if (this->strtoken == this->reserved[i]) 
        {
            return i;
        }
    }
    return -1;
}

void Lexer::Retract() 
{
    // 子程序过程，将搜索指示器回调一个字符位置，并将ch置为空字符
    if(this->ch != EOF && this->ch != -1 && this->ch != ' ' && this->ch != '\n') 
    {
        this->file.unget();
        // 回退列号
        if(this->column > 0) 
        {
            this->column--;
        }
    }
    this->ch = '\n';
}

int Lexer::InsertId() 
{
    // 整型函数过程，将strToken中的标识符插入到符号表中，返回符号表的指针
    this->id.push_back(this->strtoken);
    return id.size();   // 符号表的指针
}

int Lexer::InsertConst() 
{
    // 整型函数过程，将strToken中的常量插入到常量表中，返回常量表的指针
    this->constTable.push_back(this->strtoken);
    return this->constTable.size();   // 常量表的指针
}

void Lexer::ProcError() 
{
    // 子程序过程，输出错误信息
    printf("error: at line %d, column %d\n", this->line, this->column);
}

// 供语法分析器使用的方法
unsigned long Lexer::GetToken()
{
    return this->currentToken;
}

string Lexer::GetStrToken()
{
    return this->strtoken;
}

int Lexer::GetLine()
{
    return this->line;
}

int Lexer::GetColumn()
{
    return this->column;
}

void Lexer::NextToken()
{
    // 读取下一个token，使用GetBC跳过空白字符
    this->strtoken = "";  // 每次开始时清空strtoken
    this->ch = this->GetChar();
    if(this->file.eof() || this->ch == EOF) 
    {
        this->currentToken = NUL;
        this->tokenReady = true;
        return;
    }
    this->ch = this->GetBC();
    if(this->file.eof() || this->ch == EOF) 
    {
        this->currentToken = NUL;
        this->tokenReady = true;
        return;
    }

    if(this->IsLetter()) {
        while(this->IsLetter() || this->IsDigit()) 
        {
            this->strtoken = this->Concat();
            this->ch = this->GetChar();
            if(this->file.eof() || this->ch == EOF) break;
            if(this->ch == ' ' || this->ch == '\n' || this->ch == '\t') break;
        }
        if(!this->file.eof() && this->ch != EOF && this->ch != -1) 
        {
            this->Retract();
        }
        this->code = this->Reserved();
        if(this->code == -1)  // 非保留字
        {
            this->value = this->InsertId();
            this->currentToken = IDENT;
        }
        else
        {
            // 根据保留字编码设置token类型
            switch(this->code) {
                case 0: this->currentToken = ODD_SYM; break;
                case 1: this->currentToken = BEGIN_SYM; break;
                case 2: this->currentToken = END_SYM; break;
                case 3: this->currentToken = IF_SYM; break;
                case 4: this->currentToken = THEN_SYM; break;
                case 5: this->currentToken = WHILE_SYM; break;
                case 6: this->currentToken = DO_SYM; break;
                case 7: this->currentToken = CALL_SYM; break;
                case 8: this->currentToken = CONST_SYM; break;
                case 9: this->currentToken = VAR_SYM; break;
                case 10: this->currentToken = PROC_SYM; break;
                case 11: this->currentToken = WRITE_SYM; break;
                case 12: this->currentToken = READ_SYM; break;
                case 13: this->currentToken = PROGM_SYM; break;
                case 14: this->currentToken = ELSE_SYM; break;
                default: this->currentToken = NUL; break;
            }
        }
    }
    else if(this->IsDigit()) 
    {
        while(this->IsDigit()) 
        {
            this->strtoken = this->Concat();
            this->ch = this->GetChar();
            if(this->file.eof() || this->ch == EOF) break;
            if(this->ch == ' ' || this->ch == '\n' || this->ch == '\t') break;
        }
        
        if(this->IsLetter())
        {
            // 非法：数字后面直接跟字母，这里我们采取的是吃掉后续字母数字，直到遇到分隔符
            ProcError();
            while(this->IsLetter() || this->IsDigit())
            {
                this->ch = this->GetChar();
                if(this->file.eof() || this->ch == EOF) break;
            }
            if(!this->file.eof() && this->ch != EOF)
                this->Retract();
            this->currentToken = NUL;   // 将当前token类型设置为NUL，表示非法，后续会设置非法类型
            this->tokenReady = true;
            return;
        }

        if(!this->file.eof() && this->ch != EOF && this->ch != -1) 
        {
            this->Retract();
        }
        this->value = this->InsertConst();
        this->currentToken = NUMBER;
    }
    else if(this->ch == '=')
    {
        this->strtoken = "=";
        this->currentToken = EQL;
    }
    else if(this->ch == '+')
    {
        this->strtoken = "+";
        this->currentToken = PLUS;
    }
    else if(this->ch == '-')
    {
        this->strtoken = "-";
        this->currentToken = MINUS;
    }
    else if(this->ch == '*')
    {
        this->strtoken = "*";
        this->ch = this->GetChar();
        if (this->ch == '*')
        {
            this->strtoken = "**";
            // PL/0不支持**，这里作为错误处理，说实话有点左右脑互搏了，明明词法分析书上参考的部分给的是power**，但是这里又不支持了，给我看笑了
            ProcError();
            this->currentToken = NUL;
        }
        else
        {
            if(!this->file.eof()) {
                this->Retract();
            }
            this->currentToken = MULTI;
        }
    }
    else if(this->ch == '/')
    {
        this->strtoken = "/";
        this->currentToken = DIVIS;
    }
    else if(this->ch == ';')
    {
        this->strtoken = ";";
        this->currentToken = SEMICOLON;
    }
    else if(this->ch == ':')
    {
        this->ch = this->GetChar();
        if(this->ch == '=')
        {
            this->strtoken = ":=";
            this->currentToken = ASSIGN;
        }
        else
        {
            if(!this->file.eof()) {
                this->Retract();
            }
            this->strtoken = ":";
            // 单独的:不是合法token，报错
            ProcError();
            this->currentToken = NUL;
        }
    }
    else if(this->ch == ',')
    {
        this->strtoken = ",";
        this->currentToken = COMMA;
    }
    else if(this->ch == '(')
    {
        this->strtoken = "(";
        this->currentToken = LPAREN;
    }
    else if(this->ch == ')')
    {
        this->strtoken = ")";
        this->currentToken = RPAREN;
    }
    else if(this->ch == '<')
    {
        this->strtoken = "<";
        this->ch = this->GetChar();
        if(this->ch == '=')
        {
            this->strtoken = "<=";
            this->currentToken = LEQ;
        }
        else if(this->ch == '>')
        {
            this->strtoken = "<>";
            this->currentToken = NEQ;
        }
        else
        {
            if(!this->file.eof()) {
                this->Retract();
            }
            this->currentToken = LSS;
        }
    }
    else if(this->ch == '>')
    {
        this->strtoken = ">";
        this->ch = this->GetChar();
        if(this->ch == '=')
        {
            this->strtoken = ">=";
            this->currentToken = GEQ;
        }
        else
        {
            if(!this->file.eof()) {
                this->Retract();
            }
            this->currentToken = GRT;
        }
    }
    else if(this->ch != EOF && this->ch != -1) 
    {
        // 遇到非法字符，报告错误并跳过
        this->strtoken = string(1, this->ch);
        ProcError();
        // 跳过这个字符，继续读取下一个token
        this->currentToken = NUL;
        this->tokenReady = true;
        // 递归调用NextToken继续读取（但使用循环避免栈溢出）
        // 这里直接返回NUL，让调用者处理
        return;
    }
    
    this->tokenReady = true;
}

void Lexer::start()
{
    while(!this->file.eof())
    {
        this->strtoken = "";  // 每次循环开始时清空strtoken
        this->ch=this->GetChar();
        if(this->file.eof() || this->ch == EOF) break;  // 检查文件是否结束
        this->ch=this->GetBC();
        if(this->file.eof() || this->ch == EOF) break;  // 检查文件是否结束

        if(this->IsLetter()) {
            while(this->IsLetter() || this->IsDigit()) 
            {
                this->strtoken=this->Concat();
                this->ch=this->GetChar();
                if(this->file.eof() || this->ch == EOF) break;
                // 不跳过空白字符，空白字符应该作为标识符/保留字的结束标志
                if(this->ch == ' ' || this->ch == '\n') break;
            }
            // 退出循环时，ch是第一个非字母数字字符（可能是空白、标点或EOF），需要回退以便下次处理
            if(!this->file.eof() && this->ch != EOF && this->ch != -1) 
            {
                this->Retract();
            }
            this->code=this->Reserved();
            if(this->code==-1)  // 非保留字返回-1
            {
                this->value=this->InsertId();
                printf("id: %s, value: %d\n", this->strtoken.c_str(), this->value);     // 输出标识符及其在符号表中的指针位置
            }
            else
            {
                printf("reserved: %s\n", this->strtoken.c_str());       // 输出保留字及其编码
            }
        }
        else if(this->IsDigit()) 
        {
            while(this->IsDigit()) 
            {
                this->strtoken=this->Concat();
                this->ch=this->GetChar();
                if(this->file.eof() || this->ch == EOF) break;
                // 不跳过空白字符，空白字符应该作为常量的结束标志
                if(this->ch == ' ' || this->ch == '\n') break;
            }
            
            // 第二步检查数字后面紧跟的是不是字母
            if(this->IsLetter())
            {
                // 非法：数字后面直接跟字母，如 123abc
                // 这里我们吃掉后续字母数字，直到遇到分隔符
                ProcError();
                while(this->IsLetter() || this->IsDigit())
                {
                    this->ch = this->GetChar();
                    if(this->file.eof() || this->ch == EOF) break;
                }
                
                // 回退一个字符，准备下次处理
                if(!this->file.eof() && this->ch != EOF)
                    this->Retract();

                continue;     // 跳过本次循环，继续下一个token
            }

            // 退出循环时，ch是第一个非数字字符（可能是空白、标点或EOF），需要回退以便下次处理
            if(!this->file.eof() && this->ch != EOF && this->ch != -1) 
            {
                this->Retract();
            }
            this->value=this->InsertConst();
            printf("const: %s, value: %d\n", this->strtoken.c_str(), this->value);     // 输出常量及其在常量表中的指针位置
        }
        else if(this->ch=='=')
        {
            printf("reserved: assign =\n");
        }
        else if(this->ch=='+')
        {
            printf("reserved: plus +\n");
        }
        else if(this->ch=='*')
        {
            this->ch = this->GetChar();
            if (this->ch == '*')
            {
                printf("reserved: power **\n");
            }
            else
            {
                if(!this->file.eof()) {
                    this->Retract();
                }
                printf("reserved: star *\n");
            }
        }
        else if(this->ch==';')
        {
            printf("reserved: semicolon ;\n");
        }
        else if(this->ch==':')
        {
            printf("reserved: colon :\n");
        }
        else if(this->ch==',')
        {
            printf("reserved: comma ,\n");
        }
        else if(this->ch=='(')
        {
            printf("reserved: lparen (\n");
        }
        else if(this->ch==')')
        {
            printf("reserved: rparen )\n");
        }
        else if(this->ch=='{')
        {
            printf("reserved: lbrace {\n");
        }
        else if(this->ch=='}')
        {
            printf("reserved: rbrace }\n");
        }
        else if(this->ch != EOF && this->ch != -1 && this->ch != ' ' && this->ch != '\n') {
            this->ProcError();
        }
    }
}

