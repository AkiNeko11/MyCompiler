/**
 * @file SymTable.hpp
 * @brief 符号表管理模块
 * @details 实现符号表的构建与查询，支持分程序结构
 */

#ifndef _SYMBOL_TABLE_HPP
#define _SYMBOL_TABLE_HPP

#include <Types.hpp>
#include <Lexer.hpp>
using namespace std;

/**
 * @enum Category
 * @brief 符号类别枚举
 * @details 标识符号表中各项的类别属性
 */
enum Category
{
    NIL,    // 空(未定义)
    VAR,    // 变量
    PROCE,  // 过程
    CST,    // 常量
    FORM,   // 形式参数
    PROG,   // 程序(主程序入口)
};

/**
 * @class Information
 * @brief 符号信息基类
 * @details 存储符号的通用属性，派生类扩展特定类型的属性
 */
class Information
{
public:
    Category cat;     // 符号类别
    size_t level;     // 所在层次
    size_t offset;    // 相对偏移地址
    size_t entry;     // 入口地址(过程使用)

    Information() : cat(Category::NIL), level(0), offset(0), entry(-1) {};

    virtual void SetValue(wstring value) {}
    virtual int GetValue() { return -1; }
    virtual void show();
    virtual void SetEntry(size_t entry) {};
    virtual size_t GetEntry() { return -1; };
};

/**
 * @class VarInfo
 * @brief 变量信息类
 * @details 存储变量特有的属性，如变量值
 */
class VarInfo : public Information
{
private:
    int value;    // 变量当前值

public:
    VarInfo() : Information(), value(0) {};

    void SetValue(wstring val) override;
    void SetValue(int nowValue) { value = nowValue; };
    int GetValue() override;
    void show() override;
};

/**
 * @class ProcInfo
 * @brief 过程信息类
 * @details 存储过程特有的属性，如定义状态和形参列表
 */
class ProcInfo : public Information
{
public:
    bool isDefined;                 // 是否已定义
    vector<size_t> formVarList;     // 形参在符号表中的位置列表

    ProcInfo();

    void show() override;
    void SetEntry(size_t entry) override;
    size_t GetEntry() override;
};

/**
 * @class SymTableItem
 * @brief 符号表项
 * @details 符号表中的单个条目，包含名称、信息和链接指针
 */
class SymTableItem
{
public:
    Information* info;    // 符号信息(多态指向具体类型)
    wstring name;         // 符号名称
    size_t previous;      // 同一作用域内前一符号的位置(链式结构)
    
    void show();
};

/**
 * @class SymTable
 * @brief 符号表管理器
 * @details 管理整个程序的符号表，支持嵌套作用域
 */
class SymTable
{
public:
    size_t sp;                      // 当前作用域链表尾指针
    vector<SymTableItem> table;     // 符号表主体
    vector<size_t> display;         // 层次显示表(每层作用域的链尾位置)
    size_t level;                   // 当前嵌套层次

public:
    SymTable() : sp(0), level(0) { display.resize(1, 0); };

    SymTableItem GetTable(int num);                               // 获取指定位置的符号表项
    void PopDisplay() { display.pop_back(); }                     // 退出当前作用域

    void EnterProgm(wstring name);                                // 进入主程序
    void showAll();                                               // 显示整个符号表
    int InsertToTable(wstring name, size_t offset, Category cat); // 插入新符号
    int SearchInfo(wstring name, Category cat);                   // 查找符号
    void MkTable();                                               // 创建新作用域
    void InitAndClear();                                          // 初始化并清空符号表
    void AddWidth(size_t addr, size_t width);                     // 更新过程的栈帧大小
};

extern SymTable symTable;

#endif
