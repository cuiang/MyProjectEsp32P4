/**
 ****************************************************************************************************
 * @file        esp_time_framework.h
 * @version     V1.0
 * @date        2026-06-17
 * @brief       ESP_TIME定时器
 * @note        目标频率	代入公式	精度	档位数
 * @note         20kHz	   20000/200   100%	    1
 * @note         10kHz	   10000/200	50%	    2
 * @note          5kHz	    5000/200	25%	    4
 * @note          1kHz	    1000/200	 5%	   20
 * @note         200Hz	     200/200	 1%	  100
 * @note         100Hz	     100/200   0.5%	  200
 ****************************************************************************************************
 */

#ifndef __ESP_TIME_FRAMEWORK_H_
#define __ESP_TIME_FRAMEWORK_H_

//========C语言标准库=================================
#include <string.h>                                                                                 // 标准C语言输入输出库

//========FreeRTOS库=================================
#include "freertos/FreeRTOS.h"                                                                      // FreeRTOS库
#include "freertos/task.h"                                                                          // FreeRTOS库：用于任务管理

//========ESP-IDF库==================================
#include "esp_timer.h"                                                                              // ESP-IDF库：定时器功能库
#include "driver/gpio.h"                                                                            // ESP-IDF库：GPIO引脚库
#include "esp_log.h"                                                                                // ESP-IDF库：日志库

//========自定义的库==================================
#include "return_value.h"                                                                           // 返回值

// 设置 TAG 的名称：为这份文件的文件名
#define FILENAME __FILE__                                                                           // 获取这个文件的文件名（含路径）
#define TAG (FILENAME + (strlen(FILENAME) - strlen(strrchr(FILENAME, '/' ) + 1)))                   // 把 TAG 设置为当前文件的文件名（不含路径）

// --- 新增：定义底层硬件定时器的频率 (100kHz, 10us) ---
// 这是 esp_timer 能稳定运行的高频率，用于提供高精度
#define TIMER_PERIOD 50                                                                             // 这是由软件定时器限定决定的，软件定时器最多每50us进入回调函数一次,也就是每秒1,000,000/50=20,000次
#define TIMER_FREQ_HZ 20000u                                                                        // 所以这里设定 ESP_TIME 输出的 PWM最多是20 KHz，但由于需要处理上升沿和下降沿，实际上一个周期最多为10KHz

// 定义 GPIO 绑定的状态枚举（用于控制引脚是正常输出还是暂停）
typedef enum {
    PIN_STATE_ACTIVE = 0,                                                                           // 正常输出 PWM
    PIN_STATE_PAUSED_LOW,                                                                           // 暂停输出，强制维持低电平
    PIN_STATE_PAUSED_HIGH,                                                                          // 暂停输出，强制维持高电平
    PIN_STATE_PAUSED_HOLD,                                                                          // 暂停输出，保持当前电平不变
    PIN_STATE_NOT_INIT,                                                                             // 引脚未初始化
} pin_state_t;

// 单个 GPIO 引脚的配置与状态结构体（每个引脚独立维护自己的状态）
typedef struct {
    gpio_num_t gpio_num;                                                                            // GPIO 引脚号（如 GPIO_NUM_14）
    uint32_t duty_percent;                                                                          // 占空比百分比 (0 ~ 100)
    pin_state_t state;                                                                              // 当前引脚状态（活跃/暂停）
    uint32_t pulse_target;                                                                          // 目标脉冲数量 (0 表示无限输出，>0 表示输出指定次数后暂停)
    uint8_t phase_invert;                                                                           // 相位偏移控制（0 正常相位， 1 反相/差分相位）
    uint32_t pulse_count;                                                                           // 当前已输出的脉冲计数
    pin_state_t post_state;                                                                         // 脉冲输出完成后的暂停状态（低/高/保持）
    int64_t error_accumulator;                                                                      // 用于误差扩散算法
    uint8_t last_level;                                                                             // 用于精确检测上升/下降沿数
} gpio_binding_t;

// 定时器任务配置结构体（管理整个定时器及其绑定的所有引脚）
typedef struct {
    esp_timer_handle_t handle;                                                                      // 底层 esp_timer 句柄
    const char *name;                                                                               // 定时器名称（用于调试打印）
    uint32_t freq_hz;                                                                               // 当前设定的频率 (Hz)
    uint32_t period_ticks;                                                                          // 当前周期的 Tick 总数（内部根据频率计算）
    uint32_t current_tick;                                                                          // 当前周期内的 Tick 计数（从0递增到 period_ticks-1）
    gpio_binding_t *bindings;                                                                       // 指向绑定的 GPIO 数组的指针
    uint8_t bind_count;                                                                             // 绑定的 GPIO 数量
} generic_timer_task_t;

