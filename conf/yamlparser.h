
#include <stddef.h>
#include <stdint.h>

struct UimlYamlNode {
    uint32_t NameHash;
    const char* NameRef;
    UimlYamlNode* Next;
    union {
        UimlYamlNode* Children;
        uint32_t U32;
        int32_t I32;
        float F32;
        char* Str;
    };
};

#ifdef __cplusplus
extern "C" {
#endif

size_t UimlYamlParse(const char* input, struct UimlYamlNode** output);

struct UimlYamlNode* UimlYamlGetValue(struct UimlYamlNode* input, const char* childName);
struct UimlYamlNode* UimlYamlGetValueByPath(struct UimlYamlNode* input, const char* path);

#ifdef __cplusplus
} // extern "C" {
#endif
