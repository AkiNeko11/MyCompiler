# PL/0 编译器设计与实现

## 一、项目概述

本项目实现了一个完整的 **PL/0 语言编译器**，包含词法分析、语法分析、语义分析、中间代码生成以及解释执行等全部编译流程。

### 1.1 什么是 PL/0？

PL/0 是由 Niklaus Wirth（Pascal 语言之父）设计的一门教学用编程语言，是 Pascal 的简化版本。它具有以下特点：
- 支持常量、变量声明
- 支持过程（函数）定义和调用
- 支持基本的控制结构（if-else、while）
- 语法简单，非常适合学习编译原理

### 1.2 编译流程

```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│  源代码      │ -> │  词法分析    │ -> │  语法分析    │ -> │  语义分析    │
│  (UTF-8)    │    │  (Lexer)    │    │  (Parser)   │    │ (SymTable)  │
└─────────────┘    └─────────────┘    └─────────────┘    └─────────────┘
                                                               │
                  ┌─────────────┐    ┌─────────────┐           │
                  │  解释执行    │ <- │ P-Code生成  │ <---------┘
                  │(Interpreter)│    │ (PCodeList) │
                  └─────────────┘    └─────────────┘
```

### 1.3 项目特性

- **UTF-8 支持**：支持读取 UTF-8 编码的源文件（含 BOM 检测）
- **缓冲区读取**：采用按需加载的缓冲区机制，支持大文件处理
- **Clang 风格错误诊断**：
  - 🎨 彩色控制台输出（错误红色、警告黄色、提示绿色）
  - 📍 源码行显示与精准位置指示（`^^^`）
  - 💡 智能修复建议（hint 提示）
  - 📊 编译摘要统计
- **完善的错误恢复**：支持跳过错误继续编译，可报告多个错误
- **详细的调试信息**：输出编译过程的详细信息

---

## 二、PL/0 语言语法规范

### 2.1 文法定义（BNF 范式）

```bnf
<prog>      → program <id>；<block>
<block>     → [<condecl>][<vardecl>][<proc>]<body>
<condecl>   → const <const>{,<const>};
<const>     → <id> := <integer>
<vardecl>   → var <id>{,<id>};
<proc>      → procedure <id>（[<id>{,<id>}]）;<block>{;<proc>}
<body>      → begin <statement>{;<statement>} end
<statement> → <id> := <exp>
            | if <lexp> then <statement> [else <statement>]
            | while <lexp> do <statement>
            | call <id>（[<exp>{,<exp>}]）
            | <body>
            | read (<id>{,<id>})
            | write (<exp>{,<exp>})
<lexp>      → <exp> <lop> <exp> | odd <exp>
<exp>       → [+|-] <term> {<aop><term>}
<term>      → <factor> {<mop><factor>}
<factor>    → <id> | <integer> | (<exp>)
<lop>       → = | <> | < | <= | > | >=
<aop>       → + | -
<mop>       → * | /
<id>        → 字母{字母|数字}
<integer>   → 数字{数字}
```

### 2.2 保留字

| 保留字 | 含义 |
|--------|------|
| `program` | 程序开始 |
| `const` | 常量声明 |
| `var` | 变量声明 |
| `procedure` | 过程声明 |
| `begin` | 语句块开始 |
| `end` | 语句块结束 |
| `if` | 条件判断 |
| `then` | if的后续 |
| `else` | 否则分支 |
| `while` | 循环 |
| `do` | while的后续 |
| `call` | 过程调用 |
| `read` | 读取输入 |
| `write` | 输出 |
| `odd` | 奇数判断 |

### 2.3 示例程序

```pascal
program example;
const pi := 3;
var x, y, z;
begin
    x := 10;
    y := 20;
    z := x + y;
    write(z)
end
```

---

## 三、模块详解

### 3.1 类型定义模块 (Types.hpp/cpp)

#### 功能
- 定义全局常量和宏
- 实现带缓冲区的 UTF-8 文件读取器
- 提供字符串转换工具函数

#### Token 类型定义

使用位掩码设计，便于进行集合运算：