// 批量更新参数结构体
typedef struct {
    gpio_num_t gpio_num;      // 目标引脚
    uint32_t new_duty_percent; // 新的占空比
    uint32_t new_pulse_target; // 新的总周期数 (0 表示无限)
} pin_update_t;

// --- 静态函数声明 ---
static void timer_callback(void *arg);

/**
 * @brief 初始化并绑定 GPIO
 * @param *task : 定时器任务指针
 * @param freq_hz : 定时器输出PWM频率
 * @param *bindings ： 需要绑定的GPIO引脚数组指针
 * @param count : 绑定引脚数量
 * @return 返回值或错误码
 */
return_value framework_timer_create(generic_timer_task_t *task, uint32_t freq_hz, 
    gpio_binding_t *bindings, uint8_t count);
/**
 * @brief 启动定时器
 * @param *task : 定时器任务指针
 */
esp_err_t framework_timer_start(generic_timer_task_t *task) ; 
/**
 * @brief 动态修改频率（注意：会影响到所有绑定了该条定时器的所有GPIO）
 * @param *task : 定时器任务指针
 * @param freq_hz : 定时器输出PWM频率
 * @return 返回值或错误代码
 */
return_value framework_timer_set_freq(generic_timer_task_t *task, uint32_t freq_hz) ;
/**
 * @brief 动态修改单个 GPIO 占空比，不影响其它在同一个定时器上的其他GPIO的占空比
 * @param *task : 定时器任务指针
 * @param gpio_num : GPIO引脚编号
 * @param duty_percent : 占空比
 * @return 返回值或错误代码
 */
return_value framework_pin_set_duty(generic_timer_task_t *task, gpio_num_t gpio_num, 
    uint32_t duty_percent) ;
/**
 * @brief 暂停单个GPIO的PWM输出，不影响其它在同一个定时器上的其他GPIO
 * @param *task : 定时器任务指针
 * @param gpio_num : GPIO引脚编号
 * @param pause_state : 状态枚举
 * @return 返回值或错误码
 */
return_value framework_pin_pause(generic_timer_task_t *task, gpio_num_t gpio_num, 
    pin_state_t pause_state) ;
/**
 * @brief 销毁定时器并处理引脚最终状态
 * @param *task : 定时器任务指针
 * @param final_state : 状态枚举
 * @return 返回值或错误码
 */
return_value framework_timer_destroy(generic_timer_task_t *task, pin_state_t final_state) ;

/**
 * @brief 询问引脚PWM总周期数（目标脉冲数）
 * @param *task : 定时器任务指针
 * @param gpio_num : GPIO引脚编号
 * @return int32_t : 返回目标脉冲数 (>0), 0 表示无限循环, -1 表示引脚未找到
 */
int32_t framework_pin_get_pulse_target(generic_timer_task_t *task, gpio_num_t gpio_num);

/**
 * @brief 询问引脚PWM剩余周期数
 * @param *task : 定时器任务指针
 * @param gpio_num : GPIO引脚编号
 * @return int32_t : 返回剩余脉冲数, 0 表示无限循环或已完成, -1 表示引脚未找到
 */
int32_t framework_pin_get_remaining_pulses(generic_timer_task_t *task, gpio_num_t gpio_num);

/**
 * @brief 询问引脚PWM占空比
 * @param *task : 定时器任务指针
 * @param gpio_num : GPIO引脚编号
 * @return int32_t : 返回占空比 (0-100), -1 表示引脚未找到
 */
int32_t framework_pin_get_duty(generic_timer_task_t *task, gpio_num_t gpio_num);

/**
 * @brief 询问引脚的逻辑状态（如：活跃、暂停低、暂停高）
 * @param *task : 定时器任务指针
 * @param gpio_num : GPIO引脚编号
 * @return pin_state_t : 返回状态枚举，如果未找到引脚，返回 -1
 */
pin_state_t  framework_pin_get_state(generic_timer_task_t *task, gpio_num_t gpio_num);

/**
 * @brief 询问引脚当前的物理电平（高/低）
 * @param *task : 定时器任务指针
 * @param gpio_num : GPIO引脚编号
 * @return pin_state_t
 * @note 注意：此函数仅在 PIN_STATE_ACTIVE 状态下有效，它基于数学推算而非直接读取寄存器。
 */
pin_state_t framework_pin_get_physical_level(generic_timer_task_t *task, gpio_num_t gpio_num);

/**
 * @brief 批量修改多个引脚的占空比和总周期数
 * @param *task : 定时器任务指针
 * @param updates : 指向更新信息数组的指针
 * @param update_count : 数组长度
 * @return return_value : 错误码
 */
return_value framework_batch_update(generic_timer_task_t *task, const pin_update_t *updates, 
                                   uint8_t update_count);
#endif