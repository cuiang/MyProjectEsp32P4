/**
 ****************************************************************************************************
 * @file        wdt.c
 * @version     V1.0
 * @date        2026-06-16
 * @brief       任务看门狗驱动代码
 ****************************************************************************************************
 */

#include "wdt.h"

esp_timer_handle_t wdt_tim_handle;                                                                  // 定时器回调函数句柄

/**
 * @brief       初始化任务看门狗计时器
 * @param       arr: 自动重装载值
 *              tps: 定时器周期（微秒）
 */
void wdt_init(uint64_t tps)
{
    /* 定义一个定时器结构体 */
    esp_timer_create_args_t tim_periodic_arg = {
        .name = "wdt_timer",                                                                        // 定时器名称
        .callback =	&wdt_isr_handler,                                                               // 设置回调函数
        .arg = NULL,                                                                                // 不携带参数
    };

    /* 创建定时器事件 */
    esp_timer_create(&tim_periodic_arg, &wdt_tim_handle);                                           // 创建一个事件
    esp_timer_start_periodic(wdt_tim_handle, tps);                                                  // 每周期内触发一次
}

/**
 * @brief       （喂狗）重新启动当前运行的计时器
 * @param       timeout: 定时器超时时间，该超时时间以微妙作为基本计算单位，故而设置超时时间为1s，则需要转换为微妙（μs），即timeout = 1s = 1000000μs
 * @retval      无
 */
void restart_timer(uint64_t timeout)
{
    esp_timer_restart(wdt_tim_handle, timeout);                                                     // 重新启动当前运行的计时器，用以模拟喂狗过程
}

/**
 * @brief       看门狗回调函数
 * @param       arg: 无参数传入
 * @retval      无
 */
void IRAM_ATTR wdt_isr_handler(void *arg)
{
    ESP_LOGE(TAG, "》》》当前执行代码行[%d]，函数%s ：未喂狗，导致程序复位。", __LINE__, __FUNCTION__); // 进入函数前输出调试日志，得知执行到哪个函数
    esp_restart();                                                                                  // 若没有及时进行喂狗，那么芯片将一直进行复位
}
