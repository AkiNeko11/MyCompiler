#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdio.h>


using namespace std;

// 全局变量：行号和列号
int line = 1;    // 从第1行开始
int column = 0;  // 从第0列开始（读取字符后+1）

// 函数声明
char GetChar(fstream &file);
char GetBC(fstream &file,char ch);
string Concat(fstream &file,char ch,string strToken);
bool IsLetter(char ch);
bool IsDigit(char ch);
int Reserved(string strToken,vector<string> reserved);
void Retract(fstream &file,char &ch);
int InsertId(string strToken,vector<string> &id);
int InsertConst(string strToken,vector<string> &constTable);
void ProcError();


int main() {
    fstream file("test.txt");
    if (!file.is_open()) {
        cout << "Error: Failed to open file" << endl;
        return 1;
    }

    vector<string> reserved;
    
    reserved.push_back("program");
    reserved.push_back("const");
    reserved.push_back("var");
    reserved.push_back("procedure");
    reserved.push_back("begin");
    reserved.push_back("end");
    reserved.push_back("read");
    reserved.push_back("write");
    reserved.push_back("call");


    vector<string> id;
    vector<string> constTable;

    int code;
    int value;

    string strtoken;
    char ch;

    // 初始化行号和列号
    line = 0;
    column = 0;

    while(!file.eof())
    {
        strtoken = "";  // 每次循环开始时清空strtoken
        ch=GetChar(file);
        if(file.eof() || ch == EOF) break;  // 检查文件是否结束
        ch=GetBC(file,ch);
        if(file.eof() || ch == EOF) break;  // 检查文件是否结束

        if(IsLetter(ch)) {
            while(IsLetter(ch) || IsDigit(ch)) 
            {
                strtoken=Concat(file,ch,strtoken);
                ch=GetChar(file);
                if(file.eof() || ch == EOF) break;
                // 不跳过空白字符，空白字符应该作为标识符/保留字的结束标志
                if(ch == ' ' || ch == '\n') break;
            }
            // 退出循环时，ch是第一个非字母数字字符（可能是空白、标点或EOF），需要回退以便下次处理
            if(!file.eof() && ch != EOF && ch != -1) 
            {
                Retract(file,ch);
            }
            code=Reserved(strtoken,reserved);
            if(code==-1)  // 非保留字返回-1
            {
                value=InsertId(strtoken,id);
                printf("id: %s, value: %d\n", strtoken.c_str(), value);     // 输出标识符及其在符号表中的指针位置
            }
            else
            {
                printf("reserved: %s\n", strtoken.c_str());       // 输出保留字及其编码
            }
        }
        else if(IsDigit(ch)) 
        {
            while(IsDigit(ch)) 
            {
                strtoken=Concat(file,ch,strtoken);
                ch=GetChar(file);
                if(file.eof() || ch == EOF) break;
                // 不跳过空白字符，空白字符应该作为常量的结束标志
                if(ch == ' ' || ch == '\n') break;
            }
            
            // 第二步：关键！检查数字后面紧跟的是不是字母
            if(IsLetter(ch))
            {
                // 非法：数字后面直接跟字母，如 123abc
                // 可以选择：吃掉整个非法标识符（跳过到分隔符），或者直接退出
                // 这里我们吃掉后续字母数字，直到遇到分隔符
                ProcError();
                while(IsLetter(ch) || IsDigit(ch))
                {
                    ch = GetChar(file);
                    if(file.eof() || ch == EOF) break;
                }
                
                // 回退一个字符，准备下次处理
                if(!file.eof() && ch != EOF)
                    Retract(file, ch);

                continue;     // 跳过本次循环，继续下一个token
            }

            // 退出循环时，ch是第一个非数字字符（可能是空白、标点或EOF），需要回退以便下次处理
            if(!file.eof() && ch != EOF && ch != -1) 
            {
                Retract(file,ch);
            }
            value=InsertConst(strtoken,constTable);
            printf("const: %s, value: %d\n", strtoken.c_str(), value);     // 输出常量及其在常量表中的指针位置
        }
        else if(ch=='=')
        {
            printf("reserved: assign =\n");
        }
        else if(ch=='+')
        {
            printf("reserved: plus +\n");
        }
        else if(ch=='*')
        {
            ch = GetChar(file);
            if (ch == '*')
            {
                printf("reserved: power **\n");
            }
            else
            {
                if(!file.eof()) {
                    Retract(file,ch);
                }
                printf("reserved: star *\n");
            }
        }
        else if(ch==';')
        {
            printf("reserved: semicolon ;\n");
        }
        else if(ch==':')
        {
            printf("reserved: colon :\n");
        }
        else if(ch==',')
        {
            printf("reserved: comma ,\n");
        }
        else if(ch=='(')
        {
            printf("reserved: lparen (\n");
        }
        else if(ch==')')
        {
            printf("reserved: rparen )\n");
        }
        else if(ch=='{')
        {
            printf("reserved: lbrace {\n");
        }
        else if(ch=='}')
        {
            printf("reserved: rbrace }\n");
        }
        else if(ch != EOF && ch != -1 && ch != ' ' && ch != '\n') {
            ProcError();
        }
    }

    return 0;

}

