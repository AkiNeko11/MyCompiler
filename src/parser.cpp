/**
 * @file Parser.cpp
 * @brief 递归下降语法分析器实现
 * @details 实现PL/0语言的语法分析，采用递归下降方法，
 *          同时完成语义检查和P-Code中间代码生成
 */

#include <parser.hpp>

// 语法分析器全局实例
Parser parser;

/**
 * @brief 报告语法错误
 * @param errorType 错误类型
 * @param expected 期望的符号
 * @param context 错误上下文
 */
void Parser::reportError(unsigned int errorType, const wchar_t *expected, const wchar_t *context)
{
    errorHandle.error(errorType, expected, context,
                      lexer.GetPreWordRow(), lexer.GetPreWordCol(), 
                      lexer.GetRowPos(), lexer.GetColPos());
}

/**
 * @brief 错误恢复函数(单参数版)
 * @param s1 期望的符号集合
 * @param s2 同步符号集合
 * @param n 错误类型
 * @param extra 额外错误信息
 * @return 1表示匹配s1, -1表示匹配s2, 2表示到达文件末尾, 0表示其他
 * @details 若当前符号不在s1中则报错，然后跳过符号直到找到s1∪s2中的符号
 */
int Parser::judge(const unsigned long s1, const unsigned long s2, const unsigned int n, const wchar_t *extra)
{
    if (!(lexer.GetTokenType() & s1)) {
        errorHandle.error(n, extra, lexer.GetPreWordRow(),
                          lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
        unsigned long s3 = s1 | s2;

        while (!(lexer.GetTokenType() & s3)) {
            if (lexer.GetCh() == L'\0') {
                return 2;
            }
            lexer.GetWord();
        }
        
        if (lexer.GetTokenType() & s1) {
            return 1;
        }
        else if (lexer.GetTokenType() & s2) {
            return -1;
        }
        else
            return 0;
    }
    else {
        return 1;
    }
}

/**
 * @brief 错误恢复函数(双参数版)
 * @param s1 期望的符号集合
 * @param s2 同步符号集合
 * @param n 错误类型
 * @param extra1 第一个额外信息
 * @param extra2 第二个额外信息
 * @return 1表示匹配s1, -1表示匹配s2, 2表示到达文件末尾, 0表示其他
 */
int Parser::judge(const unsigned long s1, const unsigned long s2, const unsigned int n, 
                  const wchar_t *extra1, const wchar_t *extra2)
{
    if (!(lexer.GetTokenType() & s1)) {
        errorHandle.error(n, extra1, extra2, lexer.GetPreWordRow(),
                          lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
        unsigned long s3 = s1 | s2;

        while (!(lexer.GetTokenType() & s3)) {
            if (lexer.GetCh() == L'\0') {
                return 2;
            }
            lexer.GetWord();
        }

        if (lexer.GetTokenType() & s1)
            return 1;
        else if (lexer.GetTokenType() & s2) {
            return -1;
        }
        else
            return 0;
    }
    else
        return 1;
}

/* ============================================================
 * PL/0 文法定义 (供参考)
 * ============================================================
 * <lop>       → = | <> | < | <= | > | >=
 * <aop>       → + | -
 * <mop>       → * | /
 * <id>        → l{l|d}  (l表示字母, d表示数字)
 * <integer>   → d{d}
 * 
 * <statement> → <id> := <exp>
 *             | if <lexp> then <statement> [else <statement>]
 *             | while <lexp> do <statement>
 *             | call <id>([<exp>{,<exp>}])
 *             | <body>
 *             | read (<id>{,<id>})
 *             | write (<exp>{,<exp>})
 * ============================================================ */

/**
 * @brief 语句处理
 * @details 处理赋值语句、条件语句、循环语句、调用语句、
 *          复合语句、读写语句等
 */
void Parser::statement()
{
    // 赋值语句: <id> := <exp>
    if (lexer.GetTokenType() == IDENT)
    {
        int pos = symTable.SearchInfo(lexer.GetStrToken(), Category::VAR);
        VarInfo *cur_info = nullptr;
        if (pos == -1)
            errorHandle.error(UNDECLARED_IDENT, lexer.GetStrToken().c_str(),
                              lexer.GetPreWordRow(), lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
        else
            cur_info = (VarInfo *)symTable.table[pos].info;
        lexer.GetWord();
        if (lexer.GetTokenType() == ASSIGN)
        {
            if (cur_info && cur_info->cat == Category::CST)
                errorHandle.error(ILLEGAL_RVALUE_ASSIGN, lexer.GetPreWordRow(),
                                  lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            lexer.GetWord();
            exp();
        }
        else if (lexer.GetTokenType() & firstExp)
        {
            errorHandle.error(MISSING, L":=", lexer.GetPreWordRow(),
                              lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            exp();
        }
        else if (lexer.GetTokenType() & EQL)
        {
            errorHandle.error(EXPECT_STH_FIND_ANTH, L":=", L"=", lexer.GetPreWordRow(),
                              lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            lexer.GetWord();
            exp();
        }
        else
            errorHandle.error(ILLEGAL_DEFINE, L"<ident>", lexer.GetPreWordRow(),
                              lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
        if (cur_info)
            // 生成存储指令
            pcodelist.emit(store, cur_info->level, cur_info->offset / UNIT_SIZE + ACT_PRE_REC_SIZE + cur_info->level + 1);
    }
    // 条件语句: if <lexp> then <statement> [else <statement>]
    else if (lexer.GetTokenType() & IF_SYM)
    {
        lexer.GetWord();
        lexp();
        int entry_jpc = -1, entry_jmp = -1;
        
        if (lexer.GetTokenType() & THEN_SYM)
        {
            entry_jpc = pcodelist.emit(jpc, 0, 0);
            lexer.GetWord();
            statement();
            if (lexer.GetTokenType() & ELSE_SYM)
            {
                entry_jmp = pcodelist.emit(jmp, 0, 0);
                lexer.GetWord();
                // 回填else入口地址
                pcodelist.backpatch(entry_jpc, pcodelist.code_list.size());
                statement();
                // 回填if结束地址
                pcodelist.backpatch(entry_jmp, pcodelist.code_list.size());
            }
            else
                pcodelist.backpatch(entry_jpc, pcodelist.code_list.size());
        }
        else if (lexer.GetTokenType() & firstStatement)
        {
            entry_jpc = pcodelist.emit(jpc, 0, 0);
            errorHandle.error(MISSING, L"then", lexer.GetPreWordRow(),
                              lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            statement();
            if (lexer.GetTokenType() & ELSE_SYM)
            {
                entry_jmp = pcodelist.emit(jmp, 0, 0);
                lexer.GetWord();
                pcodelist.backpatch(entry_jpc, pcodelist.code_list.size());
                statement();
                pcodelist.backpatch(entry_jmp, pcodelist.code_list.size());
            }
            else
                pcodelist.backpatch(entry_jpc, pcodelist.code_list.size());
        }
        else if (lexer.GetTokenType() & ELSE_SYM)
        {
            entry_jmp = pcodelist.emit(jmp, 0, 0);
            lexer.GetWord();
            statement();
            pcodelist.backpatch(entry_jmp, pcodelist.code_list.size());
        }
        else
            errorHandle.error(ILLEGAL_DEFINE, L"<if>", lexer.GetPreWordRow(),
                              lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
    }
    // 循环语句: while <lexp> do <statement>
    else if (lexer.GetTokenType() == WHILE_SYM)
    {
        lexer.GetWord();
        size_t condition = pcodelist.code_list.size();
        lexp();
        // 条件为假时跳出循环
        size_t loop = pcodelist.emit(jpc, 0, 0);
        if (lexer.GetTokenType() == DO_SYM)
        {
            lexer.GetWord();
            statement();
            // 跳回条件判断处
            pcodelist.emit(jmp, 0, condition);
        }
        else if (lexer.GetTokenType() & firstStatement)
        {
            errorHandle.error(MISSING, L"do", lexer.GetPreWordRow(),
                              lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            statement();
            pcodelist.emit(jmp, 0, condition);
        }
        else
            errorHandle.error(MISSING, L"do", lexer.GetPreWordRow(),
                              lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
        // 回填循环出口
        pcodelist.backpatch(loop, pcodelist.code_list.size());
    }
    // 调用语句: call <id>([<exp>{,<exp>}])
    else if (lexer.GetTokenType() == CALL_SYM)
    {
        lexer.GetWord();
        ProcInfo *cur_info = nullptr;
        
        if (lexer.GetTokenType() & IDENT)
        {
            int pos = symTable.SearchInfo(lexer.GetStrToken(), Category::PROCE);
            if (pos == -1)
                errorHandle.error(UNDECLARED_PROC, lexer.GetStrToken().c_str(), lexer.GetPreWordRow(),
                                  lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            else
                cur_info = (ProcInfo *)symTable.table[pos].info;
            // 检查过程是否已定义
            if (cur_info && !cur_info->isDefined)
                errorHandle.error(UNDEFINED_PROC, lexer.GetStrToken().c_str(), lexer.GetPreWordRow(),
                                  lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            lexer.GetWord();
            if (lexer.GetTokenType() & LPAREN)
            {
                lexer.GetWord();
                if (lexer.GetTokenType() & firstExp)
                {
                    exp();
                    // 传递实参
                    if (cur_info)
                        pcodelist.emit(store, -1, ACT_PRE_REC_SIZE + cur_info->level + 1 + 1);
                    size_t i = 1;
                    while ((lexer.GetTokenType() & COMMA) || (lexer.GetTokenType() & firstExp))
                    {
                        if (lexer.GetTokenType() & COMMA)
                            lexer.GetWord();
                        else
                            errorHandle.error(MISSING, L",", lexer.GetPreWordRow(),
                                              lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                        if (lexer.GetTokenType() & firstExp)
                        {
                            exp();
                            if (cur_info)
                                pcodelist.emit(store, -1, ACT_PRE_REC_SIZE + cur_info->level + 1 + 1 + i++);
                        }
                        else
                            exp();
                    }
                    // 检查参数个数
                    if (cur_info && i != cur_info->formVarList.size())
                        errorHandle.error(INCOMPATIBLE_VAR_LIST, lexer.GetPreWordRow(),
                                          lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                    if (lexer.GetTokenType() & RPAREN)
                    {
                        lexer.GetWord();
                        if (cur_info)
                            pcodelist.emit(call, cur_info->level, cur_info->entry);
                    }
                    else
                        errorHandle.error(MISSING, L")", lexer.GetPreWordRow(),
                                          lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                }
                else if (lexer.GetTokenType() & RPAREN)
                    lexer.GetWord();
                else if (lexer.GetTokenType() & followStatement)
                    errorHandle.error(MISSING, L")", lexer.GetPreWordRow(),
                                      lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            }
            else if (lexer.GetTokenType() & firstExp)
            {
                errorHandle.error(MISSING, L"(", lexer.GetPreWordRow(),
                                  lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                exp();
                if (cur_info)
                    pcodelist.emit(store, -1, ACT_PRE_REC_SIZE + cur_info->level + 2);
                size_t i = 1;
                while ((lexer.GetTokenType() & COMMA) || (lexer.GetTokenType() & firstExp))
                {
                    if (lexer.GetTokenType() & COMMA)
                        lexer.GetWord();
                    else
                        errorHandle.error(MISSING, L",", lexer.GetPreWordRow(),
                                          lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                    if (lexer.GetTokenType() & firstExp)
                    {
                        exp();
                        if (cur_info)
                            pcodelist.emit(store, -1, ACT_PRE_REC_SIZE + cur_info->level + 2 + i++);
                    }
                    else
                        exp();
                }
                if (cur_info && i != cur_info->formVarList.size())
                    errorHandle.error(INCOMPATIBLE_VAR_LIST, lexer.GetPreWordRow(),
                                      lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                if (lexer.GetTokenType() & RPAREN)
                {
                    lexer.GetWord();
                    if (cur_info)
                        pcodelist.emit(call, cur_info->level, cur_info->entry);
                }
                else
                    errorHandle.error(MISSING, L")", lexer.GetPreWordRow(),
                                      lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            }
        }
        else if (lexer.GetTokenType() & LPAREN)
        {
            errorHandle.error(MISSING, L"<id>", lexer.GetPreWordRow(),
                              lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            lexer.GetWord();
            if (lexer.GetTokenType() & firstExp)
            {
                exp();
                while ((lexer.GetTokenType() & COMMA) || (lexer.GetTokenType() & firstExp))
                {
                    if (lexer.GetTokenType() & COMMA)
                        lexer.GetWord();
                    else
                        errorHandle.error(MISSING, L",", lexer.GetPreWordRow(),
                                          lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                    exp();
                }

                if (lexer.GetTokenType() & RPAREN)
                    lexer.GetWord();
                else
                    errorHandle.error(MISSING, L")", lexer.GetPreWordRow(),
                                      lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            }
            else if (lexer.GetTokenType() & RPAREN)
                lexer.GetWord();
            else if (lexer.GetTokenType() & followStatement)
                errorHandle.error(MISSING, L")", lexer.GetPreWordRow(),
                                  lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
        }
        else if (lexer.GetTokenType() & RPAREN)
        {
            errorHandle.error(ILLEGAL_DEFINE, L"<call>", lexer.GetPreWordRow(),
                              lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            lexer.GetWord();
        }
        else
            errorHandle.error(ILLEGAL_DEFINE, L"<call>", lexer.GetPreWordRow(),
                              lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
    }
    // 复合语句
    else if (lexer.GetTokenType() == BEGIN_SYM)
        body();
    // 读语句: read (<id>{,<id>})
    else if (lexer.GetTokenType() == READ_SYM)
    {
        lexer.GetWord();
        if (lexer.GetTokenType() == LPAREN)
        {
            lexer.GetWord();
            if (lexer.GetTokenType() & IDENT)
            {
                int pos = symTable.SearchInfo(lexer.GetStrToken(), Category::VAR);
                VarInfo *cur_info = nullptr;
                if (pos == -1)
                    errorHandle.error(UNDECLARED_PROC, lexer.GetStrToken().c_str(), lexer.GetPreWordRow(),
                                      lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                else
                    cur_info = (VarInfo *)symTable.table[pos].info;
                if (cur_info)
                {
                    if (cur_info->cat == Category::CST)
                        errorHandle.error(ILLEGAL_RVALUE_ASSIGN, lexer.GetPreWordRow(),
                                          lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                    pcodelist.emit(red, 0, 0);
                    pcodelist.emit(store, cur_info->level, cur_info->offset / UNIT_SIZE + ACT_PRE_REC_SIZE + cur_info->level + 1);
                }
                lexer.GetWord();
                while (lexer.GetTokenType() & COMMA)
                {
                    lexer.GetWord();
                    if (lexer.GetTokenType() & IDENT)
                    {
                        int pos1 = symTable.SearchInfo(lexer.GetStrToken(), Category::VAR);
                        VarInfo *cur_info1 = nullptr;
                        if (pos1 == -1)
                            errorHandle.error(UNDECLARED_PROC, lexer.GetStrToken().c_str(), lexer.GetPreWordRow(),
                                              lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                        else
                            cur_info1 = (VarInfo *)symTable.table[pos1].info;
                        if (cur_info1)
                        {
                            if (cur_info1->cat == Category::CST)
                                errorHandle.error(ILLEGAL_RVALUE_ASSIGN, lexer.GetPreWordRow(),
                                                  lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                            pcodelist.emit(red, 0, 0);
                            pcodelist.emit(store, cur_info1->level, cur_info1->offset / UNIT_SIZE + ACT_PRE_REC_SIZE + cur_info1->level + 1);
                        }
                        lexer.GetWord();
                    }
                    else if (lexer.GetTokenType() & COMMA)
                        errorHandle.error(MISSING, L"<id>", lexer.GetPreWordRow(),
                                          lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                    else
                    {
                        errorHandle.error(MISSING, L"<id>", lexer.GetPreWordRow(),
                                          lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                        break;
                    }
                }
                if (lexer.GetTokenType() & RPAREN)
                    lexer.GetWord();
                else if (lexer.GetTokenType() & followStatement)
                    errorHandle.error(MISSING, L")", lexer.GetPreWordRow(),
                                      lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            }
            else if (lexer.GetTokenType() & RPAREN)
            {
                errorHandle.error(MISSING, L"<id>", lexer.GetPreWordRow(),
                                  lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                lexer.GetWord();
            }
            else if (lexer.GetTokenType() & COMMA)
            {
                errorHandle.error(MISSING, L"<id>", lexer.GetPreWordRow(),
                                  lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                while (lexer.GetTokenType() & COMMA)
                {
                    lexer.GetWord();
                    if (lexer.GetTokenType() & IDENT)
                    {
                        int pos = symTable.SearchInfo(lexer.GetStrToken(), Category::VAR);
                        VarInfo *cur_info = nullptr;
                        if (pos == -1)
                            errorHandle.error(UNDECLARED_PROC, lexer.GetStrToken().c_str(), lexer.GetPreWordRow(),
                                              lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                        else
                            cur_info = (VarInfo *)symTable.table[pos].info;
                        if (cur_info)
                        {
                            if (cur_info->cat == Category::CST)
                                errorHandle.error(ILLEGAL_RVALUE_ASSIGN, lexer.GetPreWordRow(),
                                                  lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                            pcodelist.emit(red, 0, 0);
                            pcodelist.emit(store, cur_info->level, cur_info->offset / UNIT_SIZE + ACT_PRE_REC_SIZE + cur_info->level + 1);
                        }
                        lexer.GetWord();
                    }
                    else if (lexer.GetTokenType() & COMMA)
                        errorHandle.error(MISSING, L"<id>", lexer.GetPreWordRow(),
                                          lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());

                    else
                    {
                        errorHandle.error(MISSING, L"<id>", lexer.GetPreWordRow(),
                                          lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                        break;
                    }
                }
                if (lexer.GetTokenType() & RPAREN)
                    lexer.GetWord();
                else if (lexer.GetTokenType() & followStatement)
                    errorHandle.error(MISSING, L")", lexer.GetPreWordRow(),
                                      lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            }
            else if (lexer.GetTokenType() & followStatement)
                errorHandle.error(MISSING, L")", lexer.GetPreWordRow(),
                                  lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
        }
        else if (lexer.GetTokenType() & IDENT)
        {
            errorHandle.error(MISSING, L"(", lexer.GetPreWordRow(),
                              lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            int pos = symTable.SearchInfo(lexer.GetStrToken(), Category::VAR);
            VarInfo *cur_info = nullptr;
            if (pos == -1)
                errorHandle.error(UNDECLARED_PROC, lexer.GetStrToken().c_str(), lexer.GetPreWordRow(),
                                  lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            else
                cur_info = (VarInfo *)symTable.table[pos].info;
            if (cur_info)
            {
                if (cur_info->cat == Category::CST)
                    errorHandle.error(ILLEGAL_RVALUE_ASSIGN, lexer.GetPreWordRow(),
                                      lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                pcodelist.emit(red, 0, 0);
                pcodelist.emit(store, cur_info->level, cur_info->offset / UNIT_SIZE + ACT_PRE_REC_SIZE + cur_info->level + 1);
            }
            lexer.GetWord();
            while (lexer.GetTokenType() & COMMA)
            {
                lexer.GetWord();
                if (lexer.GetTokenType() & IDENT)
                {
                    int pos1 = symTable.SearchInfo(lexer.GetStrToken(), Category::VAR);
                    VarInfo *cur_info1 = nullptr;
                    if (pos1 == -1)
                        errorHandle.error(UNDECLARED_PROC, lexer.GetStrToken().c_str(), lexer.GetPreWordRow(),
                                          lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                    else
                        cur_info1 = (VarInfo *)symTable.table[pos1].info;
                    if (cur_info1)
                    {
                        if (cur_info1->cat == Category::CST)
                            errorHandle.error(ILLEGAL_RVALUE_ASSIGN, lexer.GetPreWordRow(),
                                              lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                        pcodelist.emit(red, 0, 0);
                        pcodelist.emit(store, cur_info1->level, cur_info1->offset / UNIT_SIZE + ACT_PRE_REC_SIZE + cur_info1->level + 1);
                    }
                    lexer.GetWord();
                }
                else if (lexer.GetTokenType() & COMMA)
                    errorHandle.error(MISSING, L"<id>", lexer.GetPreWordRow(),
                                      lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());

                else
                {
                    errorHandle.error(MISSING, L"<id>", lexer.GetPreWordRow(),
                                      lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                    break;
                }
            }
            if (lexer.GetTokenType() & RPAREN)
                lexer.GetWord();
            else if (lexer.GetTokenType() & followStatement)
                errorHandle.error(MISSING, L")", lexer.GetPreWordRow(),
                                  lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
        }
        else if (lexer.GetTokenType() & RPAREN)
        {
            errorHandle.error(ILLEGAL_DEFINE, L"<read>", lexer.GetPreWordRow(),
                              lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            lexer.GetWord();
        }
        else if (lexer.GetTokenType() & COMMA)
        {
            errorHandle.error(MISSING, L"(<id>", lexer.GetPreWordRow(),
                              lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            while (lexer.GetTokenType() & COMMA)
            {
                lexer.GetWord();
                if (lexer.GetTokenType() & IDENT)
                {
                    int pos = symTable.SearchInfo(lexer.GetStrToken(), Category::VAR);
                    VarInfo *cur_info = nullptr;
                    if (pos == -1)
                        errorHandle.error(UNDECLARED_PROC, lexer.GetStrToken().c_str(), lexer.GetPreWordRow(),
                                          lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                    else
                        cur_info = (VarInfo *)symTable.table[pos].info;
                    if (cur_info)
                    {
                        if (cur_info->cat == Category::CST)
                            errorHandle.error(ILLEGAL_RVALUE_ASSIGN, lexer.GetPreWordRow(),
                                              lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                        pcodelist.emit(red, 0, 0);
                        pcodelist.emit(store, cur_info->level, cur_info->offset / UNIT_SIZE + ACT_PRE_REC_SIZE + cur_info->level + 1);
                    }
                    lexer.GetWord();
                }
                else if (lexer.GetTokenType() & COMMA)
                    errorHandle.error(MISSING, L"<id>", lexer.GetPreWordRow(),
                                      lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                else
                {
                    errorHandle.error(MISSING, L"<id>", lexer.GetPreWordRow(),
                                      lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                    break;
                }
            }
            if (lexer.GetTokenType() & RPAREN)
                lexer.GetWord();
            else if (lexer.GetTokenType() & followStatement)
                errorHandle.error(MISSING, L")", lexer.GetPreWordRow(),
                                  lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
        }
        else
            judge(0, followStatement, ILLEGAL_DEFINE, L"<read>");
    }
    // 写语句: write (<exp>{,<exp>})
    else if (lexer.GetTokenType() == WRITE_SYM)
    {
        lexer.GetWord();
        if (lexer.GetTokenType() & LPAREN)
        {
            lexer.GetWord();
            if (lexer.GetTokenType() & firstExp)
            {
                exp();
                pcodelist.emit(wrt, 0, 0);
                while ((lexer.GetTokenType() & COMMA) || (lexer.GetTokenType() & firstExp))
                {
                    if (lexer.GetTokenType() & COMMA)
                        lexer.GetWord();
                    else
                        errorHandle.error(MISSING, L",", lexer.GetPreWordRow(),
                                          lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                    if (lexer.GetTokenType() & firstExp)
                    {
                        exp();
                        pcodelist.emit(wrt, 0, 0);
                    }
                    else
                        exp();
                }
                if (lexer.GetTokenType() & RPAREN)
                    lexer.GetWord();
                else if (lexer.GetTokenType() & followStatement)
                    errorHandle.error(MISSING, L")", lexer.GetPreWordRow(),
                                      lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            }
            else if (lexer.GetTokenType() & RPAREN)
            {
                errorHandle.error(MISSING, L"<exp>", lexer.GetPreWordRow(),
                                  lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                lexer.GetWord();
            }
            else if (lexer.GetTokenType() & COMMA)
            {
                errorHandle.error(MISSING, L"<exp>", lexer.GetPreWordRow(),
                                  lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                while ((lexer.GetTokenType() & COMMA) || (lexer.GetTokenType() & firstExp))
                {
                    if (lexer.GetTokenType() & COMMA)
                        lexer.GetWord();
                    else
                        errorHandle.error(MISSING, L",", lexer.GetPreWordRow(),
                                          lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                    if (lexer.GetTokenType() & firstExp)
                    {
                        exp();
                        pcodelist.emit(wrt, 0, 0);
                    }
                    else
                        exp();
                }
                if (lexer.GetTokenType() & RPAREN)
                    lexer.GetWord();
                else if (lexer.GetTokenType() & followStatement)
                    errorHandle.error(MISSING, L")", lexer.GetPreWordRow(),
                                      lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            }
            else if (lexer.GetTokenType() & followStatement)
                errorHandle.error(MISSING, L")", lexer.GetPreWordRow(),
                                  lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
        }
        else if (lexer.GetTokenType() & firstExp)
        {
            errorHandle.error(MISSING, L"(", lexer.GetPreWordRow(),
                              lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            exp();
            pcodelist.emit(wrt, 0, 0);
            while ((lexer.GetTokenType() & COMMA) || (lexer.GetTokenType() & firstExp))
            {
                if (lexer.GetTokenType() & COMMA)
                    lexer.GetWord();
                else
                    errorHandle.error(MISSING, L",", lexer.GetPreWordRow(),
                                      lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                if (lexer.GetTokenType() & firstExp)
                {
                    exp();
                    pcodelist.emit(wrt, 0, 0);
                }
                else
                    exp();
            }
            if (lexer.GetTokenType() & RPAREN)
                lexer.GetWord();
            else if (lexer.GetTokenType() & followStatement)
                errorHandle.error(MISSING, L")", lexer.GetPreWordRow(),
                                  lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
        }
        else if (lexer.GetTokenType() & RPAREN)
        {
            errorHandle.error(ILLEGAL_DEFINE, L"<write>", lexer.GetPreWordRow(),
                              lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            lexer.GetWord();
        }
        else if (lexer.GetTokenType() & COMMA)
        {
            errorHandle.error(MISSING, L"(<exp>", lexer.GetPreWordRow(),
                              lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            while ((lexer.GetTokenType() & COMMA) || (lexer.GetTokenType() & firstExp))
            {
                if (lexer.GetTokenType() & COMMA)
                    lexer.GetWord();
                else
                    errorHandle.error(MISSING, L",", lexer.GetPreWordRow(),
                                      lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                if (lexer.GetTokenType() & firstExp)
                {
                    exp();
                    pcodelist.emit(wrt, 0, 0);
                }
                else
                    exp();
            }
            if (lexer.GetTokenType() & RPAREN)
                lexer.GetWord();
            else if (lexer.GetTokenType() & followStatement)
                errorHandle.error(MISSING, L")", lexer.GetPreWordRow(),
                                  lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
        }
        else
            judge(0, followStatement, ILLEGAL_DEFINE, L"<write>");
        // 输出换行
        pcodelist.emit(opr, 0, 13);
    }
    else
        judge(0, followStatement, ILLEGAL_DEFINE, L"statement");
}

/**
 * @brief 表达式处理
 * @details 文法: <exp> → [+|-]<term>{<aop><term>}
 */
void Parser::exp()
{
    unsigned long aop = NUL;
    if (lexer.GetTokenType() & firstExp)
    {
        // 处理可选的正负号
        if (lexer.GetTokenType() & (PLUS | MINUS))
        {
            aop = lexer.GetTokenType();
            lexer.GetWord();
        }

        if (lexer.GetTokenType() & firstTerm)
        {
            term();
            // 负号取反
            if (aop & MINUS)
                pcodelist.emit(opr, 0, OPR_NEGTIVE);
            // 处理加减运算
            while (lexer.GetTokenType() & (PLUS | MINUS))
            {
                aop = lexer.GetTokenType();
                lexer.GetWord();
                if (lexer.GetTokenType() & firstTerm)
                {
                    term();
                    if (aop == MINUS)
                        pcodelist.emit(opr, 0, OPR_SUB);
                    else
                        pcodelist.emit(opr, 0, OPR_ADD);
                }
                else
                    errorHandle.error(REDUNDENT, lexer.GetStrToken().c_str(), lexer.GetPreWordRow(),
                                      lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            }
        }
    }
    else
        judge(0, followExp, ILLEGAL_DEFINE, L"expression (invalid expression start)");
}

/**
 * @brief 项处理
 * @details 文法: <term> → <factor>{<mop><factor>}
 */
void Parser::term()
{
    if (lexer.GetTokenType() & firstTerm)
    {
        factor();
        // 处理乘除运算
        while (lexer.GetTokenType() & (MULTI | DIVIS))
        {
            unsigned long nop = lexer.GetTokenType();
            lexer.GetWord();
            if (lexer.GetTokenType() & firstFactor)
            {
                factor();
                if (nop == MULTI)
                    pcodelist.emit(opr, 0, OPR_MULTI);
                else
                    pcodelist.emit(opr, 0, OPR_DIVIS);
            }
            else if (lexer.GetTokenType() & (MULTI | DIVIS))
                errorHandle.error(SYNTAX_ERROR, L"<factor>",
                                  L"Two consecutive operators found. Expected a <factor> after '*' or '/'.",
                                  lexer.GetPreWordRow(), lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            else
                errorHandle.error(EXPECT, L"a valid <factor> after '*' or '/'.",
                                  lexer.GetPreWordRow(), lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
        }
    }
    else
        judge(0, followTerm, ILLEGAL_DEFINE, L"term (invalid term start)");
}

/**
 * @brief 因子处理
 * @details 文法: <factor> → <id> | <integer> | (<exp>)
 */
void Parser::factor()
{
    // 标识符
    if (lexer.GetTokenType() == IDENT)
    {
        int pos = symTable.SearchInfo(lexer.GetStrToken(), Category::VAR);
        VarInfo *cur_info = nullptr;
        if (pos == -1)
            errorHandle.error(UNDECLARED_IDENT, lexer.GetStrToken().c_str(),
                              lexer.GetPreWordRow(), lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
        else
            cur_info = (VarInfo *)symTable.table[pos].info;
        if (cur_info)
        {
            // 常量直接加载值
            if (cur_info->cat == Category::CST)
            {
                int val = cur_info->GetValue();
                pcodelist.emit(lit, cur_info->level, val);
            }
            // 变量从内存加载
            else
                pcodelist.emit(load, cur_info->level, cur_info->offset / UNIT_SIZE + ACT_PRE_REC_SIZE + cur_info->level + 1);
        }
        lexer.GetWord();
    }
    // 数字常量
    else if (lexer.GetTokenType() == NUMBER)
    {
        pcodelist.emit(lit, 0, w_str2int(lexer.GetStrToken()));
        lexer.GetWord();
    }
    // 括号表达式
    else if (lexer.GetTokenType() == LPAREN)
    {
        lexer.GetWord();
        exp();
        if (lexer.GetTokenType() == RPAREN)
            lexer.GetWord();
        else
            errorHandle.error(MISSING_DETAILED, L"')'",
                              L"Expected closing parenthesis ')'.",
                              lexer.GetPreWordRow(), lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
    }
    else
        judge(0, followFactor, ILLEGAL_DEFINE, L"factor");
}

/**
 * @brief 复合语句处理
 * @details 文法: <body> → begin <statement>{;<statement>} end
 */
void Parser::body()
{
    if (lexer.GetTokenType() == BEGIN_SYM)
    {
        lexer.GetWord();
        statement();
        while ((lexer.GetTokenType() & SEMICOLON) || (lexer.GetTokenType() & firstStatement))
        {
            if (lexer.GetTokenType() & SEMICOLON)
                lexer.GetWord();
            else
                errorHandle.error(MISSING, L";", lexer.GetPreWordRow(),
                                  lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            statement();
        }
        if (lexer.GetTokenType() & END_SYM)
            lexer.GetWord();
        else
            judge(0, followBody, MISSING, L"end");
    }
    else if (lexer.GetTokenType() & firstStatement)
    {
        errorHandle.error(MISSING, L"begin", lexer.GetPreWordRow(),
                          lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());

        while ((lexer.GetTokenType() & SEMICOLON) || (lexer.GetTokenType() & firstStatement))
        {
            if (lexer.GetTokenType() & SEMICOLON)
                lexer.GetWord();
            else
                errorHandle.error(MISSING, L";", lexer.GetPreWordRow(),
                                  lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            statement();
        }
        if (lexer.GetTokenType() & END_SYM)
            lexer.GetWord();
        else
            judge(0, followBody, MISSING, L"end");
    }
    else if (lexer.GetTokenType() & END_SYM)
    {
        errorHandle.error(ILLEGAL_DEFINE, L"<body>", lexer.GetPreWordRow(),
                          lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
        lexer.GetWord();
    }
    else
        judge(0, followBody, ILLEGAL_DEFINE, L"'<body>'");
}

/**
 * @brief 条件表达式处理
 * @details 文法: <lexp> → <exp> <lop> <exp> | odd <exp>
 */
void Parser::lexp()
{
    if (lexer.GetTokenType() & firstExp)
    {
        exp();
        // 检查关系运算符
        if (lexer.GetTokenType() & (EQL | NEQ | LSS | LEQ | GRT | GEQ))
        {
            unsigned int lop = lexer.GetTokenType();
            lexer.GetWord();
            exp();
            // 生成比较指令
            switch (lop)
            {
            case LSS:
                pcodelist.emit(opr, 0, OPR_LSS);
                break;
            case LEQ:
                pcodelist.emit(opr, 0, OPR_LEQ);
                break;
            case GRT:
                pcodelist.emit(opr, 0, OPR_GRT);
                break;
            case GEQ:
                pcodelist.emit(opr, 0, OPR_GEQ);
                break;
            case NEQ:
                pcodelist.emit(opr, 0, OPR_NEQ);
                break;
            case EQL:
                pcodelist.emit(opr, 0, OPR_EQL);
                break;
            default:
                break;
            }
        }
        else
        {
            errorHandle.error(MISSING,
                              L"Expected a logical operator (e.g., '=', '<>', '<') after the expression.",
                              lexer.GetPreWordRow(), lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            lexer.GetWord();
            exp();
        }
    }
    // odd表达式
    else if (lexer.GetTokenType() & ODD_SYM)
    {
        lexer.GetWord();
        if (lexer.GetTokenType() & firstExp)
        {
            exp();
            pcodelist.emit(opr, 0, OPR_ODD);
        }
        else
            errorHandle.error(EXPECT, L"expression", lexer.GetPreWordRow(), lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
    }
    else
        judge(0, followLexp, ILLEGAL_DEFINE, L"lexp (invalid logical expression start)");
}

/**
 * @brief 变量声明处理
 * @details 文法: <vardecl> → var <id>{,<id>};
 */
void Parser::vardecl()
{
    if (lexer.GetTokenType() == VAR_SYM)
    {
        lexer.GetWord();
        if (lexer.GetTokenType() & IDENT)
        {
            symTable.InsertToTable(lexer.GetStrToken(), glo_offset, Category::VAR);
            glo_offset += 4;
            lexer.GetWord();
            while (lexer.GetTokenType() == COMMA)
            {
                lexer.GetWord();
                if (lexer.GetTokenType() & IDENT)
                {
                    symTable.InsertToTable(lexer.GetStrToken(), glo_offset, Category::VAR);
                    glo_offset += 4;
                    lexer.GetWord();
                }
                else
                    errorHandle.error(MISSING, L"<id>", lexer.GetPreWordRow(),
                                      lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            }
            if (lexer.GetTokenType() & SEMICOLON)
                lexer.GetWord();
            else
                judge(0, SEMICOLON, MISSING, L";");
        }
        else if (lexer.GetTokenType() & COMMA)
        {
            errorHandle.error(MISSING, L"<id>", lexer.GetPreWordRow(),
                              lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            while (lexer.GetTokenType() == COMMA)
            {
                lexer.GetWord();
                if (lexer.GetTokenType() & IDENT)
                {
                    symTable.InsertToTable(lexer.GetStrToken(), glo_offset, Category::VAR);
                    glo_offset += 4;
                    lexer.GetWord();
                }
                else
                    errorHandle.error(MISSING, L"<id>", lexer.GetPreWordRow(),
                                      lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            }
            if (lexer.GetTokenType() & SEMICOLON)
                lexer.GetWord();
            else
                judge(0, SEMICOLON, MISSING, L";");
        }
        else if (lexer.GetTokenType() & SEMICOLON)
        {
            errorHandle.error(MISSING, L"<id>", lexer.GetPreWordRow(),
                              lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            lexer.GetWord();
        }
        else
            judge(0, followVardecl, ILLEGAL_DEFINE, L"<var>");
    }
    else
        judge(0, followVardecl, ILLEGAL_DEFINE, L"<vardecl>");
}

/**
 * @brief 常量定义处理
 * @details 文法: <const> → <id> := <integer>
 */
void Parser::constA()
{
    if (lexer.GetTokenType() == IDENT)
    {
        symTable.InsertToTable(lexer.GetStrToken(), 0, CST);
        lexer.GetWord();
        if (lexer.GetTokenType() == ASSIGN)
            lexer.GetWord();
        else if (lexer.GetTokenType() != ASSIGN && lexer.GetTokenType() != NUMBER)
        {
            errorHandle.error(MISSING, L":=", lexer.GetPreWordRow(),
                              lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            lexer.GetWord();
        }
        else if (lexer.GetTokenType() == NUMBER)
        {
            errorHandle.error(MISSING, L":=", lexer.GetPreWordRow(),
                              lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            symTable.table[symTable.table.size() - 1].info->SetValue(lexer.GetStrToken());
            lexer.GetWord();
            return;
        }
        if (lexer.GetTokenType() == NUMBER)
        {
            symTable.table[symTable.table.size() - 1].info->SetValue(lexer.GetStrToken());
            lexer.GetWord();
        }
        else
            errorHandle.error(MISSING, L"[number]", lexer.GetPreWordRow(),
                              lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
    }
    else if (lexer.GetTokenType() == ASSIGN)
    {
        errorHandle.error(MISSING, L"<id>", lexer.GetPreWordRow(),
                          lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
        lexer.GetWord();
        lexer.GetWord();
    }
    else if (lexer.GetTokenType() == NUMBER)
    {
        errorHandle.error(MISSING, L"<id>:=", lexer.GetPreWordRow(),
                          lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
        lexer.GetWord();
    }
    else
        judge(0, followConst, ILLEGAL_DEFINE, L"<const>");
}

/**
 * @brief 常量声明处理
 * @details 文法: <condecl> → const <const>{,<const>};
 */
void Parser::condecl()
{
    if (lexer.GetTokenType() == CONST_SYM)
    {
        lexer.GetWord();
        if (lexer.GetTokenType() & firstConst)
        {
            constA();
            while (lexer.GetTokenType() & COMMA)
            {
                lexer.GetWord();
                constA();
            }
            if (lexer.GetTokenType() & SEMICOLON)
                lexer.GetWord();
            else
                judge(0, followCondecl, MISSING, L";");
        }
        else if (lexer.GetTokenType() & COMMA)
        {
            errorHandle.error(MISSING, L"<const>", lexer.GetPreWordRow(),
                              lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            while (lexer.GetTokenType() & COMMA)
            {
                lexer.GetWord();
                constA();
            }
            if (lexer.GetTokenType() & SEMICOLON)
                lexer.GetWord();
            else
                judge(0, followCondecl, MISSING, L";");
        }
        else if (lexer.GetTokenType() & SEMICOLON)
        {
            errorHandle.error(MISSING, L"<const>", lexer.GetPreWordRow(),
                              lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            lexer.GetWord();
        }
        else
            judge(0, followCondecl, ILLEGAL_DEFINE, L"<condecl>");
    }
}

/**
 * @brief 过程声明处理
 * @details 文法: <proc> → procedure <id>([<id>{,<id>}]);<block>{;<proc>}
 */
void Parser::proc()
{
    int flag = 0;
    if (lexer.GetTokenType() == PROC_SYM)
    {
        lexer.GetWord();
        ProcInfo *cur_info = nullptr;
        
        if (lexer.GetTokenType() == IDENT)
        {
            symTable.MkTable();
            int cur_proc = symTable.InsertToTable(lexer.GetStrToken(), 0, Category::PROCE);
            if (cur_proc != -1)
            {
                cur_info = (ProcInfo *)symTable.table[cur_proc].info;
                size_t entry = pcodelist.emit(jmp, 0, 0);
                symTable.table[symTable.table.size() - 1].info->SetEntry(entry);
            }
            lexer.GetWord();
            if (lexer.GetTokenType() & LPAREN)
            {
                // 进入新层次
                symTable.display.push_back(0);
                symTable.level++;

                lexer.GetWord();
                if (lexer.GetTokenType() & IDENT)
                {
                    int form_var = symTable.InsertToTable(lexer.GetStrToken(), glo_offset, Category::FORM);
                    glo_offset += 4;
                    if (cur_info)
                        cur_info->formVarList.push_back(form_var);

                    lexer.GetWord();
                    while ((lexer.GetTokenType() & COMMA) || (lexer.GetTokenType() & IDENT))
                    {
                        if (lexer.GetTokenType() & COMMA)
                            lexer.GetWord();
                        else
                            errorHandle.error(MISSING, L"','",
                                              lexer.GetPreWordRow(), lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                        if (lexer.GetTokenType() & IDENT)
                        {
                            int form_var = symTable.InsertToTable(lexer.GetStrToken(), glo_offset, Category::FORM);
                            glo_offset += 4;
                            if (cur_info)
                                cur_info->formVarList.push_back(form_var);
                            lexer.GetWord();
                        }
                        else
                            errorHandle.error(MISSING, L"'<id>'",
                                              lexer.GetPreWordRow(), lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                    }
                }
                if (lexer.GetTokenType() & RPAREN)
                {
                    lexer.GetWord();
                    if (lexer.GetTokenType() & SEMICOLON)
                    {
                        lexer.GetWord();
                        block();
                        // 生成返回指令
                        pcodelist.emit(opr, 0, OPR_RETURN);
                        // 退出当前层次
                        symTable.display.pop_back();
                        symTable.level--;

                        while (lexer.GetTokenType() & SEMICOLON)
                        {
                            lexer.GetWord();
                            proc();
                        }
                    }
                    else
                    {
                        errorHandle.error(MISSING, L"';'",
                                          lexer.GetPreWordRow(), lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                        block();
                        pcodelist.emit(opr, 0, OPR_RETURN);
                        symTable.display.pop_back();
                        symTable.level--;
                        while (lexer.GetTokenType() & SEMICOLON)
                        {
                            lexer.GetWord();
                            proc();
                        }
                    }
                }
            }
            else if (lexer.GetTokenType() & IDENT)
            {
                symTable.display.push_back(0);
                symTable.level++;

                int form_var = symTable.InsertToTable(lexer.GetStrToken(), glo_offset, Category::FORM);
                glo_offset += 4;
                if (cur_info)
                    cur_info->formVarList.push_back(form_var);
                errorHandle.error(MISSING, L"'('",
                                  lexer.GetPreWordRow(), lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                lexer.GetWord();
                while ((lexer.GetTokenType() & COMMA) || (lexer.GetTokenType() & IDENT))
                {
                    if (lexer.GetTokenType() & COMMA)
                        lexer.GetWord();
                    else
                        errorHandle.error(MISSING, L"','",
                                          lexer.GetPreWordRow(), lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                    if (lexer.GetTokenType() & IDENT)
                    {
                        int form_var = symTable.InsertToTable(lexer.GetStrToken(), glo_offset, Category::FORM);
                        glo_offset += 4;
                        if (cur_info)
                            cur_info->formVarList.push_back(form_var);
                        lexer.GetWord();
                    }
                    else
                        errorHandle.error(MISSING, L"'<id>'",
                                          lexer.GetPreWordRow(), lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                }
                if (lexer.GetTokenType() & RPAREN)
                {
                    lexer.GetWord();
                    if (lexer.GetTokenType() & SEMICOLON)
                    {
                        lexer.GetWord();
                        block();
                        pcodelist.emit(opr, 0, OPR_RETURN);
                        symTable.display.pop_back();
                        symTable.level--;

                        while (lexer.GetTokenType() & SEMICOLON)
                        {
                            lexer.GetWord();
                            proc();
                        }
                    }
                    else
                    {
                        errorHandle.error(MISSING, L"';'",
                                          lexer.GetPreWordRow(), lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                        block();
                        pcodelist.emit(opr, 0, OPR_RETURN);
                        symTable.display.pop_back();
                        symTable.level--;

                        while (lexer.GetTokenType() & SEMICOLON)
                        {
                            lexer.GetWord();
                            proc();
                        }
                    }
                }
            }
            else if (lexer.GetTokenType() & RPAREN)
            {
                symTable.display.push_back(0);
                symTable.level++;

                errorHandle.error(MISSING, L"'('",
                                  lexer.GetPreWordRow(), lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                lexer.GetWord();
                if (lexer.GetTokenType() & SEMICOLON)
                {
                    lexer.GetWord();
                    block();
                    pcodelist.emit(opr, 0, OPR_RETURN);
                    symTable.level--;
                    symTable.display.pop_back();
                    while (lexer.GetTokenType() & SEMICOLON)
                    {
                        lexer.GetWord();
                        proc();
                    }
                }
                else
                {
                    errorHandle.error(MISSING, L"';'",
                                      lexer.GetPreWordRow(), lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                    block();
                    pcodelist.emit(opr, 0, OPR_RETURN);
                    symTable.level--;
                    symTable.display.pop_back();
                    while (lexer.GetTokenType() & SEMICOLON)
                    {
                        lexer.GetWord();
                        proc();
                    }
                }
            }
        }
        else if (lexer.GetTokenType() & LPAREN)
        {
            symTable.MkTable();
            int cur_proc = symTable.InsertToTable(L"null", 0, Category::PROCE);
            if (cur_proc != -1)
            {
                cur_info = (ProcInfo *)symTable.table[cur_proc].info;
                size_t entry = pcodelist.emit(jmp, 0, 0);
                symTable.table[symTable.table.size() - 1].info->SetEntry(entry);
            }
            symTable.display.push_back(0);
            symTable.level++;
            errorHandle.error(MISSING, L"'<id>'",
                              lexer.GetPreWordRow(), lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());

            lexer.GetWord();
            if (lexer.GetTokenType() & IDENT)
            {
                int form_var = symTable.InsertToTable(lexer.GetStrToken(), glo_offset, Category::FORM);
                glo_offset += 4;
                if (cur_info)
                    cur_info->formVarList.push_back(form_var);
                lexer.GetWord();
                while ((lexer.GetTokenType() & COMMA) || (lexer.GetTokenType() & IDENT))
                {
                    if (lexer.GetTokenType() & COMMA)
                        lexer.GetWord();
                    else
                        errorHandle.error(MISSING, L"','",
                                          lexer.GetPreWordRow(), lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                    if (lexer.GetTokenType() & IDENT)
                    {
                        int form_var = symTable.InsertToTable(lexer.GetStrToken(), glo_offset, Category::FORM);
                        glo_offset += 4;
                        if (cur_info)
                            cur_info->formVarList.push_back(form_var);
                        lexer.GetWord();
                    }
                    else
                        errorHandle.error(MISSING, L"'<id>'",
                                          lexer.GetPreWordRow(), lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                }
            }
            if (lexer.GetTokenType() & RPAREN)
            {
                lexer.GetWord();
                if (lexer.GetTokenType() & SEMICOLON)
                {
                    lexer.GetWord();
                    block();
                    pcodelist.emit(opr, 0, OPR_RETURN);
                    symTable.level--;
                    symTable.display.pop_back();
                    while (lexer.GetTokenType() & SEMICOLON)
                    {
                        lexer.GetWord();
                        proc();
                    }
                }
                else
                {
                    errorHandle.error(MISSING, L"';'",
                                      lexer.GetPreWordRow(), lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
                    block();
                    pcodelist.emit(opr, 0, OPR_RETURN);
                    symTable.level--;
                    symTable.display.pop_back();
                    while (lexer.GetTokenType() & SEMICOLON)
                    {
                        lexer.GetWord();
                        proc();
                    }
                }
            }
        }
    }
    else
        judge(0, followProc, ILLEGAL_DEFINE, L"procedure");
}

/**
 * @brief 分程序处理
 * @details 文法: <block> → [<condecl>][<vardecl>][<proc>]<body>
 */
void Parser::block()
{
    int r = judge(firstBlock, followBlock, MISSING, L"body");
    if (r == 1)
    {
        // 常量声明
        if (lexer.GetTokenType() & firstCondecl)
            condecl();

        // 变量声明
        if (lexer.GetTokenType() & firstVardecl)
            vardecl();

        size_t cur_proc = symTable.sp;
        ProcInfo *cur_info = (ProcInfo *)symTable.table[cur_proc].info;
        symTable.AddWidth(cur_proc, glo_offset);
        
        // 过程声明
        if (lexer.GetTokenType() & firstProc)
            proc();
        
        // 生成分配空间指令
        size_t entry = pcodelist.emit(alloc, 0, cur_info->offset / UNIT_SIZE + ACT_PRE_REC_SIZE + symTable.level + 1);
        size_t target = cur_info->entry;
        // 回填过程入口地址
        pcodelist.backpatch(target, entry);
        
        // 标记过程已定义
        if (cur_proc)
            cur_info->isDefined = true;
        
        // 复合语句
        body();
    }
}

/**
 * @brief 程序处理
 * @details 文法: <prog> → program <id>;<block>
 */
void Parser::prog()
{
    int r = judge(PROGM_SYM, IDENT | SEMICOLON | firstBlock, MISSING, L"program");
    if (r == 1)
        lexer.GetWord();

    if (lexer.GetTokenType() == IDENT)
    {
        symTable.MkTable();
        symTable.EnterProgm(lexer.GetStrToken());
        lexer.GetWord();
        if (lexer.GetTokenType() == SEMICOLON)
        {
            lexer.GetWord();
            size_t entry = pcodelist.emit(jmp, 0, 0);
            symTable.table[0].info->SetEntry(entry);
            block();
            pcodelist.emit(opr, 0, OPR_RETURN);
            if (lexer.GetCh() != L'\0' && lexer.GetCh() != L'#')
                errorHandle.error(ILLEGAL_WORD, (L"'" + lexer.GetStrToken() + L"'").c_str(),
                                  lexer.GetPreWordRow(), lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            return;
        }
        else
        {
            size_t entry = pcodelist.emit(jmp, 0, 0);
            symTable.table[0].info->SetEntry(entry);
            errorHandle.error(MISSING, L";",
                              lexer.GetPreWordRow(), lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            block();
            pcodelist.emit(opr, 0, OPR_RETURN);
            if (lexer.GetCh() != L'\0' && lexer.GetCh() != L'#')
                errorHandle.error(ILLEGAL_WORD, (L"'" + lexer.GetStrToken() + L"'").c_str(),
                                  lexer.GetPreWordRow(), lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
            return;
        }
    }

    if (lexer.GetTokenType() == SEMICOLON)
    {
        errorHandle.error(MISSING, L"program name",
                          lexer.GetPreWordRow(), lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());

        symTable.MkTable();
        symTable.EnterProgm(L"null");
        lexer.GetWord();
        size_t entry = pcodelist.emit(jmp, 0, 0);
        symTable.table[0].info->SetEntry(entry);
        block();
        pcodelist.emit(opr, 0, OPR_RETURN);
        if (lexer.GetCh() != '\0' && lexer.GetCh() != L'#')
            errorHandle.error(ILLEGAL_WORD, (L"'" + lexer.GetStrToken() + L"'").c_str(),
                              lexer.GetPreWordRow(), lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
        return;
    }

    if (lexer.GetTokenType() & firstBlock)
    {
        symTable.MkTable();
        symTable.EnterProgm(L"null");
        errorHandle.error(EXPECT_STH_FIND_ANTH, L"id", (L"'" + lexer.GetStrToken() + L"'").c_str(),
                          lexer.GetPreWordRow(), lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
        size_t entry = pcodelist.emit(jmp, 0, 0);
        symTable.table[0].info->SetEntry(entry);
        block();
        pcodelist.emit(opr, 0, OPR_RETURN);
        if (lexer.GetCh() != '\0' && lexer.GetCh() != L'#')
            errorHandle.error(ILLEGAL_WORD, (L"'" + lexer.GetStrToken() + L"'").c_str(),
                              lexer.GetPreWordRow(), lexer.GetPreWordCol(), lexer.GetRowPos(), lexer.GetColPos());
        return;
    }

    return;
}

/**
 * @brief 启动语法分析
 * @details 入口函数，调用prog()开始分析并输出结果
 */
void Parser::analyze()
{
    lexer.GetWord();
    prog();
    errorHandle.over();
}
