/**
 * @file Types.cpp
 * @brief 通用类型与工具函数实现
 * @details 实现带缓冲区的Unicode文件读取器和字符串转换工具函数
 */

#include <types.hpp>
using namespace std;

// 全局偏移量，用于计算变量在栈帧中的位置
size_t glo_offset;

/**
 * @brief 判断宽字符是否为数字
 * @param ch 待判断的宽字符
 * @return 是数字返回true，否则返回false
 */
static bool isWDigit(wchar_t ch)
{
    return (ch >= L'0' && ch <= L'9');
}

/**
 * @brief 宽字符串转整数
 * @param numStr 待转换的宽字符串
 * @return 转换后的整数值，转换失败返回0
 * @details 采用移位运算优化乘10操作: x*10 = x*8 + x*2 = (x<<3) + (x<<1)
 */
int w_str2int(wstring numStr)
{
    // 空串检查
    if (numStr.empty()) {
        wcout << L"[Error] Cannot convert empty string to integer!" << endl;
        return 0;
    }
    
    // 合法性预检查
    for (const wchar_t& ch : numStr) {
        if (!isWDigit(ch)) {
            wcout << L"[Error] Invalid character in number string: '" << ch << L"'" << endl;
            return 0;
        }
    }
    
    // 转换计算
    int result = 0;
    for (size_t i = 0; i < numStr.length(); ++i) {
        result = (result << 3) + (result << 1);  // result *= 10
        result += (numStr[i] - L'0');
    }
    
    return result;
}

/**
 * @brief 整数转宽字符串
 * @param num 待转换的整数
 * @return 转换后的宽字符串
 */
wstring int2w_str(int num)
{
    if (num == 0) return L"0";
    
    wstring result = L"";
    bool isNegative = (num < 0);
    if (isNegative) num = -num;
    
    while (num > 0) {
        result = static_cast<wchar_t>(L'0' + num % 10) + result;
        num /= 10;
    }
    
    return isNegative ? L"-" + result : result;
}

/* ============================================================
 *           ReadUnicode 类实现 (带缓冲区)
 * ============================================================ */

/**
 * @brief 构造函数
 */
ReadUnicode::ReadUnicode()
    : isFileOpen(false), reachedEnd(false), 
      bufferStartPos(0), bufferLength(0), totalCharsLoaded(0)
{
    memset(buffer, 0, sizeof(buffer));
}

/**
 * @brief 析构函数
 */
ReadUnicode::~ReadUnicode()
{
    if (file.is_open()) {
        file.close();
    }
}

/**
 * @brief 初始化/重置读取器
 */
void ReadUnicode::InitReadUnicode()
{
    if (file.is_open()) {
        file.close();
    }
    isFileOpen = false;
    reachedEnd = false;
    bufferStartPos = 0;
    bufferLength = 0;
    totalCharsLoaded = 0;
    memset(buffer, 0, sizeof(buffer));
}

/**
 * @brief 计算UTF-8字符的编码长度
 * @param byte UTF-8首字节
 * @return 编码长度(1-4)，非法返回-1
 */
int ReadUnicode::calcUtf8Length(unsigned char byte)
{
    if (byte < 0x80)        return 1;  // 0xxxxxxx - ASCII
    if (byte < 0xC0)        return -1; // 10xxxxxx - 非法首字节
    if (byte < 0xE0)        return 2;  // 110xxxxx
    if (byte < 0xF0)        return 3;  // 1110xxxx
    if (byte <= 0xF4)       return 4;  // 11110xxx
    return -1;  // 超出有效范围
}

/**
 * @brief 从文件读取并解码一个UTF-8字符
 * @param outChar 输出的宽字符
 * @return 成功返回true，失败或EOF返回false
 * @details 自动跳过回车符(CR)，处理Windows换行
 */
bool ReadUnicode::readOneChar(wchar_t& outChar)
{
    int byte;
    
    // 读取字节，跳过回车符(处理Windows的\r\n)
    do {
        byte = file.get();
        if (byte == EOF) {
            return false;  // 文件结束
        }
    } while (byte == '\r');  // 跳过回车符
    
    unsigned char firstByte = static_cast<unsigned char>(byte);
    int charLen = calcUtf8Length(firstByte);
    
    if (charLen == -1) {
        wcout << L"[Error] Invalid UTF-8 byte: 0x" << hex << (int)firstByte << dec << endl;
        return false;
    }
    
    // 单字节ASCII字符
    if (charLen == 1) {
        outChar = firstByte;
        return true;
    }
    
    // 多字节UTF-8字符
    wchar_t codepoint = firstByte & (0xFF >> (charLen + 1));
    
    for (int i = 1; i < charLen; ++i) {
        byte = file.get();
        if (byte == EOF) {
            wcout << L"[Error] Incomplete UTF-8 sequence" << endl;
            return false;
        }
        unsigned char contByte = static_cast<unsigned char>(byte);
        // 验证后续字节格式 (10xxxxxx)
        if ((contByte & 0xC0) != 0x80) {
            wcout << L"[Error] Invalid UTF-8 continuation byte" << endl;
            return false;
        }
        codepoint = (codepoint << 6) | (contByte & 0x3F);
    }
    
    outChar = codepoint;
    return true;
}

