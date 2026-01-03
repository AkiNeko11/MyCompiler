/**
 * @file PCode.hpp
 * @brief P-Code中间代码模块
 * @details 定义P-Code指令集及其管理类
 */

#ifndef _P_CODE_HPP
#define _P_CODE_HPP

#include <Types.hpp>

/**
 * @enum Operation
 * @brief P-Code操作码枚举
 * @details 定义虚拟机支持的所有指令类型
 */
enum Operation {
    lit,    // LIT: 将常量a压入栈顶
    opr,    // OPR: 执行运算操作，a指定运算类型
    load,   // LOD: 取变量值(层差L，偏移a)压入栈顶
    store,  // STO: 栈顶值存入变量(层差L，偏移a)
    call,   // CAL: 调用过程(层差L，入口地址a)
    alloc,  // INT: 在栈顶分配a个存储单元
    jmp,    // JMP: 无条件跳转到地址a
    jpc,    // JPC: 条件跳转，栈顶为0则跳转到地址a
    red,    // RED: 读取输入存入变量(层差L，偏移a)
    wrt,    // WRT: 输出栈顶值
};

/**
 * @class PCode
 * @brief 单条P-Code指令
 * @details 包含操作码、层差和地址三个字段
 */
class PCode {
public:
    Operation op;   // 操作码
    int L;          // 层差(level difference)
    int a;          // 地址或立即数

    PCode(Operation op1, int L1, int a1) : op(op1), L(L1), a(a1) {};
};

/**
 * @class PCodeList
 * @brief P-Code指令序列管理器
 * @details 管理生成的P-Code指令，支持添加、回填和显示操作
 */
class PCodeList {
public:
    vector<PCode> code_list;    // 指令序列

    int emit(Operation op, int L, int a);           // 生成一条指令，返回指令地址
    void backpatch(size_t target, size_t addr);     // 回填跳转地址
    void show();                                     // 显示所有指令
    void clear() { code_list.clear(); };            // 清空指令序列
};

extern PCodeList pcodelist;

#endif
