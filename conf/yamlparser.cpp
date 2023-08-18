
/**
实现一个最小化的、类YAML的文本解释器。

限制：
只能使用最基础的数据类型：int32, uint32, float, 字符串；字典。
字符串不支持任何形式的转义，而且必须以双引号包围。
根对象必须为字典。
一个键只能占用一行。
元素节点不保存其值类型，读取者读取时必须自行选取与解析器输出行为一致的类型。

键名不得包含空格或者冒号（其他任意字符均可）。

*/

#include <string.h>
#include "yamlparser.h"
#include "hasher.h"
#include "vector.h"
#include "cmsis_os.h"

// RAII 守护堆分配
class AllocGuard {
public:
    AllocGuard(void** p) : guardedPtr(p) {};
    ~AllocGuard() { if (guardedPtr != nullptr) vPortFree(*guardedPtr); }
    void cancel() { guardedPtr = nullptr; }
private:
    void** guardedPtr;
};

// RAII 守护输出节点
class UimlYamlNodeGuard {
public:
    UimlYamlNodeGuard(UimlYamlNode** p) : guardedNode(p) {};
    ~UimlYamlNodeGuard() {
        if (guardedNode != nullptr) {
            if ((*guardedNode)->Children != NULL) {
                UimlYamlNodeGuard destructChildren(&(*guardedNode)->Children);
            }
            if ((*guardedNode)->Next != NULL) {
                auto chain = (*guardedNode)->Next;
                do {
                    auto nextnext = chain->Next;
                    UimlYamlNodeGuard destructChain(&chain);
                    chain = nextnext;
                } while (chain != NULL);
            }
            vPortFree(guardedNode);
        }
    }
    void cancel() { guardedNode = nullptr; }
private:
    UimlYamlNode** guardedNode;
};

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

    enum { SkipWhitespace, Key, Colon, TryValue, SkipToNewLine, ValueString, ValueNumber, ValueDict } state = SkipWhitespace;
    enum { NextKey, NextColon, NextValue, NextLine } nextElem = NextKey;
    enum { Undetermined, vtU32, vtI32, vtF32, vtString, vtDict } valueType = Undetermined;
    int spacesToSkip = 0;
    bool decrementSpacesToSkip = true;
    const char * keyNameRef = nullptr;
    size_t keyNameLength = 0;
    const char * strValueBegin = nullptr;
    char * strValueRef = nullptr;

    AllocGuard strValueGuard((void**)&strValueRef);
    UimlYamlNodeGuard outNodeGuard(&out);

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
        switch (state) {
            case SkipWhitespace:
                if (!strncmp("  bits", input + i, 6)) __debugbreak();
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
                        return UimlYamlParseStatus::UimlYamlInvalidIndentation;
                    }
                    gotoNextElem();
                } else if (spacesToSkip < 0) {
                    return UimlYamlParseStatus::UimlYamlInvalidIndentation;
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
                    return UimlYamlParseStatus::UimlYamlInvalidKey;
                }
                state = SkipWhitespace;
                break;

            case TryValue:
                if (c == '"') state = ValueString;
                else if ((c > '0' && c < '9') || c == '-') { state = ValueNumber; i--; }
                else if (c == ' ') state = SkipWhitespace;
                else if (c == '\n') state = ValueDict; 
                else return UimlYamlParseStatus::UimlYamlInvalidValue;
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

            case ValueNumber:
                break;

            case ValueDict:
                valueType = vtDict;
                i += UimlParseYamlDictIndent(input + i, &current->Children, indentSpace + 2);
                i--; // 加上lasti之后会指向\n，向前退一个字节，确保可以正确结束行
                state = SkipToNewLine; // 复用这部分代码做好下一个元素的准备
                break;

            case SkipToNewLine:
                if (c == '\n') { // 一行结束
                    if (strValueRef) {
                    } // TODO

                    switch (valueType) {
                        case Undetermined: // 没有值也没有Key的空行
                            break;
                        case vtU32:
                            break;
                        case vtI32:
                            break;
                        case vtF32:
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
                    lasti = i;
                    break;
                } else if (c != ' ') {
                    return UimlYamlParseStatus::UimlYamlInvalidValue;
                }
                break;
        }
    }

BreakLoop:

    *output = out;
    strValueGuard.cancel();
    outNodeGuard.cancel();

    return lasti == 0 ? i : lasti;
}

size_t UimlParseYaml(const char* input, UimlYamlNode** output) {
    return UimlParseYamlDictIndent(input, output, 0);
}
