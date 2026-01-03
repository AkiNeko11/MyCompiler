/**
 * @file SymTable.cpp
 * @brief 符号表管理器实现
 * @details 实现符号表的创建、查找、插入和显示等操作，
 *          支持嵌套作用域的处理
 */

#include <SymTable.hpp>

// 符号表全局实例
SymTable symTable;

/**
 * @brief 显示符号信息(基类)
 */
void Information::show()
{
    wcout << setw(10) << L"cat: " << setw(5) << (int)cat
          << setw(10) << L"offset: " << setw(5) << offset
          << setw(10) << L"level: " << setw(5) << level;
}

/**
 * @brief 设置变量值
 * @param val 宽字符串形式的值
 */
void VarInfo::SetValue(wstring val) 
{ 
    this->value = w_str2int(val); 
}

/**
 * @brief 获取变量值
 * @return 变量当前值
 */
int VarInfo::GetValue() 
{ 
    return this->value; 
}

/**
 * @brief 显示变量信息
 */
void VarInfo::show()
{
    wcout << setw(10) << L"cat:" << setw(5) << (int)cat
          << setw(10) << L"offset:" << setw(5) << offset
          << setw(10) << L"level:" << setw(5) << level
          << setw(10) << L"value:" << setw(5) << value;
}

/**
 * @brief 获取指定位置的符号表项
 * @param num 符号表索引
 * @return 符号表项
 */
SymTableItem SymTable::GetTable(int num)
{
    return table.at(num);
}

/**
 * @brief 过程信息构造函数
 */
ProcInfo::ProcInfo() : Information()
{
    this->entry = -1;
    this->isDefined = false;
}

/**
 * @brief 显示过程信息
 */
void ProcInfo::show()
{
    wcout << setw(10) << L"cat:" << setw(5) << (int)cat
          << setw(10) << L"size:" << setw(5) << offset
          << setw(10) << L"level:" << setw(5) << level
          << setw(10) << L"entry:" << setw(5) << entry
          << setw(17) << L"form var list:";
    
    if (formVarList.empty())
        wcout << setw(5) << L"null";
    for (size_t mem : formVarList)
        wcout << setw(5) << symTable.GetTable(mem).name;
}

/**
 * @brief 设置过程入口地址
 * @param entry 入口地址
 */
void ProcInfo::SetEntry(size_t entry) 
{ 
    this->entry = entry; 
}

/**
 * @brief 获取过程入口地址
 * @return 入口地址
 */
size_t ProcInfo::GetEntry() 
{ 
    return this->entry; 
}

/**
 * @brief 在符号表中查找符号
 * @param name 符号名称
 * @param cat 符号类别
 * @return 符号在表中的位置，-1表示未找到
 * @details 从当前层向外层逐层查找，返回最近的匹配项
 */
int SymTable::SearchInfo(wstring name, Category cat)
{
    unsigned int curAddr = 0;
    
    // 第0层为空时直接返回
    if (level == 0 && display[0] == 0)
        return -1;
    
    // 从当前层向外层遍历
    for (int i = level; i >= 0; i--) {
        curAddr = display[i];
        while (1) {
            if (cat == Category::PROCE) {
                // 查找过程名
                if (table[curAddr].info->cat == Category::PROCE && table[curAddr].name == name)
                    return curAddr;
            }
            else {
                // 查找非过程符号
                if (table[curAddr].info->cat != Category::PROCE && table[curAddr].name == name)
                    return curAddr;
            }
            
            // previous为0表示已到本层链表头
            if (table[curAddr].previous == 0)
                break;
            curAddr = table[curAddr].previous;
        }
    }
    return -1;
}

/**
 * @brief 创建新的作用域
 * @details 更新sp指向新作用域的起始位置
 */
void SymTable::MkTable()
{
    sp = table.size();
}

/**
 * @brief 显示符号表项信息
 */
void SymTableItem::show()
{
    wcout << setw(5) << name << setw(10) << "previous:" << setw(4) << previous;
    info->show();
    wcout << setw(10) << "display:";
    for (int i = 0; i <= info->level; i++) {
        wcout << setw(5) << symTable.display[i];
    }
    wcout << endl;
}

/**
 * @brief 向符号表插入新符号
 * @param name 符号名称
 * @param offset 相对偏移
 * @param cat 符号类别
 * @return 插入位置，-1表示插入失败(重复定义)
 */
int SymTable::InsertToTable(wstring name, size_t offset, Category cat)
{
    int pos = SearchInfo(name, cat);
    
    // 检查过程名重复定义
    if (cat == Category::PROCE && pos != -1 && table[pos].info->level == level) {
        errorHandle.error(REDECLEARED_PROC, name.c_str(), 
                          lexer.GetPreWordRow(), lexer.GetPreWordCol(), 
                          lexer.GetRowPos(), lexer.GetColPos());
        return -1;
    }
    // 检查其他符号重复定义
    else if (pos != -1 && table[pos].info->level == level && cat != Category::PROCE) {
        errorHandle.error(REDECLEARED_IDENT, name.c_str(), 
                          lexer.GetPreWordRow(), lexer.GetPreWordCol(), 
                          lexer.GetRowPos(), lexer.GetColPos());
        return -1;
    }

    size_t curAddr = table.size();
    SymTableItem item;
    item.name = name;
    item.previous = display[level];
    display[level] = curAddr;

    // 根据类别创建不同的信息对象
    if (cat == Category::PROCE) {
        ProcInfo *procInfo = new ProcInfo;
        procInfo->cat = cat;
        procInfo->entry = 0;
        procInfo->offset = 0;
        procInfo->level = level;
        item.info = procInfo;
    }
    else {
        VarInfo *varInfo = new VarInfo;
        varInfo->offset = offset;
        varInfo->cat = cat;
        varInfo->level = level;
        varInfo->SetValue(0);
        item.info = varInfo;
    }
    
    table.push_back(item);
    return curAddr;
}

/**
 * @brief 进入主程序
 * @param name 程序名称
 */
void SymTable::EnterProgm(wstring name)
{
    SymTableItem item;
    item.previous = 0;
    item.name = name;
    
    ProcInfo* procInfo = new ProcInfo;
    procInfo->offset = 0;
    procInfo->cat = Category::PROCE;
    procInfo->level = 0;
    item.info = procInfo;
    
    table.push_back(item);
}

/**
 * @brief 显示整个符号表
 */
void SymTable::showAll()
{
    wcout << L"____________________________________________________SymTable_______________________________________________" << endl;
    for (SymTableItem mem : SymTable::table) {
        mem.show();
    }
    wcout << L"___________________________________________________________________________________________________________" << endl;
}

/**
 * @brief 更新过程的栈帧大小
 * @param addr 过程在符号表中的位置
 * @param width 栈帧大小
 */
void SymTable::AddWidth(size_t addr, size_t width)
{
    table[addr].info->offset = width;
    glo_offset = 0;
}

/**
 * @brief 初始化并清空符号表
 */
void SymTable::InitAndClear()
{
    sp = 0;
    table.clear();
    display.clear();
    table.reserve(100);
    display.resize(1, 0);
}
