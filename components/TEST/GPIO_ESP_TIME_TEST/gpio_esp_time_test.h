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

//========ESP-IDF库==================================
#include "mcu_gpio_setup.h"                                                                         // 引入GPIO定义库
#include "esp_time_framework.h"                                                                     // 引入自定义ESP_TIME库
#include "gpio_level_switch.h"                                                                      // GPIO引脚控制

/**
 * @brief 输出PWM测试
 * @param  无
 * @return 无
 */
void gpio_esp_time_test_fun(void);

#endif