```cpp
// 使用示例：判断是否为加减运算符
if (tokenType & (PLUS | MINUS)) { ... }

// 关系运算符
#define EQL 0x1        // =
#define NEQ 0x2        // <>
#define LSS 0x4        // <

// 算术运算符
#define PLUS 0x40      // +
#define MINUS 0x80     // -

// 标识符与数字
#define IDENT 0x400    // 标识符
#define NUMBER 0x800   // 数值

// 保留字
#define BEGIN_SYM 0x40000  // begin
#define END_SYM 0x80000    // end
```

#### ReadUnicode 类（带缓冲区）

采用按需加载的缓冲区机制，不一次性读取整个文件：

```cpp
const size_t BUFFER_SIZE = 1024;  // 缓冲区大小

class ReadUnicode {
private:
    ifstream file;                 // 文件流
    wchar_t buffer[BUFFER_SIZE];   // 字符缓冲区
    size_t bufferStartPos;         // 缓冲区起始位置
    size_t bufferLength;           // 有效字符数
    
    bool loadNextBuffer();         // 加载下一块数据
    bool readOneChar(wchar_t& ch); // 读取单个UTF-8字符
    
public:
    void readFile2USC2(string filename);      // 打开文件
    wchar_t getProgmWStr(const size_t pos);   // 获取指定位置字符
    bool isEmpty();                           // 是否为空
    size_t getLoadedCount();                  // 已加载字符数
};
```

**工作原理：**
```
文件:  [=== 块1 ===][=== 块2 ===][=== 块3 ===]...
            ↑
        当前缓冲区

词法分析器请求位置 → 检查是否在缓冲区内
    ↓ 是 → 直接返回
    ↓ 否 → 自动加载下一块缓冲区
```

#### 工具函数

```cpp
// 宽字符串转整数（使用位运算优化）
int w_str2int(wstring numStr);

// 整数转宽字符串
wstring int2w_str(int num);
```

---

### 3.2 词法分析器 (Lexer.hpp/cpp)

#### 功能
将源代码字符流转换为 Token（词法单元）流。

#### 工作原理

```
源代码: "x := 10 + y"
         ↓ 词法分析
Token流: [IDENT:"x"] [ASSIGN:":="] [NUMBER:"10"] [PLUS:"+"] [IDENT:"y"]
```

#### 核心数据结构

```cpp
class Lexer {
private:
    wchar_t ch;              // 当前读取的字符
    unsigned long tokenType; // 当前 Token 类型
    wstring strToken;        // 当前 Token 的字符串值
    size_t nowPtr;           // 字符指针位置
    size_t colPos, rowPos;   // 列号、行号（用于报错定位）
    
public:
    void GetWord();                // 获取下一个 Token
    unsigned long GetTokenType();  // 获取 Token 类型
    wstring GetStrToken();         // 获取 Token 字符串
};
```

#### GetWord() 工作流程

```
开始
  ↓
跳过空白字符 (GetBC)
  ↓
读取一个字符 (GetChar)
  ↓
┌─────────────────────────────────────────────┐
│ 判断字符类型：                               │
│  - 字母开头 → 读取标识符/关键字              │
│  - 数字开头 → 读取数字                       │
│  - ':' → 可能是 ':=' 赋值符                 │
│  - '<' → 可能是 '<' 或 '<=' 或 '<>'         │
│  - '>' → 可能是 '>' 或 '>='                 │
│  - 运算符 → 直接返回对应类型                 │
└─────────────────────────────────────────────┘
  ↓
设置 tokenType 和 strToken
  ↓
结束
```

---

### 3.3 语法分析器 (Parser.hpp/cpp)

#### 功能
1. 检查源代码是否符合语法规则
2. 构建符号表（语义分析）
3. 生成 P-Code 中间代码

#### 分析方法：递归下降分析

每个语法规则对应一个函数：

| 语法规则 | 对应函数 |
|----------|----------|
| `<prog>` | `prog()` |
| `<block>` | `block()` |
| `<condecl>` | `condecl()` |
| `<vardecl>` | `vardecl()` |
| `<proc>` | `proc()` |
| `<body>` | `body()` |
| `<statement>` | `statement()` |
| `<lexp>` | `lexp()` |
| `<exp>` | `exp()` |
| `<term>` | `term()` |
| `<factor>` | `factor()` |

