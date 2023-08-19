
#include "../conf/yamlparser.h"
#include <iostream>

constexpr const char* yaml_simple = 
R"(
test1:
  foo: "hello"
  bar: "world"

test2:
  biz:
    wow: "okay"
  bits:
    p: 3000
    i:0.1
    d:3.0
)";

void TestYamlParser() {
    UimlYamlNode* here;

    UimlParseYaml(yaml_simple, &here);

    std::cerr << "YAML test didn't crash." << std::endl;
}
