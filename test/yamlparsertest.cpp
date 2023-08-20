
#include "../conf/yamlparser.h"
#include <iostream>

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
    wheelbase: 100.0
    wheeltrack: 100.0
    wheel-radius: 76.0
    offset-x: 0.0
    offset-y: 0.0
  move:
    max-vx: 2000.0
    max-vy: 2000.0
)";

void TestYamlParser() {
    UimlYamlNode* here;

    UimlYamlParse(yaml_simple, &here);
    auto pChassis = UimlYamlGetValue(here->Children, "chassis");
    auto pInfo = UimlYamlGetValue(pChassis->Children, "info");
    auto pOffsetX = UimlYamlGetValue(pInfo->Children, "wheel-radius");


    std::cerr << "chassis.info.wheel-radius: " << pOffsetX->F32 << std::endl;
    std::cerr << "chassis.info.wheel-radius: " << UimlYamlGetValueByPath(here, "/chassis/info/wheel-radius")->F32 << std::endl;

    std::cerr << "YAML test didn't crash." << std::endl;
}