#### FIRST 集和 FOLLOW 集

用于预测分析和错误恢复：

```cpp
// FIRST集：某个语法成分可能的开始符号
unsigned long firstStatement = IDENT | IF_SYM | WHILE_SYM | CALL_SYM | BEGIN_SYM | READ_SYM | WRITE_SYM;
unsigned long firstExp = IDENT | NUMBER | LPAREN | PLUS | MINUS;

// FOLLOW集：某个语法成分后面可能跟的符号
unsigned long followStatement = SEMICOLON | END_SYM | ELSE_SYM;
```

#### 递归下降示例

以 `<exp> → [+|-]<term>{<aop><term>}` 为例：

```cpp
void Parser::exp() {
    // [+|-] 可选的正负号
    if (lexer.GetTokenType() & (PLUS | MINUS)) {
        aop = lexer.GetTokenType();
        lexer.GetWord();
    }
    
    // <term> 第一个项
    term();
    
    // {<aop><term>} 循环处理后续的 +/- 项
    while (lexer.GetTokenType() & (PLUS | MINUS)) {
        aop = lexer.GetTokenType();
        lexer.GetWord();
        term();
        // 生成加减运算的 P-Code
        if (aop == MINUS)
            pcodelist.emit(opr, 0, OPR_SUB);
        else
            pcodelist.emit(opr, 0, OPR_ADD);
    }
}
```

#### 错误恢复机制

`judge()` 函数实现错误恢复：

```cpp
// s1: 期望的符号集
// s2: 可以同步的符号集（FOLLOW集）
// 返回: 1=匹配s1, -1=匹配s2, 2=文件结束
int judge(s1, s2, errorType, errorMsg) {
    if (当前符号 不在 s1 中) {
        报告错误;
        while (当前符号 不在 s1∪s2 中) {
            继续读取下一个符号;  // 跳过错误部分
        }
    }
    return 匹配结果;
}
```

---

### 3.4 符号表 (SymTable.hpp/cpp)

#### 功能
管理程序中的标识符信息（变量、常量、过程）。

#### 数据结构

```cpp
// 符号类型
enum Category {
    NIL,   // 空
    VAR,   // 变量
    PROCE, // 过程
    CST,   // 常量
    FORM,  // 形参
    PROG   // 主程序
};

// 符号信息基类
class Information {
    Category cat;   // 符号类型
    size_t level;   // 所在层级（嵌套深度）
    size_t offset;  // 相对地址偏移
    size_t entry;   // 入口地址（过程用）
};

// 变量信息
class VarInfo : public Information {
    int value;  // 变量值（常量用）
};

// 过程信息
class ProcInfo : public Information {
    bool isDefined;              // 是否已定义
    vector<size_t> formVarList;  // 形参列表
};

// 符号表项
class SymTableItem {
    Information* info;  // 符号信息
    wstring name;       // 符号名称
    size_t previous;    // 链接到同层前一个符号
};

// 符号表
class SymTable {
    vector<SymTableItem> table;  // 符号表存储
    vector<size_t> display;      // Display 表（层次索引）
    size_t level;                // 当前层级
    size_t sp;                   // 当前过程起始位置
};
```

#### Display 表机制

Display 表用于快速定位各层的符号：

```
假设程序结构：
program main;           // level 0
    procedure A();      // level 1
        procedure B();  // level 2
        ...

Display表：
display[0] → 指向 level 0 最新符号的位置
display[1] → 指向 level 1 最新符号的位置
display[2] → 指向 level 2 最新符号的位置
```

#### 符号查找流程

```cpp
int SearchInfo(wstring name, Category cat) {
    // 从当前层向外层逐层查找
    for (int i = level; i >= 0; i--) {
        curAddr = display[i];  // 该层最新符号
        while (curAddr 有效) {
            if (找到匹配的符号)
                return curAddr;
            curAddr = table[curAddr].previous;  // 沿链表向前
        }
    }
    return -1;  // 未找到
}
```

---

### 3.5 P-Code 生成 (PCode.hpp/cpp)

#### 什么是 P-Code？

P-Code（Pseudo Code）是一种栈式虚拟机指令，介于高级语言和机器码之间。

#### 指令集

