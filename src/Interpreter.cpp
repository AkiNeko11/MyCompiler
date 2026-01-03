/**
 * @file Interpreter.cpp
 * @brief P-Code解释器实现
 * @details 实现栈式虚拟机，逐条解释执行P-Code指令
 */

#include <Interpreter.hpp>

// 解释器全局实例
Interpreter interpreter;

/**
 * @brief 初始化解释器
 * @details 重置程序计数器、栈顶指针和基址寄存器
 */
void Interpreter::Init()
{
    pc = 0;
    top = 0;
    sp = 0;
}

/**
 * @brief 执行LIT指令 - 加载常量
 * @param op 操作码
 * @param L 层差(未使用)
 * @param a 常量值
 * @details 将常量a压入数据栈栈顶
 */
void Interpreter::lit(Operation op, int L, int a)
{
    if (top == running_stack.size())
        running_stack.push_back(a);
    else
        running_stack[top] = a;
    top++;
    pc++;
}

/**
 * @brief 执行OPR指令 - 算术/逻辑运算
 * @param op 操作码
 * @param L 层差(未使用)
 * @param a 运算类型
 * @details 根据a的值执行不同的运算操作
 */
void Interpreter::opr(Operation op, int L, int a)
{
    // 过程返回
    if (a == OPR_RETURN) {
        pc = running_stack[sp + RETURN_ADDRESS];
        int old_sp = running_stack[sp + OLD_SP];
        top -= top - sp;
        sp = old_sp;
        return;
    }
    // 取负
    else if (a == OPR_NEGTIVE) {
        running_stack[top - 1] = ~running_stack[top - 1] + 1;
    }
    // 加法
    else if (a == OPR_ADD) {
        int res = running_stack[top - 2] + running_stack[top - 1];
        running_stack[top - 2] = res;
        top--;
    }
    // 减法
    else if (a == OPR_SUB) {
        int res = running_stack[top - 2] - running_stack[top - 1];
        running_stack[top - 2] = res;
        top--;
    }
    // 乘法
    else if (a == OPR_MULTI) {
        int res = running_stack[top - 2] * running_stack[top - 1];
        running_stack[top - 2] = res;
        top--;
    }
    // 除法
    else if (a == OPR_DIVIS) {
        int res = running_stack[top - 2] / running_stack[top - 1];
        running_stack[top - 2] = res;
        top--;
    }
    // 奇偶判断
    else if (a == OPR_ODD) {
        running_stack[top - 1] = (running_stack[top - 1] & 0b1) == 1;
    }
    // 等于比较
    else if (a == OPR_EQL) {
        bool res = running_stack[top - 2] == running_stack[top - 1];
        running_stack[top - 2] = res;
        top--;
    }
    // 不等于比较
    else if (a == OPR_NEQ) {
        bool res = running_stack[top - 2] != running_stack[top - 1];
        running_stack[top - 2] = res;
        top--;
    }
    // 小于比较
    else if (a == OPR_LSS) {
        bool res = running_stack[top - 2] < running_stack[top - 1];
        running_stack[top - 2] = res;
        top--;
    }
    // 小于等于比较
    else if (a == OPR_LEQ) {
        bool res = running_stack[top - 2] <= running_stack[top - 1];
        running_stack[top - 2] = res;
        top--;
    }
    // 大于比较
    else if (a == OPR_GRT) {
        bool res = running_stack[top - 2] > running_stack[top - 1];
        running_stack[top - 2] = res;
        top--;
    }
    // 大于等于比较
    else if (a == OPR_GEQ) {
        bool res = running_stack[top - 2] >= running_stack[top - 1];
        running_stack[top - 2] = res;
        top--;
    }
    pc++;
}

/**
 * @brief 执行LOD指令 - 加载变量
 * @param op 操作码
 * @param L 层差
 * @param a 相对偏移
 * @details 根据层差和偏移从指定位置取值压入栈顶
 */
void Interpreter::lod(Operation op, int L, int a)
{
    // 通过display表定位目标活动记录
    if (top == running_stack.size())
        running_stack.push_back(running_stack[running_stack[sp + DISPLAY + L] + a]);
    else
        running_stack[top] = running_stack[running_stack[sp + DISPLAY + L] + a];
    top++;
    pc++;
}

/**
 * @brief 执行STO指令 - 存储变量
 * @param op 操作码
 * @param L 层差
 * @param a 相对偏移
 * @details 将栈顶值存入指定位置并弹栈
 */
