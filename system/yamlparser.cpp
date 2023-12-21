
/**
实现一个最小化的、类YAML的，在C板上消耗最小化资源的文本解析器。

限制：
只能使用最基础的数据类型：int32, uint32, float, 字符串；字典。
字符串不支持任何形式的转义，而且必须以双引号包围。
根对象必须为字典。
一个键只能占用一行。
元素节点不保存其值类型，读取者读取时必须自行选取与解析器输出行为一致的类型。

整数可以是1234这样写的形式，也可以是0x1234这样写的十六进制形式。其他写法不支持。
只支持小数形式的浮点数（123.456），不支持指数形式。
如数字被写作浮点形式，即使它的小数部分在当前精度下会变为0，值也会被按float存储。

键名不得包含空格或者冒号（其他任意字符均可）。

没有错误返回机制，因为将Children（字典）与其他32位长度的类型union起来时，已无法自动释放资源。
检测到未预期的字符时，会直接试图跳转至结尾状态。
*/

#include "yamlparser.h"
#include "hasher.h"
#include <stdio.h>
#include <string.h>

extern "C"
{
#include "vector.h"
}
#include "cmsis_os.h"
#include "strict.h"

#ifdef UIML_TESTCASE
#define Dbg(x, ...) fprintf(stderr, __FUNCTION__##": " x, ##__VA_ARGS__)
size_t BuiltKeyCount = 0;
size_t RemovedNamerefCount = 0;
#else
#define Dbg(x, ...)
#endif

struct ParserContext
{
    const char *Input;
    size_t Length;
    size_t Offset;
    size_t SubDictEnd;

    bool IsEOF() { return Offset >= Length; }
    int Peek()
    {
        if (!IsEOF())
            return Input[Offset];
        else
            return EOF;
    }
    int Take()
    {
        if (!IsEOF())
            return Input[Offset++];
        else
            return EOF;
    }
    int Put()
    {
        if (Offset > 0)
            return Input[--Offset];
        else
            return EOF;
    }
};

enum ParserState
{
    PS_OK,
    PS_EOF,
    PS_EOL,
    PS_DictEnd, // 得到的缩进比预期少且正好为2的倍数，表示正在解析的字典已经结束
    PS_InvalidChar,
};

static inline UimlYamlNode *CreateYamlNode()
{
    UimlYamlNode *ret = (UimlYamlNode *)pvPortMalloc(sizeof(UimlYamlNode));
    ret->Children = NULL;
    ret->Next = NULL;
    ret->NameRef = NULL;
    ret->NameHash = 0;
    ret->I32 = 0;
    return ret;
}

static inline ParserState UimlParseYamlSkipComment(ParserContext &ctx)
{
    if (ctx.Peek() != '#')
    {
        Dbg("invalid char %c at %d\n", ctx.Peek(), ctx.Offset);
        return PS_InvalidChar;
    }

    while (ctx.Peek() != '\n')
    {
        if (ctx.IsEOF())
        {
            Dbg("EOF at %d\n", ctx.Offset);
            return PS_EOF;
        }

        ctx.Take();
    }
    ctx.Take(); // 取出\n
    Dbg("skip comment at %d\n", ctx.Offset);
    return PS_EOL;
}

// 跳过缩进以及缩进后的注释
static ParserState UimlParseYamlSkipIndentation(ParserContext &ctx, int indentSize)
{
    int indent = 0;
    if (ctx.IsEOF())
    {
        Dbg("EOF at %d\n", ctx.Offset);
        return PS_EOF;
    }

    while (ctx.Peek() == ' ')
    { // 试图跳过所有应该跳过的空格
        indent++;
        ctx.Take();
    }

    if (ctx.Peek() == '\n') // 如果是换行符，则返回EOL
    {
        ctx.Take();
        Dbg("skip empty indent at %d\n", ctx.Offset);
        return PS_EOL;
    }
    else if (ctx.Peek() == '#') // 如果这里是注释（允许存在），则跳过注释
    {
        Dbg("skip indented comment at %d, got indent depth %d\n", ctx.Offset, indent);
        return UimlParseYamlSkipComment(ctx);
    }
    else if (indent == indentSize)
    {
        Dbg("indentation match at %d, expected and got %d\n", ctx.Offset, indentSize);
        return PS_OK;
    }
    else if (indentSize != indent &&
             (indentSize - indent) % 2 == 0) // 如果正好缺2n个空格，则认为是字典结束
    {
        Dbg("dict end at %d, expected %d, got %d\n", ctx.Offset, indentSize, indent);
        return PS_DictEnd;
    }
    else // 否则，跳过的空格个数与参数指定的不匹配，则认为缩进不匹配，返回错误
    {
        Dbg("indentation mismatch at %d, expected %d, got %d\n", ctx.Offset, indentSize, indent);
        return PS_InvalidChar;
    }
}