| 指令 | 格式 | 功能 |
|------|------|------|
| LIT | LIT 0, a | 将常量 a 压入栈顶 |
| OPR | OPR 0, a | 执行运算，a 指定运算类型 |
| LOD | LOD L, a | 取层差为 L、偏移为 a 的变量，压入栈顶 |
| STO | STO L, a | 将栈顶值存入层差为 L、偏移为 a 的变量 |
| CAL | CAL L, a | 调用层差为 L 的过程，入口地址为 a |
| INT | INT 0, a | 在栈顶分配 a 个空间 |
| JMP | JMP 0, a | 无条件跳转到地址 a |
| JPC | JPC 0, a | 栈顶为假时跳转到地址 a |
| RED | RED 0, 0 | 从输入读取一个值压入栈顶 |
| WRT | WRT 0, 0 | 输出栈顶值 |

#### OPR 运算类型

| a 值 | 运算 |
|------|------|
| 0 | 返回（RET） |
| 1 | 取负 |
| 2 | 加法 |
| 3 | 减法 |
| 4 | 乘法 |
| 5 | 除法 |
| 6 | odd 判断 |
| 7 | 等于 |
| 8 | 不等于 |
| 9 | 小于 |
| 10 | 大于等于 |
| 11 | 大于 |
| 12 | 小于等于 |

#### 代码生成示例

源代码：`x := a + b * 2`

生成的 P-Code：
```
LOD 0, 3    // 加载变量 a
LOD 0, 4    // 加载变量 b
LIT 0, 2    // 常量 2
OPR 0, 4    // 乘法 (b * 2)
OPR 0, 2    // 加法 (a + b*2)
STO 0, 5    // 存入变量 x
```

#### 回填技术

对于跳转指令，目标地址在生成时可能未知，需要回填：

```cpp
// if 语句的代码生成
lexp();                              // 生成条件表达式代码
int jpc_addr = emit(jpc, 0, 0);      // 生成 JPC，地址待填
statement();                          // 生成 then 分支代码
backpatch(jpc_addr, 当前地址);        // 回填跳转地址
```

---

### 3.6 解释器 (Interpreter.hpp/cpp)

#### 功能
执行生成的 P-Code 指令。

#### 运行时栈结构

```
栈内存布局：
┌────────────────────────────────────┐ ← top (栈顶)
│           临时变量/计算值            │
├────────────────────────────────────┤
│           局部变量                  │
├────────────────────────────────────┤
│         Display 表副本              │
├────────────────────────────────────┤
│      全局 Display 地址 (GLO_DIS)    │
├────────────────────────────────────┤
│         老 SP (DL)                 │
├────────────────────────────────────┤
│        返回地址 (RA)                │ ← sp (当前活动记录基址)
└────────────────────────────────────┘
```

#### 活动记录

每次过程调用，都会创建一个活动记录：

```
偏移  内容
0     返回地址 (RA)
1     老 SP，动态链 (DL)
2     全局 Display 表地址
3+    Display 表副本 (level+1 个单元)
...   形参和局部变量
```

#### 过程调用过程 (CAL 指令)

```cpp
void Interpreter::cal(Operation op, int L, int a) {
    // 1. 保存返回地址
    running_stack[top + 0] = pc + 1;
    
    // 2. 保存老 SP
    running_stack[top + 1] = sp;
    
    // 3. 复制 Display 表
    for (int i = 0; i <= L; i++)
        running_stack[top + 3 + i] = running_stack[sp + 3 + i];
    
    // 4. 设置新层的 Display 项
    running_stack[top + 3 + L + 1] = top;
    
    // 5. 更新 SP 和 PC
    sp = top;
    pc = a;  // 跳转到过程入口
}
```

#### 过程返回过程 (OPR 0, 0)

```cpp
void Interpreter::opr_return() {
    pc = running_stack[sp + 0];   // 恢复返回地址
    int old_sp = running_stack[sp + 1];  // 获取老 SP
    top = sp;                      // 弹出当前活动记录
    sp = old_sp;                   // 恢复 SP
}
```

---

### 3.7 错误处理 (ErrorHandle.hpp/cpp)

#### 功能特性

本编译器实现了**类似 Clang 风格的专业错误诊断系统**，具有以下特性：

