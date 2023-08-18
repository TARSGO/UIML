
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

enum UimlYamlParseStatus {
    UimlYamlInvalidIndentation = -1,
    UimlYamlInvalidKey = -2,
    UimlYamlInvalidValue = -3,
};

#ifdef __cplusplus
extern "C" {
#endif

size_t UimlParseYaml(const char* input, struct UimlYamlNode** output);


#ifdef __cplusplus
} // extern "C" {
#endif
