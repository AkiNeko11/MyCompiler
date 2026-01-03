/**
 * @file Lexer.cpp
 * @brief 词法分析器实现
 * @details 实现对PL/0源程序的词法分析，识别保留字、标识符、
 *          数字、运算符和界符等词法单元
 */

#include <lexer.hpp>

/**
 * @brief 初始化词法分析器
 * @details 重置所有状态变量，初始化符号映射表
 */
void Lexer::InitLexer()
{
    // 状态变量初始化
    ch = L' ';
    tokenType = NUL;
    strToken = L"";
    colPos = 0;
    rowPos = 1;
    preWordCol = 0;
    preWordRow = 1;
    nowPtr = 0;

    // 符号类型到名称的映射表
    sym_map[NUL] = L"NUL";
    sym_map[IDENT] = L"IDENT";
    sym_map[NUMBER] = L"NUMBER";
    sym_map[PLUS] = L"PLUS";
    sym_map[MINUS] = L"MINUS";
    sym_map[MULTI] = L"MULTI";
    sym_map[DIVIS] = L"DIVIS";
    sym_map[ODD_SYM] = L"ODD_SYM";
    sym_map[EQL] = L"EQL";
    sym_map[NEQ] = L"NEQ";
    sym_map[LSS] = L"LSS";
    sym_map[LEQ] = L"LEQ";
    sym_map[GRT] = L"GRT";
    sym_map[GEQ] = L"GEQ";
    sym_map[LPAREN] = L"LPAREN";
    sym_map[RPAREN] = L"RPAREN";
    sym_map[COMMA] = L"COMMA";
    sym_map[SEMICOLON] = L"SEMICOLON";
    sym_map[ASSIGN] = L"BECOMES";
    sym_map[BEGIN_SYM] = L"BEGIN_SYM";
    sym_map[END_SYM] = L"END_SYM";
    sym_map[IF_SYM] = L"IF_SYM";
    sym_map[THEN_SYM] = L"THEN_SYM";
    sym_map[WHILE_SYM] = L"WHILE_SYM";
    sym_map[DO_SYM] = L"DO_SYM";
    sym_map[CALL_SYM] = L"CALL_SYM";
    sym_map[CONST_SYM] = L"CONST_SYM";
    sym_map[VAR_SYM] = L"VAR_SYM";
    sym_map[PROC_SYM] = L"PROC_SYM";
    sym_map[WRITE_SYM] = L"WRITE_SYM";
    sym_map[READ_SYM] = L"READ_SYM";
    sym_map[PROGM_SYM] = L"PROGM_SYM";
    sym_map[ELSE_SYM] = L"ELSE_SYM";
}

/**
 * @brief 判断当前字符是否为数字
 * @return true表示是数字，false表示不是
 */
bool Lexer::IsDigit()
{
    return (ch >= L'0' && ch <= L'9');
}

/**
 * @brief 判断当前字符是否为字母
 * @return true表示是字母，false表示不是
 */
bool Lexer::IsLetter()
{
    return ((ch >= L'a' && ch <= L'z') || (ch >= L'A' && ch <= L'Z'));
}

/**
 * @brief 判断当前字符是否为界符
 * @return true表示是界符，false表示不是
 */
bool Lexer::IsBoundary()
{
    return ch == L' ' || ch == L'\t' || ch == L'\n' || 
           ch == L'#' || ch == L'\0' || ch == L';' || ch == L',';
}

/**
 * @brief 判断当前字符是否为运算符
 * @return 运算符在表中的索引，-1表示不是运算符
 */
int Lexer::IsOperator()
{
    wchar_t* p = opr_table;
    while (p - opr_table < OPR_MAX) {
        if (ch == *p) {
            return p - opr_table;
        }
        p++;
    }
    return -1;
}

/**
 * @brief 读取下一个字符
 * @details 从源程序中读取下一个字符，更新位置指针
 */
void Lexer::GetChar()
{
    ch = readUnicode.getProgmWStr(nowPtr);
    nowPtr++;
    colPos++;
}

/**
 * @brief 跳过空白字符
 * @details 移动指针跳过连续的空格和制表符
 */
void Lexer::GetBC()
{
    while (readUnicode.getProgmWStr(nowPtr) && 
           (readUnicode.getProgmWStr(nowPtr) == L' ' || 
            readUnicode.getProgmWStr(nowPtr) == L'\t')) {
        GetChar();
    }
}

/**
 * @brief 回退一个字符
 * @details 将读取指针回退一位，用于超前搜索后的回退
 */
void Lexer::Retract()
{
    nowPtr--;
    ch = readUnicode.getProgmWStr(nowPtr);
    colPos--;
}

/**
 * @brief 将当前字符追加到词法单元字符串
 */
void Lexer::Concat()
{
    strToken += ch;
}