- **彩色控制台输出**：使用不同颜色区分错误级别和代码元素
- **源码高亮显示**：直接在源代码行中用红色标记错误位置
- **精准位置指示**：使用 `^` 符号精确指向错误所在位置
- **智能修复建议**：根据错误类型自动生成 `hint` 提示信息
- **编译摘要统计**：编译结束后显示错误和警告的统计信息

#### 错误输出格式

```
文件名:行号:列号: error: 错误信息
   行号 | 源代码内容（错误部分红色高亮）
      | ^^^^ （绿色位置指示器）
      | hint: 修复建议（绿色）
```

#### 错误输出示例

```
test.txt:7:12: error: missing ;
   7 |     z := x + y
     |              ^
     | hint: Expected ';' here

test.txt:5:5: error: use of undeclared identifier 'abc'
   5 |     abc := 10
     |     ^^^
     | hint: Declare 'abc' first

─────────────────────────────────────────────────────────
✗ 2 error(s) generated.
─────────────────────────────────────────────────────────
Compilation failed.
```

#### 颜色定义

| 颜色 | 用途 |
|------|------|
| 红色 (COLOR_RED) | 错误标签、错误位置高亮 |
| 黄色 (COLOR_YELLOW) | 警告标签 |
| 绿色 (COLOR_GREEN) | 位置指示器 `^`、修复建议、成功信息 |
| 青色 (COLOR_CYAN) | 行号、提示标签 |
| 白色 (COLOR_WHITE) | 文件位置信息、错误消息正文 |

#### 错误级别

| 级别 | 含义 |
|------|------|
| LEVEL_NOTE | 注释/提示信息 |
| LEVEL_WARNING | 警告（不阻止编译） |
| LEVEL_ERROR | 错误（编译失败） |
| LEVEL_FATAL | 致命错误（立即终止） |

#### 错误类型与修复建议

| 错误码 | 含义 | 自动生成的修复建议 |
|--------|------|-------------------|
| MISSING | 缺少某符号 | `Add 'X' here` |
| EXPECT | 期望某符号 | `Expected 'X' here` |
| EXPECT_STH_FIND_ANTH | 期望A但发现B | `Did you mean 'A' instead of 'B'?` |
| ILLEGAL_WORD | 非法单词 | `Please check the 'X'` |
| ILLEGAL_DEFINE | 非法定义 | `Please check the 'X'` |
| UNDECLARED_IDENT | 未声明的标识符 | `Declare 'X' first` |
| UNDECLARED_PROC | 未声明的过程 | `Declare 'X' first` |
| UNDEFINED_PROC | 过程未定义 | `Define 'X' first` |
| REDUNDENT | 多余的符号 | `Remove 'X' here` |
| SYNTAX_ERROR | 语法错误 | `Please check the syntax: 'X'` |
| ILLEGAL_RVALUE_ASSIGN | 对右值赋值（给常量赋值） | `Constants cannot be modified. Use 'var' instead of 'const' if you need to change this value` |
| INCOMPATIBLE_VAR_LIST | 参数数量不匹配 | `Check the number of arguments. The procedure expects a different number of parameters` |
| REDECLEARED_IDENT | 重定义参数 | `Did not redeclare the identifier 'X'` |
| REDECLEARED_PROC | 重定义过程 | `Did not redeclare the procedure name 'X'` |

#### 核心实现

```cpp
// 打印源码片段和位置指示器
void printSourceSnippet(size_t row, size_t col, size_t highlightLen) {
    wstring sourceLine = getSourceLine(row);
    
    // 打印行号
    setColor(COLOR_CYAN);
    wcout << L"   " << row << L" | ";
    resetColor();
    
    // 打印源代码行，高亮错误位置（红色标记）
    for (size_t i = 0; i < sourceLine.length(); ++i) {
        if (i + 1 >= col && i + 1 < col + highlightLen) {
            setColor(COLOR_RED);  // 错误位置红色高亮
            wcout << sourceLine[i];
            resetColor();
        } else {
            wcout << sourceLine[i];
        }
    }
    
    // 打印位置指示器 (^^^^)
    setColor(COLOR_GREEN);
    wcout << generatePointer(col, highlightLen) << endl;
}

// 根据错误类型生成修复建议
if (n == MISSING) {
    swprintf_s(suggestionBuf, 256, L"Expected '%s' here", extra);
} else if (n == UNDECLARED_IDENT || n == UNDECLARED_PROC) {
    swprintf_s(suggestionBuf, 256, L"Declare '%s' first", extra);
} else if (n == REDUNDENT) {
    swprintf_s(suggestionBuf, 256, L"Remove '%s' here", extra);
}
// ... 其他错误类型
```

