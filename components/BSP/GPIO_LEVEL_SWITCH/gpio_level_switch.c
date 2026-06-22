/**
 ****************************************************************************************************
 * @file        gpio_level_switch.c
 * @version     V1.0
 * @date        2026-06-16
 * @brief       GPIO引脚电平控制
 ****************************************************************************************************
 */

#include "gpio_level_switch.h"

#pragma region 输出引脚控制
/**
 * @brief 设置输出引脚电平
 * @param gpio_pin : 引脚编号
 * @param enablde : 设置默认输出0低电平，1高电平
 * @return : 返回值或错误代码
 */
return_value gpio_set_lv(gpio_num_t gpio_pin, int8_t enabled)
{
    //ESP_LOGD(TAG, "》》》当前执行代码行[%d]，函数：%s ，参数GPIO_PIN = %d, enabled = %d。",
    //     __LINE__, __FUNCTION__, gpio_pin, enabled);                                                // 进入函数前输出调试日志，得知执行到哪个函数
    if(enabled < 0)                                                                                 // 如果enabled少于0
        enabled = 0;                                                                                // 则输出低电平
    if(enabled > 1)                                                                                 // 如果enablde大于1
        enabled = 1;                                                                                // 则输出高电平
    TRY1(gpio_set_level(gpio_pin, enabled));                                                        // 设置引脚电平    
    return RV_OK;
cleanup:
    return ERR_GPIO_SET_LEVEL_FAILURE;                                                              // 设置电平失败
}

/**
 * @brief 输出引脚初始化函数（把引脚定义成输出引脚）
 * @param gpio_pin : 引脚编号
 * @param pullup : 设置是否需要上拉电阻(内部弱上拉45KΩ) 0 不启用，1 启用
 * @param pulldown : 设置是否需要下拉电阻（内部弱下拉45KΩ） 0 不启用，1 启用
 * @param enablde : 设置默认输出0低电平，1高电平
 * @return : 返回值或错误代码
 */
return_value gpio_output_init(gpio_num_t gpio_pin, int8_t pullup, int8_t pulldown, int8_t enabled)
{
    //ESP_LOGD(TAG, "》》》当前执行代码行[%d]，函数：%s ，参数GPIO_PIN = %d, PULLUP = %d pulldown = %d, enabled = %d。",
    //     __LINE__, __FUNCTION__, gpio_pin, pullup, pulldown, enabled);                              // 进入函数前输出调试日志，得知执行到哪个函数
    if(pullup < 0 || pullup > 1)                                                                    // 检查输入参数是否不是0或1
        return ERR_GPIO_PULLUP_EXCESSIVE;                                                           // 返回超限错误
    if(pulldown < 0 || pulldown > 1)                                                                // 检查输入参数是否不是0或1
        return ERR_GPIO_PULLDOWN_EXCESSIVE;                                                         // 返回超限错误

    gpio_config_t gpio_init_struct = {0};                                                           // 创建GPIO的结构体

    gpio_init_struct.intr_type = GPIO_INTR_DISABLE;                                                 // 失能引脚中断
    gpio_init_struct.mode = GPIO_MODE_INPUT_OUTPUT;                                                 // 设置引脚为输出引脚
    gpio_init_struct.pull_up_en = pullup;                                                           // 失能上拉或使能上拉
    gpio_init_struct.pull_down_en = pulldown;                                                       // 失能下拉或使能下拉
    gpio_init_struct.pin_bit_mask = (1ull << gpio_pin) | (1ull << gpio_pin);                        // 设置引脚的位掩码

    TRY1(gpio_config(&gpio_init_struct));                                                           // 初始化引脚
    if(gpio_set_lv(gpio_pin, enabled) != ESP_OK)                                                    // 设置引脚电平
        goto cleanup2;                                                                              // 如果设置电平失败了
    return RV_OK;
cleanup:
    return ERR_ESP_TIME_PIN_PAUSE_PIN_NOT_FOUND;                                                                   // 如果初始化引脚出错了,则输出错误信息
cleanup2:
    return WARN_GPIO_INIT_SUCC_SET_LEVEL_FAILURE;                                                   // 如果只是设置电平出错，则返回警报代码
}

/**
 * @brief 设置输出引脚电平为原来电平的反电平
 * @param gpio_pin : 引脚编号
 * @return : 返回值或错误代码
 */
return_value gpio_toggle(gpio_num_t gpio_pin)
{
    TRY1(gpio_set_lv(gpio_pin, !gpio_get_level(gpio_pin)));                                         // 设置GPIO引脚的电平为原来的反电平
    return RV_OK;
cleanup:
    return ERR_GPIO_SET_LEVEL_FAILURE;                                                              // 设置电平失败
}

#pragma endregion