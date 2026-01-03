/**
 * @file PCode.cpp
 * @brief P-Code指令管理器实现
 * @details 实现P-Code指令的生成、回填和显示功能
 */

#include <PCode.hpp>

// P-Code指令序列全局实例
PCodeList pcodelist;

// 指令助记符映射表
wstring op_map[P_CODE_CNT] = {
    L"LIT",   // 加载常量
    L"OPR",   // 运算操作
    L"LOD",   // 加载变量
    L"STO",   // 存储变量
    L"CAL",   // 调用过程
    L"INT",   // 分配空间
    L"JMP",   // 无条件跳转
    L"JPC",   // 条件跳转
    L"RED",   // 读取输入
    L"WRT"    // 输出结果
};

/**
 * @brief 生成一条P-Code指令
 * @param op 操作码
 * @param L 层差
 * @param a 地址/立即数
 * @return 生成指令的地址(索引)
 */
int PCodeList::emit(Operation op, int L, int a)
{
    code_list.push_back(PCode(op, L, a));
    return code_list.size() - 1;
}

/**
 * @brief 回填跳转目标地址
 * @param target 待回填指令的位置
 * @param addr 跳转目标地址
 */
void PCodeList::backpatch(size_t target, size_t addr)
{
    if (addr == -1 || target >= code_list.size())
        return;
        code_list[target].a = addr;
}

/**
 * @brief 显示所有P-Code指令
 */
void PCodeList::show()
{
    for (size_t i = 0; i < code_list.size(); i++) {
        wcout << setw(4) << i << L"  " 
              << op_map[code_list[i].op] << L", " 
              << code_list[i].L << L", " 
              << code_list[i].a << endl;
}
}