/**
 * @brief 加载下一块缓冲区
 * @return 成功加载返回true，到达文件末尾返回false
 */
bool ReadUnicode::loadNextBuffer()
{
    if (!isFileOpen || reachedEnd) {
        return false;
    }
    
    // 更新缓冲区起始位置
    bufferStartPos += bufferLength;
    bufferLength = 0;
    memset(buffer, 0, sizeof(buffer));
    
    // 填充缓冲区
    wchar_t ch;
    while (bufferLength < BUFFER_SIZE - 1) {  // 保留1个位置给结束符
        if (!readOneChar(ch)) {
            // 文件结束，添加结束标记
            reachedEnd = true;
            buffer[bufferLength++] = L'#';
            wcout << L"[Info] End of file reached, total " << totalCharsLoaded << L" characters loaded" << endl;
            break;
        }
        buffer[bufferLength++] = ch;
        totalCharsLoaded++;
    }
    
    if (bufferLength > 0 && !reachedEnd) {
        wcout << L"[Info] Buffer loaded: pos " << bufferStartPos 
              << L" - " << (bufferStartPos + bufferLength - 1) << endl;
    }
    
    return bufferLength > 0;
}

/**
 * @brief 打开UTF-8文件并初始化缓冲区
 * @param filename 源文件路径
 */
void ReadUnicode::readFile2USC2(string filename)
{
    // 先重置状态
    InitReadUnicode();
    
    wcout << L"[Info] Opening file: " << filename.c_str() << endl;

    // 使用二进制模式打开以正确处理UTF-8编码
    file.open(filename, ios::in | ios::binary);
    if (!file.is_open()) {
        wcout << L"[Error] Failed to open file: " << filename.c_str() << endl;
        return;
    }
    
    isFileOpen = true;
    wcout << L"[Info] Compiling '" << filename.c_str() << L"' ..." << endl;
    
    // 检测并跳过UTF-8 BOM (0xEF 0xBB 0xBF)
    int b1 = file.get();
    int b2 = file.get();
    int b3 = file.get();
    
    if (b1 == 0xEF && b2 == 0xBB && b3 == 0xBF) {
        wcout << L"[Info] UTF-8 BOM detected, skipped" << endl;
    } else {
        // 不是BOM，回到文件开头
        file.seekg(0, ios::beg);
    }
    
    // 预加载第一块缓冲区
    loadNextBuffer();
}

/**
 * @brief 获取指定位置的字符
 * @param pos 字符在源程序中的全局位置(从0开始)
 * @return 指定位置的宽字符，无效位置返回'\0'
 * @details 如果请求位置不在当前缓冲区内，自动加载新的缓冲区
 */
wchar_t ReadUnicode::getProgmWStr(const size_t pos)
{
    // 检查是否在当前缓冲区范围内
    if (pos >= bufferStartPos && pos < bufferStartPos + bufferLength) {
        return buffer[pos - bufferStartPos];
    }
    
    // 请求位置在缓冲区之后，需要加载新缓冲区
    if (pos >= bufferStartPos + bufferLength && !reachedEnd) {
        // 循环加载直到找到目标位置或文件结束
        while (pos >= bufferStartPos + bufferLength && !reachedEnd) {
            if (!loadNextBuffer()) {
                break;
            }
        }
        
        // 再次检查是否在新缓冲区内
        if (pos >= bufferStartPos && pos < bufferStartPos + bufferLength) {
            return buffer[pos - bufferStartPos];
        }
    }
    
    // 位置无效或已到文件末尾
    return L'\0';
}

/**
 * @brief 判断源程序是否为空
 * @return true表示为空或未打开文件
 */
bool ReadUnicode::isEmpty()
{
    return !isFileOpen || (bufferLength == 0 && reachedEnd);
}

/**
 * @brief 获取已加载的总字符数
 * @return 总字符数(不含结束标记)
 */
size_t ReadUnicode::getLoadedCount()
{
    return totalCharsLoaded;
}

// Unicode读取器全局实例
ReadUnicode readUnicode;
