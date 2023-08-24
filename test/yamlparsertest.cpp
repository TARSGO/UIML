
#include "../conf/yamlparser.h"
#include <iostream>
#include <functional>

constexpr const char* yaml_simple = 
R"(
sys:
  rotate-pid:
    p: 1.5
    i: 0.0
    d: 0.0
    max-i: 100.0
    max-out: 200.0

chassis:
  task-interval: 2
  info:
    wheelbase: 100.0  #这里是注释
    wheeltrack: 100.0
    wheel-radius: 76.0
    offset-x: 0.0
    offset-y: 0.0
    #ThisIsAComment: 0
  move:
    max-vx: 2000.0
    max-vy: 2000.0

testcase:
  minus:
    int: -65537
    float: -123.456
  positive:
    int: 65536
    float: 12.3456
  string:
    hello: "world"
)";

template <typename T> bool CompareYamlValue(UimlYamlNode* node, T value) {
    return !memcmp(&(node->I32), &value, sizeof(T));
}
template <> bool CompareYamlValue(UimlYamlNode* node, const char* value) {
    return !strcmp(node->Str, value);
}
template <> bool CompareYamlValue(UimlYamlNode* node, float value) {
    return (node->F32 - value <= 1e-6);
}
template <> bool CompareYamlValue(UimlYamlNode* node, double value) {
    return (node->F32 - value <= 1e-6);
}

template <typename T> void AssertYamlValue(UimlYamlNode* root, const char* path, T value) {
    auto node = UimlYamlGetValueByPath(root, path);
    if (node == NULL) {
        std::cerr << "FAIL: Requested value of \"" << path << "\" but the item does not exist!\n";
        return;
    }
    if (!CompareYamlValue(node, value)) {
        std::cerr << "FAIL: Value of \"" << path << "\" expected: " << value << ", got: " << *reinterpret_cast<T*>(&(node->I32)) << '\n';
    }
}

void TestYamlParser() {
    std::cerr << __FUNCTION__ << std::endl;
    UimlYamlNode* root;

    UimlYamlParse(yaml_simple, &root);
    auto pChassis = UimlYamlGetValue(root->Children, "chassis");
    auto pInfo = UimlYamlGetValue(pChassis->Children, "info");
    auto pOffsetX = UimlYamlGetValue(pInfo->Children, "wheel-radius");

    std::cerr << "YAML test didn't crash." << std::endl;

    AssertYamlValue(root, "/chassis/info/wheel-radius", 76.0);
    AssertYamlValue(root, "/testcase/minus/int", -65537);
    AssertYamlValue(root, "/testcase/minus/float", -123.456);
    AssertYamlValue(root, "/testcase/positive/int", 65536);
    AssertYamlValue(root, "/testcase/positive/float", 12.3456);
    AssertYamlValue(root, "/testcase/string/hello", "world");

    std::cerr << "chassis.info.wheel-radius: " << pOffsetX->F32 << std::endl;
    
    std::cerr << "Comment test: " << UimlYamlGetValueByPath(root, "/chassis/#ThisIsAComment") << std::endl;

}