#### 编译摘要

编译结束后自动输出统计摘要：

```cpp
void printSummary() {
    if (errCnt == 0 && warnCnt == 0) {
        wcout << L"✓ Build succeeded with no errors or warnings." << endl;
    } else {
        if (errCnt > 0)
            wcout << L"✗ " << errCnt << L" error(s)";
        if (warnCnt > 0)
            wcout << L"⚠ " << warnCnt << L" warning(s)";
        wcout << L" generated." << endl;
    }
}
```

---

## 四、完整编译示例

### 输入程序

```pascal
program test;
var a, b;
begin
    a := 3;
    b := a + 2;
    write(b)
end
```

### 编译输出

```
[Info] Opening file: test/simple.txt
[Info] Compiling 'test/simple.txt' ...
[Info] End of file reached, total 56 characters loaded
No error. Congratulations!
______________________________Compile compelete!________________________________
```

### 符号表

| 位置 | 名称 | 类型 | 层级 | 偏移 |
|------|------|------|------|------|
| 0 | test | PROCE | 0 | - |
| 1 | a | VAR | 0 | 0 |
| 2 | b | VAR | 0 | 4 |

### 生成的 P-Code

```
0   JMP  0, 1     # 跳转到主程序入口
1   INT  0, 6     # 分配栈空间 (3+1+2变量)
2   LIT  0, 3     # 常量 3
3   STO  0, 4     # 存入 a
4   LOD  0, 4     # 加载 a
5   LIT  0, 2     # 常量 2
6   OPR  0, 2     # 加法
7   STO  0, 5     # 存入 b
8   LOD  0, 5     # 加载 b
9   WRT  0, 0     # 输出
10  OPR  0, 13    # 换行
11  OPR  0, 0     # 返回
```

### 运行结果

```
write: 5
```

---

## 五、项目文件结构

```
MyCompiler/
├── Include/                 # 头文件目录
│   ├── Types.hpp           # 类型定义、宏常量、UTF-8读取器
│   ├── Lexer.hpp           # 词法分析器声明
│   ├── Parser.hpp          # 语法分析器声明
│   ├── SymTable.hpp        # 符号表声明
│   ├── PCode.hpp           # P-Code 定义
│   ├── Interpreter.hpp     # 解释器声明
│   └── ErrorHandle.hpp     # 错误处理声明
├── src/                     # 源文件目录
│   ├── main.cpp            # 主程序入口
│   ├── Types.cpp           # UTF-8 读取器实现
│   ├── Lexer.cpp           # 词法分析器实现
│   ├── Parser.cpp          # 语法分析器实现
│   ├── SymTable.cpp        # 符号表实现
│   ├── PCode.cpp           # P-Code 生成实现
│   ├── Interpreter.cpp     # 解释器实现
│   └── ErrorHandle.cpp     # 错误处理实现
├── test/                    # 测试文件目录
└── README.md               # 本文档
```

---

## 六、编译与运行

### 编译命令（使用 g++）

```bash
g++ -I Include src/*.cpp -o compiler.exe
```

### 运行

```bash
./compiler.exe
```

### 菜单界面

```
========== PL/0 编译器 ==========
1. 词法分析测试
2. 语法分析测试
3. 符号表测试
4. P-Code生成测试
5. 完整编译运行
0. 退出
==================================
请选择功能:
```

---

## 七、参考资料

1. Wirth, N. (1976). *Algorithms + Data Structures = Programs*
2. Aho, A. V., Lam, M. S., Sethi, R., & Ullman, J. D. (2006). *Compilers: Principles, Techniques, and Tools* (2nd ed.)
3. 陈火旺 等. (2000). *程序设计语言编译原理* (第3版). 国防工业出版社

