
/**
实现一个最小化的、类YAML的，在C板上消耗最小化资源的文本解析器。

限制：
只能使用最基础的数据类型：int32, uint32, float, 字符串；字典。
字符串不支持任何形式的转义，而且必须以双引号包围。
根对象必须为字典。
一个键只能占用一行。
元素节点不保存其值类型，读取者读取时必须自行选取与解析器输出行为一致的类型。

只支持小数形式的浮点数（123.456），不支持指数形式。
如数字被写作浮点形式，即使它的小数部分在当前精度下会变为0，值也会被按float存储。

键名不得包含空格或者冒号（其他任意字符均可）。

没有错误返回机制，因为将Children（字典）与其他32位长度的类型union起来时，已无法自动释放资源。
检测到未预期的字符时，会直接试图跳转至结尾状态。
*/

#include <string.h>
#include "yamlparser.h"
#include "hasher.h"
extern "C" {
#include "vector.h"
}
#include "cmsis_os.h"

static UimlYamlNode* CreateYamlNode() {
    UimlYamlNode* ret = (UimlYamlNode*)pvPortMalloc(sizeof(UimlYamlNode));
    ret->Children = NULL;
    ret->Next = NULL;
    ret->NameRef = NULL;
    ret->NameHash = 0;
    ret->I32 = 0;
    return ret;
}

static size_t UimlParseYamlDictIndent(const char* input, UimlYamlNode** output, int indentSpace) {
    UimlYamlNode* out = nullptr, *current = nullptr;
    auto inputLength = strlen(input);

    enum { SkipWhitespace, Key, Colon, TryValue, SkipToNewLine, ValueString, ValueNumberBegin, ValueNumberInteger, ValueNumberDecimal, ValueDict } state = SkipWhitespace;
    enum { NextKey, NextColon, NextValue, NextLine } nextElem = NextKey;
    enum { Undetermined, vtU32, vtNeg32, vtF32, vtNegF32, vtString, vtDict } valueType = Undetermined;
    int spacesToSkip = 0;
    bool decrementSpacesToSkip = true;
    const char * keyNameRef = nullptr;
    size_t keyNameLength = 0;
    union {
        const char * strValueBegin = nullptr;
        uint32_t integerPart;
    };
    union {
        char * strValueRef = nullptr;
        float decimalPart;
    };
    float decimalCoeff = 1.0f;

    auto gotoNextElem = [&](){
        switch (nextElem) {
            case NextKey: state = Key; nextElem = NextColon; break;
            case NextColon: state = Colon; nextElem = NextValue; break;
            case NextValue: state = TryValue; nextElem = NextLine; break;
            case NextLine: state = SkipToNewLine; break;
        }
    };

    spacesToSkip = indentSpace;
    char c;
    size_t i, lasti = 0;
    for (i = 0; i < inputLength; i++) {
        c = input[i];
        if (c == '#' && state != ValueString) state = SkipToNewLine; // 注释起始
        switch (state) {
            case SkipWhitespace:
                if (c == '\n') {
                    if (nextElem == NextValue) {
                        state = ValueDict;
                    }
                    break;
                } else if (c != ' ') {
                    if (spacesToSkip == 0) {
                        spacesToSkip = 0;
                        decrementSpacesToSkip = false;
                        i--;
                    } else if (spacesToSkip == 2) { // 少一格缩进，回退
                        goto BreakLoop;
                    } else {
                        state = SkipToNewLine;
                    }
                    gotoNextElem();
                } else if (spacesToSkip < 0) {
                    state = SkipToNewLine;
                } else {
                    if (decrementSpacesToSkip) spacesToSkip--;
                }
                break;

            case Key:
                if (keyNameLength == 0) { // 初始状态
                    keyNameRef = input + i;
                    keyNameLength++;
                    // 新建节点，串入链表
                    auto newNode = CreateYamlNode();
                    if (current != nullptr) current->Next = newNode;
                    current = newNode;
                    current->NameRef = keyNameRef;
                    if (out == nullptr) out = current;
                } else { // 探测键名
                    if (c == ' ' || c == ':') { // 结束键名
                        i--;
                        state = SkipWhitespace;
                        current->NameHash = Hasher_UIML32((uint8_t*)keyNameRef, keyNameLength);
                    } else { // 键名继续
                        keyNameLength++;
                    }
                }
                break;
            
            case Colon:
                if (c != ':') {
                    state = SkipToNewLine;
                }
                state = SkipWhitespace;
                break;

            case TryValue:
                if (c == '"') state = ValueString;
                else if ((c >= '0' && c <= '9') || c == '-') { state = ValueNumberBegin; i--; }
                else if (c == ' ') state = SkipWhitespace;
                else if (c == '\n') state = ValueDict; 
                else state = SkipToNewLine;
                break;

            case ValueString:
                if (strValueBegin == nullptr) {
                    strValueBegin = input + i;
                } else if (c == '"') {
                    auto length = input + i - strValueBegin;
                    valueType = vtString;
                    strValueRef = (char*)pvPortMalloc(length + 1);
                    memcpy(strValueRef, strValueBegin, length);
                    strValueRef[length] = '\0';
                    state = SkipToNewLine;
                }
                break;

            case ValueNumberBegin:
                integerPart = 0;
                decimalPart = 0.0f;
                valueType = vtU32;
                if (c == '-') { valueType = vtNeg32; } // 拿Neg32表示是负数
                else i--; // 没有负号就回退一位，保证下一位读出来是数字
                state = ValueNumberInteger; // 直接交给专门处理数字的部分处理
                break;

            case ValueNumberInteger:
                if (c >= '0' && c <= '9') {
                    integerPart *= 10;
                    integerPart += (c - '0');
                } else if (c == '.') {
                    state = ValueNumberDecimal; // 去处理小数
                    valueType = (valueType == vtNeg32) ? vtNegF32 : vtF32;
                } else if (c == ' ' || c == '\n') {
                    i--;
                    state = SkipToNewLine; // 尝试结束
                } else {
                    // return UimlYamlInvalidValue;
                    state = SkipToNewLine;
                }
                break;

            case ValueNumberDecimal:
                if (c >= '0' && c <= '9') {
                    decimalCoeff *= 0.1f;
                    decimalPart += decimalCoeff * (c - '0');
                } else if (c == ' ' || c == '\n') {
                    i--;
                    state = SkipToNewLine; // 尝试结束
                } else {
                    // return UimlYamlInvalidValue;
                    state = SkipToNewLine;
                }
                break;

            case ValueDict:
            {
                valueType = vtDict;
                auto dictResult = UimlParseYamlDictIndent(input + i, &current->Children, indentSpace + 2);
                if ((int)dictResult < 0) { // 如果子级出错，递归退出
                    return dictResult;
                }
                i += dictResult;
                i--; // 加上lasti之后会指向\n，向前退一个字节，确保可以正确结束行
                state = SkipToNewLine; // 复用这部分代码做好下一个元素的准备
                break;
            }

            case SkipToNewLine:
                if (c == '\n') { // 一行结束
                    if (strValueRef) {
                    } // TODO

                    switch (valueType) {
                        case Undetermined: // 没有值也没有Key的空行
                            break;
                        case vtU32: // 没有负号的整数
                            current->U32 = integerPart;
                            break;
                        case vtNeg32: // 负的整数
                            current->I32 = -(int32_t)integerPart;
                            break;
                        case vtF32: // 没有负号的小数
                            current->F32 = integerPart + decimalPart;
                            break;
                        case vtNegF32: // 负的小数
                            current->F32 = -(integerPart + decimalPart);
                            break;
                        case vtString:
                            current->Str = strValueRef;
                            strValueRef = nullptr;
                            break;
                        case vtDict: break;
                    }
                    
                    spacesToSkip = indentSpace; // 解析下一个键值对
                    state = SkipWhitespace;
                    nextElem = NextKey;
                    decrementSpacesToSkip = true;
                    keyNameLength = 0;
                    strValueBegin = 0;
                    strValueRef = nullptr;
                    decimalCoeff = 1.0f;
                    lasti = i;
                    break;
                } else if (c != ' ') {
                    // return UimlYamlInvalidValue;
                }
                break;
        }
    }

BreakLoop:

    *output = out;

    return lasti == 0 ? i : lasti;
}