/**
 * @brief 查找保留字表
 * @return 保留字索引，-1表示不是保留字
 */
int Lexer::Reserve()
{
    for (int i = 0; i < RSV_WORD_MAX; i++) {
        if (resv_table[i] == strToken) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief 获取下一个词法单元
 * @details 主扫描函数，识别并返回下一个词法单元的类型和值
 */
void Lexer::GetWord()
{
    // 更新上一个合法词法单元的位置
    if (ch != L'\n') {
        preWordCol = colPos;
        preWordRow = rowPos;
    }

    strToken.clear();
    GetBC();
    GetChar();

    // 文件结束
    if (ch == L'\0') {
        return;
    }
    
    // 换行处理
    if (ch == L'\n') {
        colPos = 0;
        rowPos++;
        GetWord();  // 递归获取下一个词法单元
        return;
    }
    // 结束符
    else if (ch == L'#') {
        Concat();
        tokenType = NUL;
    }
    // 标识符或保留字
    else if (IsLetter()) {
        Concat();
        GetChar();
        while (IsLetter() || IsDigit()) {
            Concat();
            GetChar();
        }

        // 查找保留字表
        int code = Reserve();
        switch (code) {
        case -1:  tokenType = IDENT;      break;
        case 0:   tokenType = ODD_SYM;    break;
        case 1:   tokenType = BEGIN_SYM;  break;
        case 2:   tokenType = END_SYM;    break;
        case 3:   tokenType = IF_SYM;     break;
        case 4:   tokenType = THEN_SYM;   break;
        case 5:   tokenType = WHILE_SYM;  break;
        case 6:   tokenType = DO_SYM;     break;
        case 7:   tokenType = CALL_SYM;   break;
        case 8:   tokenType = CONST_SYM;  break;
        case 9:   tokenType = VAR_SYM;    break;
        case 10:  tokenType = PROC_SYM;   break;
        case 11:  tokenType = WRITE_SYM;  break;
        case 12:  tokenType = READ_SYM;   break;
        case 13:  tokenType = PROGM_SYM;  break;
        case 14:  tokenType = ELSE_SYM;   break;
        default:  tokenType = NUL;        break;
        }
        Retract();
    }
    // 数字
    else if (IsDigit()) {
        Concat();
        GetChar();
        while (IsDigit()) {
            Concat();
            GetChar();
        }
        // 数字后面紧跟字母是非法的
        if (IsLetter()) {
            errorHandle.error(ILLEGAL_WORD, (L"'" + strToken + L"'").c_str(),
                              preWordRow, preWordCol, rowPos, colPos);
            // 跳过直到下一个界符
            while (!IsBoundary()) {
                GetChar();
            }
            Retract();
            strToken.clear();
            tokenType = NUL;
        }
        else {
            tokenType = NUMBER;
            Retract();           
        }
    }
    // 赋值符号 :=
    else if (ch == L':') {
        Concat();
        GetChar();
        if (ch == L'=') {
            Concat();
            tokenType = ASSIGN;
        }
        else {
            errorHandle.error(MISSING, L"'='", preWordRow, preWordCol, rowPos, colPos);
            strToken.clear();
            tokenType = NUL;
        }
    }
    // 小于号相关: <, <=, <>
    else if (ch == L'<') {
        Concat();
        GetChar();
        if (ch == L'=') {
            Concat();
            tokenType = LEQ;
        }
        else if (ch == L'>') {
            Concat();
            tokenType = NEQ;
        }
        else {
            tokenType = LSS;
            Retract();
        }
    }
    // 大于号相关: >, >=
    else if (ch == L'>') {
        Concat();
        GetChar();
        if (ch == L'=') {
            Concat();
            tokenType = GEQ;
            preWordCol++;
        }
        else {
            tokenType = GRT;
            Retract();
        }
    }
    // 其他运算符和界符
    else {
        int code = IsOperator();
        if (code != -1) {
            Concat();
            switch (code) {
            case 0:   tokenType = PLUS;      break;
            case 1:   tokenType = MINUS;     break;
            case 2:   tokenType = MULTI;     break;
            case 3:   tokenType = DIVIS;     break;
            case 4:   tokenType = EQL;       break;
            case 7:   tokenType = LPAREN;    break;
            case 8:   tokenType = RPAREN;    break;
            case 9:   tokenType = COMMA;     break;
            case 10:  tokenType = SEMICOLON; break;
            default:  break;
            }
        }
        else {
            Concat();
            errorHandle.error(ILLEGAL_WORD, (L"'" + strToken + L"'").c_str(),
                              preWordRow, preWordCol, rowPos, colPos);
            tokenType = NUL;
        }
    }
}

/**
 * @brief 获取当前字符
 * @return 当前读入的字符
 */
wchar_t Lexer::GetCh()
{
    return ch;
}

// 词法分析器全局实例
Lexer lexer;
