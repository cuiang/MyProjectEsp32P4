/**
 ****************************************************************************************************
 * @file        __GPIO_ESP_TIME_TEST_.h
 * @version     V1.0
 * @date        2026-06-16
 * @brief       测试esp_time_framework模块，产生PWM
 ****************************************************************************************************
 */

#ifndef __GPIO_ESP_TIME_TEST_H_
#define __GPIO_ESP_TIME_TEST_H_

//========FreeRTOS库=================================
#include "freertos/FreeRTOS.h"                                                                      // FreeRTOS库
#include "freertos/task.h"                                                                          // FreeRTOS库：用于任务管理

//========自定义的库==================================
#include "mcu_gpio_setup.h"                                                                         // 引入GPIO定义库
#include "esp_time_framework.h"                                                                     // 引入自定义ESP_TIME库
#include "gpio_level_switch.h"                                                                      // GPIO引脚控制
#include "base_function.h"                                                                          // 引入自定义基础函数库

// 设置 TAG 的名称：为这份文件的文件名
#define FILENAME __FILE__                                                                           // 获取这个文件的文件名（含路径）
#define TAG (FILENAME + (strlen(FILENAME) - strlen(strrchr(FILENAME, '/' ) + 1)))                   // 把 TAG 设置为当前文件的文件名（不含路径）

/**
 * @brief 输出PWM测试
 * @param  无
 * @return 无
 */
void gpio_esp_time_test_fun(void);

#endif