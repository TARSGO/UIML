
/**
 * @brief 全局管理器。可在软总线上注册几个管理函数，用来在外部管理电机、PID等资源。
 * 
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化全局管理器。功能组件不应该调用，RTOS的默认任务会调用。
 * 
 */
void Manager_Init();

#ifdef __cplusplus
}
#endif