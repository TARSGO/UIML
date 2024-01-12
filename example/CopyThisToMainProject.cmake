
# 添加UIML
include(UIML/Uiml.cmake)
add_subdirectory(UIML)

# UIML配置项
set(UIML_ENABLE_GLOBAL_MANAGER ON) # 启用全局管理器与否
set(UIML_ENABLE_BSP_CAN ON)
set(UIML_ENABLE_BSP_SPI ON)
set(UIML_ENABLE_BSP_TIM ON)
set(UIML_ENABLE_BSP_UART ON)
set(UIML_ENABLE_BSP_GPIO OFF)
set(UIML_ENABLE_BSP_EXTI OFF)
target_link_uiml(TARGET_NAME [更改此处]
                 CPU_TYPE [更改此处]
                 SERVICE_LIST [更改此处]
                 STRICT_MODE)
