/**
 * @file Interpreter.hpp
 * @brief P-Code解释器模块
 * @details 实现P-Code虚拟机，负责执行生成的中间代码
 */

#ifndef _INTER_HPP
#define _INTER_HPP

#include <PCode.hpp>
#include <Types.hpp>
using namespace std;

/* ====== 活动记录布局常量 ====== */
#define RETURN_ADDRESS 0      // 返回地址在活动记录中的偏移
#define OLD_SP 1              // 动态链(旧基址)在活动记录中的偏移
#define GLO_DISPLAY 2         // 全局display指针存放位置
#define DISPLAY 3             // 局部display起始位置

/**
 * @class Interpreter
 * @brief P-Code解释执行器
 * @details 模拟栈式虚拟机，逐条解释执行P-Code指令
 */
class Interpreter {
public:
    size_t pc;                      // 程序计数器(指向当前指令)
    size_t top;                     // 栈顶指针(下一个可用位置)
    size_t sp;                      // 基址寄存器(当前活动记录基址)
    vector<int> running_stack;      // 运行时数据栈

    void run();   // 启动解释执行
    
private:
    /* ====== 各指令的执行函数 ====== */
    void lit(Operation op, int L, int a);   // 加载常量
    void opr(Operation op, int L, int a);   // 算术/逻辑运算
    void lod(Operation op, int L, int a);   // 加载变量
    void sto(Operation op, int L, int a);   // 存储变量
    void cal(Operation op, int L, int a);   // 过程调用
    void alc(Operation op, int L, int a);   // 分配栈空间
    void jmp(Operation op, int L, int a);   // 无条件跳转
    void jpc(Operation op, int L, int a);   // 条件跳转
    void red(Operation op, int L, int a);   // 读取输入
    void wrt(Operation op, int L, int a);   // 输出结果

    void clear();   // 清空运行时状态
    void Init();    // 初始化解释器
};

extern Interpreter interpreter;

#endif
