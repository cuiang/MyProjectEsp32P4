/**
 ****************************************************************************************************
 * @file        __GPIO_LEVEL_SWITCH_H_.h
 * @version     V1.0
 * @date        2026-06-16
 * @brief       GPIO引脚电平控制
 ****************************************************************************************************
 */

#ifndef __GPIO_LEVEL_SWITCH_H_
#define __GPIO_LEVEL_SWITCH_H_

//========C语言标准库=================================
#include <string.h>                                                                                 // 标准C语言输入输出库

//========ESP-IDF库==================================
#include "esp_log.h"                                                                                // ESP-IDF库：日志功能库
#include "driver/gpio.h"                                                                            // ESP-IDF库：GPIO引脚库

//========自定义的库==================================
#include "return_value.h"                                                                           // 返回值
#include "wdt.h"                                                                                    // 看门狗

// 设置 TAG 的名称：为这份文件的文件名
#define FILENAME __FILE__                                                                           // 获取这个文件的文件名（含路径）
#define TAG (FILENAME + (strlen(FILENAME) - strlen(strrchr(FILENAME, '/' ) + 1)))                   // 把 TAG 设置为当前文件的文件名（不含路径）

/**
 * @brief 设置输出引脚电平
 * @param gpio_pin : 引脚编号
 * @param enablde : 设置默认输出0低电平，1高电平
 * @return : 返回值或错误代码
 */
return_value gpio_set_lv(gpio_num_t gpio_pin, int8_t enabled);

/**
 * @brief 输出引脚初始化函数（把引脚定义成输出引脚）
 * @param gpio_pin : 引脚编号
 * @param pullup : 设置是否需要上拉电阻(内部弱上拉45KΩ) 0 不启用，1 启用
 * @param pulldown : 设置是否需要下拉电阻（内部弱下拉45KΩ） 0 不启用，1 启用
 * @param enablde : 设置默认输出0低电平，1高电平
 * @return : 返回值或错误代码
 */
return_value gpio_output_init(gpio_num_t gpio_pin, int8_t pullup, int8_t pulldown, 
    int8_t enabled);

/**
 * @brief 设置输出引脚电平为原来电平的反电平
 * @param gpio_pin : 引脚编号
 * @return : 返回值或错误代码
 */
return_value gpio_toggle(gpio_num_t gpio_pin);
#endif