static inline ParserState UimlParseYamlSkipSpaces(ParserContext &ctx)
{
    while (ctx.Peek() == ' ')
        ctx.Take();

    switch (ctx.Peek())
    {
    case '\n':
        ctx.Take();
        return PS_EOL;
    case '#':
        return UimlParseYamlSkipComment(ctx);
    case EOF:
        return PS_EOF;
    default:
        return PS_OK;
    }
}

static ParserState UimlParseYamlGetKeyName(ParserContext &ctx, UimlYamlNode *nodeRef)
{
    // 读取键名，直到遇到冒号或者空格
    auto originalOffset = ctx.Offset;

    while (ctx.Peek() != ':' && ctx.Peek() != ' ')
    {
        if (ctx.Peek() == '\n')
        {
            Dbg("invalid char %c at %d\n", ctx.Peek(), ctx.Offset);
            return PS_InvalidChar;
        }

        ctx.Take();
    }

    switch (ctx.Offset - originalOffset)
    {
    case 1:
        Dbg("key name: %c\n", ctx.Input[originalOffset]);
        break;
    case 2:
    case 3:
        Dbg("key name: %c%c\n", ctx.Input[originalOffset], ctx.Input[originalOffset + 1]);
        break;
    case 4:
    case 5:
        Dbg("key name: %c%c%c\n",
            ctx.Input[originalOffset],
            ctx.Input[originalOffset + 1],
            ctx.Input[originalOffset + 2]);
        break;
    default:
        Dbg("key name: %c%c%c%c\n",
            ctx.Input[originalOffset],
            ctx.Input[originalOffset + 1],
            ctx.Input[originalOffset + 2],
            ctx.Input[originalOffset + 3]);
        break;
    }

    // 计算键名的哈希值
    nodeRef->NameHash =
        Hasher_UIML32((uint8_t *)(ctx.Input + originalOffset), ctx.Offset - originalOffset);
    nodeRef->NameRef = ctx.Input + originalOffset;

    // 跳过冒号及冒号前面的空格
    while (ctx.Peek() == ' ')
        ctx.Take();

    if (ctx.Peek() != ':')
    {
        Dbg("invalid char %c at %d, expected colon\n", ctx.Peek(), ctx.Offset);
        return PS_InvalidChar;
    }

    ctx.Take();

    return PS_OK;
}

static ParserState UimlParseYamlStringValue(ParserContext &ctx, char **output)
{
    size_t originalOffset;

    if (ctx.Peek() == '"') // 双引号，认为是字符串
    {
        ctx.Take();
        originalOffset = ctx.Offset;
        while (ctx.Peek() != '"')
        {
            if (ctx.Peek() == '\n')
            {
                Dbg("invalid char %c at %d\n", ctx.Peek(), ctx.Offset);
                return PS_InvalidChar;
            }

            ctx.Take();
        }
        ctx.Take();
    }
    else
    {
        Dbg("invalid char %c at %d\n", ctx.Peek(), ctx.Offset);
        return PS_InvalidChar;
    }

    // 读取字符串
    *output = (char *)pvPortMalloc(
        ctx.Offset - originalOffset); // 当前offset在后引号上，不需要加1就已经留有了\0的空间
    memcpy(*output, ctx.Input + originalOffset, ctx.Offset - originalOffset);
    (*output)[ctx.Offset - originalOffset - 1] = '\0'; // 需要多减去后引号的位置

    Dbg("string value %s at %d\n", *output, ctx.Offset);

    return PS_OK;
}

