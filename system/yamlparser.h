
#pragma once

#include <stddef.h>
#include <stdint.h>

struct UimlYamlNode
{
    uint32_t NameHash;
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

#ifdef __cplusplus
} // extern "C" {

class UimlYamlNodeObject
{
  private:
    const UimlYamlNode *m_node;

  public:
    UimlYamlNodeObject() = delete;
    UimlYamlNodeObject(const UimlYamlNode *node) : m_node(node)
    {
    }

    operator const UimlYamlNode *() const
    {
        return m_node;
    }

    UimlYamlNodeObject operator[](const char *childName) const
    {
        return UimlYamlNodeObject(UimlYamlGetValue(m_node, childName));
    }

    template <typename T> T get(T defval)
    {
        static_assert(sizeof(T) < sizeof(size_t));

        return m_node ? *reinterpret_cast<T *>(&m_node->U32) : defval;
    }
};
#endif
