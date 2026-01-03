/**
 * @file main.cpp
 * @brief PL/0编译器主程序入口
 * @details 提供编译器的交互式菜单，支持词法分析、语法分析、
 *          符号表查看、P-Code生成和完整编译运行等功能
 */

#include <Types.hpp>
#include <Lexer.hpp>
#include <ErrorHandle.hpp>
#include <SymTable.hpp>
#include <Parser.hpp>
#include <Interpreter.hpp>
using namespace std;

// 测试文件目录
const string TEST_DIR = "test/";

/**
 * @brief 初始化编译器各模块
 * @details 重置词法分析器、错误处理器、符号表和P-Code列表
 */
void init()
{
    readUnicode.InitReadUnicode();
    lexer.InitLexer();
    errorHandle.InitErrorHandle();
    // 设置控制台为Unicode输出模式
    _setmode(_fileno(stdout), _O_U16TEXT);
    symTable.InitAndClear();
    pcodelist.clear();
}

/**
 * @brief 获取测试文件的完整路径
 * @param filename 文件名
 * @return 完整的文件路径
 */
string getFilePath(const string& filename)
{
    return TEST_DIR + filename;
}

/**
 * @brief 词法分析测试
 * @details 读取源文件并执行词法分析，输出识别的词法单元
 */
void TestLexer()
{
    string filename = "";
    wcout << L"=== 词法分析测试 ===" << endl;
    wcout << L"请输入测试文件名(如 simple.txt): ";
    
    while (cin >> filename)
    {
        init();
        readUnicode.readFile2USC2(getFilePath(filename));
        if (readUnicode.isEmpty())
        {
            wcout << L"文件打开失败，请重新输入文件名: ";
            continue;
        }

        lexer.GetWord();
        while (lexer.GetCh() != L'\0')
        {
            lexer.GetWord();
        }
        wcout << L"词法分析完成!" << endl;
        return;
    }
}

/**
 * @brief 语法分析测试
 * @details 读取源文件并执行语法分析，检查语法错误
 */
void TestParser()
{
    string filename = "";
    wcout << L"=== 语法分析测试 ===" << endl;
    wcout << L"请输入测试文件名(如 simple.txt): ";
    
    while (cin >> filename)
    {
        init();
        readUnicode.readFile2USC2(getFilePath(filename));
        if (readUnicode.isEmpty())
        {
            wcout << L"文件打开失败，请重新输入文件名: ";
            continue;
        }

        parser.analyze();
        break;
    }
}

/**
 * @brief 符号表测试
 * @details 读取源文件，执行语法分析后输出符号表内容
 */
void TestSymTable()
{
    string filename = "";
    wcout << L"=== 符号表测试 ===" << endl;
    wcout << L"请输入测试文件名(如 simple.txt): ";
    
    while (cin >> filename)
    {
        init();
        readUnicode.readFile2USC2(getFilePath(filename));
        if (readUnicode.isEmpty())
        {
            wcout << L"文件打开失败，请重新输入文件名: ";
            continue;
        }

        parser.analyze();
        symTable.showAll();
        return;
    }
}

/**
 * @brief P-Code生成测试
 * @details 读取源文件，执行编译并输出生成的P-Code指令
 */
void TestPCode()
{
    string filename = "";
    wcout << L"=== P-Code生成测试 ===" << endl;
    wcout << L"请输入测试文件名(如 simple.txt): ";
    
    while (cin >> filename)
    {
        init();
        readUnicode.readFile2USC2(getFilePath(filename));
        if (readUnicode.isEmpty())
        {
            wcout << L"文件打开失败，请重新输入文件名: ";
            continue;
        }

        parser.analyze();
        wcout << L"\n=== 生成的P-Code ===" << endl;
        pcodelist.show();
        return;
    }
}

/**
 * @brief 完整编译运行测试
 * @details 读取源文件，执行完整编译流程，若无错误则运行程序
 */
void Test()
{
    string filename = "";
    wcout << L"=== 完整编译测试 ===" << endl;
    wcout << L"请输入测试文件名(如 simple.txt): ";
    
    while (cin >> filename)
    {
        init();
        readUnicode.readFile2USC2(getFilePath(filename));
        if (readUnicode.isEmpty())
        {
            wcout << L"文件打开失败，请重新输入文件名: ";
            continue;
        }

        parser.analyze();
        wcout << L"\n=== 生成的P-Code ===" << endl;
        pcodelist.show();
        
        // 只有在没有错误时才执行程序
        if (errorHandle.GetError() == 0)
        {
            wcout << L"\n=== 程序运行结果 ===" << endl;
            interpreter.run();
        }
        return;
    }
}

/**
 * @brief 显示主菜单
 */
void showMenu()
{
    wcout << L"\n========== PL/0 编译器 ==========" << endl;
    wcout << L"1. 词法分析测试" << endl;
    wcout << L"2. 语法分析测试" << endl;
    wcout << L"3. 符号表测试" << endl;
    wcout << L"4. P-Code生成测试" << endl;
    wcout << L"5. 完整编译运行" << endl;
    wcout << L"0. 退出" << endl;
    wcout << L"==================================" << endl;
    wcout << L"请选择功能: ";
}

/**
 * @brief 程序主入口
 * @return 程序退出码
 */
int main()
{
    // 设置控制台为Unicode输出模式
    _setmode(_fileno(stdout), _O_U16TEXT);
    
    int choice = -1;
    while (choice != 0)
    {
        showMenu();
        cin >> choice;
        
        switch (choice)
        {
        case 1:
            TestLexer();
            break;
        case 2:
            TestParser();
            break;
        case 3:
            TestSymTable();
            break;
        case 4:
            TestPCode();
            break;
        case 5:
    Test();
            break;
        case 0:
            wcout << L"程序退出" << endl;
            break;
        default:
            wcout << L"无效选项，请重新选择" << endl;
            break;
        }
    }
    
    return 0;
}