static ParserState UimlParseYamlNumericValue(ParserContext &ctx, UimlYamlNode *output)
{
    bool isNegative = false;
    bool isHex = false;
    uint32_t integerPart = 0;

    // 判断负号
    char c = ctx.Peek();
    if (c == '-')
    {
        isNegative = true;
        ctx.Take();
        c = ctx.Peek();
    }

    // 判断十六进制前缀
    if (c == '0')
    {
        ctx.Take();

        // 新开一个c，如果不是x的话不能影响原来是数字的那个c
        char c2 = ctx.Peek();
        if (c2 == 'x' || c2 == 'X')
        {
            isHex = true;
            ctx.Take();
            c = ctx.Peek();
        }
        else
        {
            // 为满足下一阶段的预期，需要将0放回去， 也就是倒退一字节
            ctx.Put();
        }
    }

    // 运行到这一步，预期是c为Peek到的第一个数字
    if ((c >= '0' && c <= '9') || (isHex && ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))))
    {
        // 解析整数部分
        do
        {
            if (isHex)
            {
                if (c >= '0' && c <= '9')
                    integerPart = integerPart * 16 + (c - '0');
                else if (c >= 'a' && c <= 'f')
                    integerPart = integerPart * 16 + (c - 'a' + 10);
                else if (c >= 'A' && c <= 'F')
                    integerPart = integerPart * 16 + (c - 'A' + 10);
            }
            else
            {
                integerPart = integerPart * 10 + (c - '0');
            }
            ctx.Take();
            c = ctx.Peek();
        } while ((c >= '0' && c <= '9') ||
                 (isHex && ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))));

        // 如果是十六进制，则不应该有小数点
        if (isHex && c == '.')
        {
            Dbg("invalid char %c at %d\n", ctx.Peek(), ctx.Offset);
            return PS_InvalidChar;
        }

        // 判断是否为小数点
        if (c == '.' && !isHex)
        {
            ctx.Take();     // 丢弃小数点
            c = ctx.Peek(); // 读入数字

            float decimalCoeff = 0.1f, decimalPart = 0.0f;
            // 解析小数部分
            while (c >= '0' && c <= '9')
            {
                decimalPart += (c - '0') * decimalCoeff;
                ctx.Take();
                c = ctx.Peek();
                decimalCoeff *= 0.1f;
            }

            output->Type = UYaFloat32;
            output->F32 = integerPart + decimalPart;
            if (isNegative)
                output->F32 = -output->F32;
            Dbg("float value %f at %d\n", (isNegative ? -1 : 1) * output->F32, ctx.Offset);
            return PS_OK;
        }
        else
        {
            // 整数部分结束
            output->Type = UYaAnyInteger32;
            if (isNegative)
                output->I32 = -(int32_t)integerPart;
            else
                output->U32 = integerPart;

            if (isHex)
                Dbg("hex value %d at %d\n", (isNegative ? -1 : 1) * output->I32, ctx.Offset);
            else
                Dbg("int value %d at %d\n", (isNegative ? -1 : 1) * output->I32, ctx.Offset);

            return PS_OK;
        }
    }
    else
    {
        Dbg("invalid char %c at %d\n", ctx.Peek(), ctx.Offset);
        return PS_InvalidChar;
    }
}

static ParserState UimlParseYamlValue(ParserContext &ctx, UimlYamlNode *output)
{
    // 先判断值的类型，是数字还是字符串
    auto originalOffset = ctx.Offset;
    auto state = PS_OK;

    auto c = ctx.Peek();
    if ((c >= '0' && c <= '9') || c == '-') // 负号或者数字，认为这是一个数值类型
    {
        Dbg("numeric value at %d\n", ctx.Offset);
        return UimlParseYamlNumericValue(ctx, output);
    }
    else if (c == '"') // 双引号，认为是字符串
    {
        Dbg("string value at %d\n", ctx.Offset);
        output->Type = UYaString;
        return UimlParseYamlStringValue(ctx, &(output->Str));
    }
    else
    {
        Dbg("invalid char %c at %d\n", ctx.Peek(), ctx.Offset);
        return PS_InvalidChar;
    }
}