char GetChar(fstream &file) 
{
    // 子程序过程，将下一输入字符读到ch中，搜索指示器前移一个字符位置
    char ch;
    ch=file.get();
    
    // 更新行号和列号
    if(ch == '\n') 
    {
        line++;
        column = 0;  // 换行后列号重置为0
    } 
    else if(ch != EOF && ch != -1) 
    {
        column++;  // 非换行符，列号+1
    }
    
    return ch;
}

char GetBC(fstream &file, char ch) 
{
    // 子程序过程，检查字符ch是否为空白符号，如果是，则调用GetChar函数读取下一个字符，否则返回ch
    if(ch == EOF || ch == -1) return ch;
    
    char ch2 = ch;
    // 使用 isspace() 函数判断所有空白字符（包括空格、制表符、换行符等）
    while(isspace(static_cast<unsigned char>(ch2)) && !file.eof()) 
    {
        ch2 = GetChar(file);  // 调用GetChar，自动更新行号和列号
        if(file.eof() || ch2 == EOF) 
        {
            return EOF;
        }
    }
    return ch2;
}

string Concat(fstream &file,char ch,string strToken) 
{
    // 子程序过程，将字符ch添加到字符串strToken中
    strToken += ch;
    return strToken;
}

bool IsLetter(char ch) 
{
    // 布尔函数过程，检查字符ch是否为字母
    if (ch >= 'a' && ch <= 'z') 
    {
        return true;
    }
    return false;
}

bool IsDigit(char ch) 
{
    // 布尔函数过程，检查字符ch是否为数字
    if (ch >= '0' && ch <= '9') 
    {
        return true;
    }
    return false;
}

int Reserved(string strToken,vector<string> reserved) 
{
    // 整型函数过程，对strToken中的字符串查找保留字表，若它是一个保留字则返回它的编码，否则返回-1值
    for (int i = 0; i < reserved.size(); i++) 
    {
        if (strToken == reserved[i]) 
        {
            return i;
        }
    }
    return -1;
}

void Retract(fstream &file,char &ch) 
{
    // 子程序过程，将搜索指示器回调一个字符位置，并将ch置为空字符
    if(ch != EOF && ch != -1 && ch != ' ' && ch != '\n') 
    {
        file.unget();
        // 回退列号
        if(column > 0) 
        {
            column--;
        }
    }
    ch = '\n';
}

int InsertId(string strToken,vector<string> &id) 
{
    // 整型函数过程，将strToken中的标识符插入到符号表中，返回符号表的指针
    id.push_back(strToken);
    return id.size();   // 符号表的指针
}

int InsertConst(string strToken,vector<string> &constTable) 
{
    // 整型函数过程，将strToken中的常量插入到常量表中，返回常量表的指针
    constTable.push_back(strToken);
    return constTable.size();   // 常量表的指针
}

void ProcError() 
{
    // 子程序过程，输出错误信息
    printf("error: at line %d, column %d\n", line, column);
}
