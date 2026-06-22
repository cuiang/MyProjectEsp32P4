/**
 ****************************************************************************************************
 * @file        __GPIO_GPIO_SETUP_H_.h
 * @version     V1.0
 * @date        2026-06-16
 * @brief       MCU GPIO 引脚定义设定，电路板对应所有引脚均在此设置
 ****************************************************************************************************
 */

#ifndef __GPIO_GPIO_SETUP_H_
#define __GPIO_GPIO_SETUP_H_

//========ESP-IDF库==================================
#include "esp_timer.h"                                                                              // ESP-IDF库：定时器功能库
#include "driver/gpio.h"                                                                            // ESP-IDF库：GPIO引脚库

typedef int gpio;

/* UART引脚定义 */

/* USB引脚定义 */

/* SPI引脚定义 */

/* IIC引脚定义 */

/* CAN引脚定义 */

/* ADC引脚定义 */

/* DAC引脚定义 */

/* 输入引脚定义 */

/* 输出引脚定义 */
#define LEDG_GPIO_PIN       GPIO_NUM_14                                                             // 慧勤智远 ESP32-P4 开发板的LED0连接GPIO 14 端口 控制绿色（红色为电源）
#define LEDB_GPIO_PIN       GPIO_NUM_13                                                             // 慧勤智远 ESP32-P4 开发板的LED1连接GPIO 13 端口 控制蓝色（此为三色灯）
#define PWM33_GPIO_PIN      GPIO_NUM_33                                                             // 自定义33脚为PWM输出脚
#define PWM34_GPIO_PIN      GPIO_NUM_34                                                             // 自定义34脚为PWM输出脚

#endif