
#pragma once

#include "platform.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum UimlYamlNodeDataType
{
    UYaAnyInteger32,
    UYaFloat32,
    UYaString,
    UYaDictionary,
};

struct UimlYamlNode
{
    struct
    {
        UimlYamlNodeDataType Type : 2;
        uint32_t NameHash : 30;
    };
    const char *NameRef;
    struct UimlYamlNode *Next;
    union {
        struct UimlYamlNode *Children;
        uint32_t U32;
        int32_t I32;
        float F32;
        char *Str;
        void *Ptr;
    };
};

#ifdef __cplusplus
extern "C"
{
#endif

size_t UimlYamlParse(const char *input, struct UimlYamlNode **output);

const struct UimlYamlNode *UimlYamlGetValue(const struct UimlYamlNode *input,
                                            const char *childName);
const struct UimlYamlNode *UimlYamlGetValueByPath(const struct UimlYamlNode *input,
                                                  const char *path);

// 由于数据结构里占用了最高2bit做类型表示，Hash对比时也需要去掉刚算出来的Hash的两bit
inline uint32_t UimlYamlPartialHash(uint32_t x) { return x & (~0xC0000000); }

#ifdef __cplusplus
} // extern "C" {

class UimlYamlNodeObject
{
  private:
    const UimlYamlNode *m_node;

  public:
    UimlYamlNodeObject() = delete;
    UimlYamlNodeObject(const UimlYamlNode *node) : m_node(node) {}

    operator const UimlYamlNode *() const { return m_node; }

    UimlYamlNodeObject operator[](const char *childName) const
    {
        return UimlYamlNodeObject(UimlYamlGetValue(m_node->Children, childName));
    }

    template <typename T> T get(T defval)
    {
        static_assert(sizeof(T) <= sizeof(size_t));

        return m_node ? *reinterpret_cast<const T *>(&m_node->U32) : defval;
    }
};
#endif