void Interpreter::sto(Operation op, int L, int a)
{
    if (L >= 0) {
        // 正常存储：通过display表定位目标位置
        running_stack[running_stack[sp + DISPLAY + L] + a] = running_stack[top - 1];
        top--;
    }
    else {
        // L为-1：形参传递，需预先开辟空间
        size_t cur_size = running_stack.size();
        int val = running_stack[top - 1];
        top--;
        
        // 开辟足够的空间
        for (int i = cur_size - top; i <= a; i++)
            running_stack.push_back(0);
        
        running_stack[top + a] = val;
    }
    pc++;
}

/**
 * @brief 执行CAL指令 - 调用过程
 * @param op 操作码
 * @param L 层差
 * @param a 入口地址
 * @details 保存现场，建立新的活动记录
 */
void Interpreter::cal(Operation op, int L, int a)
{
    // 保存返回地址
    running_stack[top + RETURN_ADDRESS] = pc + 1;
    
    // 复制display表的前L+1项
    for (int i = 0; i <= L; i++)
        running_stack[top + DISPLAY + i] = running_stack[running_stack[sp + GLO_DISPLAY] + i];
    
    // 新活动记录的display表添加自身基址
    running_stack[top + DISPLAY + L + 1] = top;
    
    // 保存旧基址并更新
    running_stack[top + OLD_SP] = sp;
    sp = top;
    
    // 跳转到过程入口
    pc = a;
}

/**
 * @brief 执行INT指令 - 分配栈空间
 * @param op 操作码
 * @param L 层差(未使用)
 * @param a 分配单元数
 * @details 在栈顶分配a个存储单元
 */
void Interpreter::alc(Operation op, int L, int a)
{
    size_t cur_size = running_stack.size();
    
    if (a <= cur_size - top) {
        top += a;
    }
    else {
        // 需要扩展栈空间
        for (int i = 0; i < a - (cur_size - top); i++)
            running_stack.push_back(0);
        top = running_stack.size();
    }
    
    // 设置全局display指针
    running_stack[sp + GLO_DISPLAY] = sp + DISPLAY;
    pc++;
}

/**
 * @brief 执行JMP指令 - 无条件跳转
 * @param op 操作码
 * @param L 层差(未使用)
 * @param a 目标地址
 */
void Interpreter::jmp(Operation op, int L, int a)
{
    pc = a;
}

/**
 * @brief 执行JPC指令 - 条件跳转
 * @param op 操作码
 * @param L 层差(未使用)
 * @param a 目标地址
 * @details 栈顶为false时跳转
 */
void Interpreter::jpc(Operation op, int L, int a)
{
    if (running_stack[top - 1] == false)
        pc = a;
    else
        pc++;
    top--;
}

/**
 * @brief 执行RED指令 - 读取输入
 * @param op 操作码
 * @param L 层差(未使用)
 * @param a 地址(未使用)
 * @details 从控制台读取一个整数并压入栈顶
 */
void Interpreter::red(Operation op, int L, int a)
{
    int data;
    wcout << "read: ";
    wcin >> data;
    
    if (top == running_stack.size())
        running_stack.push_back(data);
    else
        running_stack[top] = data;
    top++;
    pc++;
}

/**
 * @brief 执行WRT指令 - 输出结果
 * @param op 操作码
 * @param L 层差(未使用)
 * @param a 地址(未使用)
 * @details 输出栈顶值并弹栈
 */
void Interpreter::wrt(Operation op, int L, int a)
{
    wcout << "write: " << running_stack[top - 1] << endl;
    top--;
    pc++;
}

/**
 * @brief 启动解释执行
 * @details 初始化后逐条执行P-Code指令
 */
void Interpreter::run()
{
    Init();
    
    // 按pc指示逐条执行指令
    for (int i = 0; i < pcodelist.code_list.size() - 1; i = pc) {
        PCode code = pcodelist.code_list[i];
        
        switch (code.op) {
        case Operation::lit:
            lit(code.op, code.L, code.a);
            break;
        case Operation::opr:
            opr(code.op, code.L, code.a);
            break;
        case Operation::load:
            lod(code.op, code.L, code.a);
            break;
        case Operation::store:
            sto(code.op, code.L, code.a);
            break;
        case Operation::call:
            cal(code.op, code.L, code.a);
            break;
        case Operation::alloc:
            alc(code.op, code.L, code.a);
            break;
        case Operation::jmp:
            jmp(code.op, code.L, code.a);
            break;
        case Operation::jpc:
            jpc(code.op, code.L, code.a);
            break;
        case Operation::red:
            red(code.op, code.L, code.a);
            break;
        case Operation::wrt:
            wrt(code.op, code.L, code.a);
            break;
        default:
            break;
        }
    }
}

/**
 * @brief 清空解释器状态
 */
void Interpreter::clear()
{
    running_stack.clear();
    sp = 0;
    top = 0;
    pc = 0;
}