static ParserState UimlParseYamlDictIndent(ParserContext &ctx,
                                           UimlYamlNode **output,
                                           int indentSpace)
{
    auto state = PS_OK;
    ctx.SubDictEnd = ctx.Offset; // 任何地方正常遇到合法行尾时都要更新

    // 当成函数用的，用于将节点串入链表
    auto addIntoOutput = [&](UimlYamlNode *node) {
        // 优化生成的字典，如果某个哈希值不产生碰撞，则把NameRef置空，表明不需要再次核对名称
        {
            auto current = node;
            while (current != NULL)
            {
                auto next = current->Next;
                auto hash = current->NameHash;
                auto collision = false;
                while (next != NULL)
                {
                    if (next->NameHash == hash)
                    {
                        collision = true;
                        break;
                    }
                    next = next->Next;
                }
                if (!collision)
                {
                    Dbg("Removed NameRef for node %p\n", current);
#ifdef UIML_TESTCASE
                    RemovedNamerefCount++;
#endif
                    current->NameRef = NULL;
                }
                current = next;
            }
        }

        // 如果output是空的，就表明是第一个节点，直接放进去
        if (*output == NULL)
            *output = node;
        else
        {
            // 否则，找到最后一个节点，把新节点串在后面
            auto p = *output;
            while (p->Next != NULL)
                p = p->Next;
            p->Next = node;
            output = &(p->Next);
        }
    };

    while (true)
    {
        // 在此处peek应当为一行的起始（除非该行只有一个换行符）

        // 跳过缩进
        state = UimlParseYamlSkipIndentation(ctx, indentSpace);
        switch (state)
        {
        case PS_OK:
            break;
        case PS_EOL:
            ctx.SubDictEnd = ctx.Offset;
            continue;
            break; // 直接遇到行尾时，则继续下一行，并更新字典结尾位置
        default:
            return state;
        }

        // 新建节点
        auto node = CreateYamlNode();
        node->Type = UYaDictionary;
#ifdef UIML_TESTCASE
        BuiltKeyCount++;
#endif

        // 检测键名
        state = UimlParseYamlGetKeyName(ctx, node);
        switch (state)
        {
        case PS_OK:
            break;
        case PS_InvalidChar:
            return state;
        default:
            return state;
        }

        // 跳过空格
        state = UimlParseYamlSkipSpaces(ctx);
        switch (state)
        {
        case PS_OK:
            break;
        case PS_EOL: {
            // 直接遇到行尾时，此时试图解析字典
            // node->Children = CreateYamlNode();
            state = UimlParseYamlDictIndent(ctx, &(node->Children), indentSpace + 2);
            switch (state)
            {
            case PS_DictEnd:
                ctx.Offset = ctx.SubDictEnd;
                addIntoOutput(node);
                continue; // 恢复偏移值到字典结束处，继续解析下一行
            case PS_EOF:
                addIntoOutput(node);
                return state; // 遇到文件结尾，需要单独串进去。返回state以便上层函数检测到结尾
            case PS_OK:
                break;
            default:
                return state;
            }
            break;
        }
        default:
            return state;
        }

        // 解析值
        state = UimlParseYamlValue(ctx, node);
        switch (state)
        {
        case PS_OK:
            break;
        default:
            return state;
        }

        // 将节点串入链表
        addIntoOutput(node);

        // 跳过空格
        state = UimlParseYamlSkipSpaces(ctx);

        // 更新子字典结束位置
        ctx.SubDictEnd = ctx.Offset;

        switch (state)
        {
        case PS_OK:
        case PS_EOL:
            continue;
            break;
        case PS_EOF: {
            Dbg("EOF at %d\n", ctx.Offset);
            return PS_OK;
            break;
        }
        default:
            return state;
        }
    }
}

size_t UimlYamlParse(const char *input, UimlYamlNode **output)
{
    *output = CreateYamlNode();
    ParserContext ctx = {input, strlen(input), 0};
    auto state = UimlParseYamlDictIndent(ctx, &((*output)->Children), 0);
    UIML_FATAL_ASSERT(ctx.Offset == ctx.Length, "YAML parser did not reach EOF!");
    return ctx.Offset;
}

const UimlYamlNode *UimlYamlGetValue(const UimlYamlNode *input, const char *childName)
{
    auto nameLength = strlen(childName);
    auto nameHash = UimlYamlPartialHash(Hasher_UIML32((uint8_t *)childName, nameLength));
    const UimlYamlNode *ret = input;

    while (ret != NULL)
    {
        if (ret->NameHash == nameHash &&
            ((ret->NameRef == NULL) || (!strncmp(ret->NameRef, childName, nameLength))))
        {
            return ret;
        }
        ret = ret->Next;
    }

    return NULL;
}

const UimlYamlNode *UimlYamlGetValueByPath(const UimlYamlNode *input, const char *path)
{
    const char *begin = path, *end;
    const UimlYamlNode *ret = input;

    do
    {
        end = strchr(begin, '/');
        if (end > begin || end == NULL)
        {
            auto nameLength = (end == NULL) ? strlen(begin) : (end - begin);
            auto nameHash = UimlYamlPartialHash(Hasher_UIML32((uint8_t *)begin, nameLength));

            ret = ret->Children;

            while (ret != NULL)
            {
                if (ret->NameHash == nameHash &&
                    ((ret->NameRef == NULL) || (!strncmp(ret->NameRef, begin, nameLength))))
                    goto Found; // 跳出内循环并跳过错误情况处理
                ret = ret->Next;
            }

            return NULL;
        Found:
            begin = end + 1;
        }
        else if (end == begin)
        {
            begin = end + 1; // 没用的单/，只跳过
        }
    } while (end != NULL);

    return ret;
}
