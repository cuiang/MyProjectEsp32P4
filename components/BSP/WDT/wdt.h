/**
 ****************************************************************************************************
 * @file        wdt.h
 * @version     V1.0
 * @date        2026-06-16
 * @brief       任务看门狗驱动代码
 ****************************************************************************************************
 */

#ifndef __WDT_H_
#define __WDT_H_

//========C语言标准库=================================


//========FreeRTOS库=================================
#include "freertos/FreeRTOS.h"                                                                      // FreeRTOS库
#include "freertos/task.h"                                                                          // FreeRTOS库：用于任务管理

//========ESP-IDF库==================================
#include "esp_timer.h"                                                                              // ESP-IDF库：定时器功能库
#include "string.h"                                                                                 // 标准C语言输入输出库
#include "esp_log.h"                                                                                // ESP-IDF库：日志功能库

// 设置 TAG 的名称：为这份文件的文件名
#define FILENAME __FILE__                                                                           // 获取这个文件的文件名（含路径）
#define TAG (FILENAME + (strlen(FILENAME) - strlen(strrchr(FILENAME, '/' ) + 1)))                   // 把 TAG 设置为当前文件的文件名（不含路径）

/* 参数定义 */
#define TWDT_TIMEOUT_MS         3000                                                                // 看门狗超时时间，设置为3000毫秒（3秒）
#define TASK_RESET_PERIOD_MS    2000                                                                // 任务重置周期，设置为2000毫秒（2秒）
#define MAIN_DELAY_MS           10000                                                               // 主循环延迟时间，设置为10000毫秒（10秒）

/* 函数声明 */
/**
 * @brief       初始化任务看门狗计时器
 * @param       arr: 自动重装载值
 *              tps: 定时器周期（微秒）
 */
void wdt_init(uint64_t tps);                                                                        // 初始化独立看门狗
/**
 * @brief       （喂狗）重新启动当前运行的计时器
 * @param       timeout: 定时器超时时间，该超时时间以微妙作为基本计算单位，故而设置超时时间为1s，则需要转换为微妙（μs），即timeout = 1s = 1000000μs
 * @retval      无
 */
void restart_timer(uint64_t timeout);                                                               // 重新启动当前运行的计时器
/**
 * @brief       看门狗回调函数
 * @param       arg: 无参数传入
 * @retval      无
 */
void IRAM_ATTR wdt_isr_handler(void *arg);

#endif