size_t UimlYamlParse(const char* input, UimlYamlNode** output) {
    *output = CreateYamlNode();
    return UimlParseYamlDictIndent(input, &((*output)->Children), 0);
}

UimlYamlNode* UimlYamlGetValue(UimlYamlNode* input, const char* childName) {
    auto nameLength = strlen(childName);
    auto nameHash = Hasher_UIML32((uint8_t*)childName, nameLength);
    UimlYamlNode* ret = input;
    
    while (ret != NULL) {
        if (ret->NameHash == nameHash && !strncmp(ret->NameRef, childName, nameLength)) {
            return ret;
        }
        ret = ret->Next;
    }

    return NULL;
}

UimlYamlNode* UimlYamlGetValueByPath(UimlYamlNode* input, const char* path) {
    const char *begin = path, *end;
    UimlYamlNode *ret = input;

    do {
        end = strchr(begin, '/');
        if (end > begin || end == NULL) {
            auto nameLength = (end == NULL) ? strlen(begin) : (end - begin);
            auto nameHash = Hasher_UIML32((uint8_t*)begin, nameLength);
            
            ret = ret->Children;

            while (ret != NULL) {
                if (ret->NameHash == nameHash && !strncmp(ret->NameRef, begin, nameLength)) {
                    goto Found; // 跳出内循环并跳过错误情况处理
                }
                ret = ret->Next;
            }

            return NULL;
Found:
            begin = end + 1;
        } else if (end == begin) {
            begin = end + 1; // 没用的单/，只跳过
        }
    } while (end != NULL);

    return ret;
}
