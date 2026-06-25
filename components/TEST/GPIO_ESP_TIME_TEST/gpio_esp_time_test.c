/**
 ****************************************************************************************************
 * @file        gpio_level_switch.c
 * @version     V1.0
 * @date        2026-06-16
 * @brief       GPIO引脚电平控制
 ****************************************************************************************************
 */

#include "gpio_esp_time_test.h"

/**
 * @brief 输出PWM测试
 * @param  无
 * @return 无
 */
void gpio_esp_time_test_fun(void)
{
    return_value rv; 
    rv = gpio_output_init(LEDG_GPIO_PIN, 0, 0, 1);                                                  // 开发板为低电平亮灯,设置高电平关闭LED
    ESP_LOGI(TAG,"已执行LED0初始化,返回代码rv = %X", rv);
    rv = gpio_output_init(PWM16_GPIO_PIN, 0, 0, 0);                                                 // 初始化16引脚，设置为低电平
    ESP_LOGI(TAG,"已执行GPIO16初始化,返回代码rv = %X", rv);
    rv = gpio_output_init(PWM17_GPIO_PIN, 0, 0, 0);                                                 // 初始化17引脚，设置为低电平
    ESP_LOGI(TAG,"已执行GPIO17初始化,返回代码rv = %X", rv);
    rv = gpio_output_init(PWM18_GPIO_PIN, 0, 0, 1);                                                 // 初始化18引脚，设置为低电平
    ESP_LOGI(TAG,"已执行GPIO18初始化,返回代码rv = %X", rv);

    static gpio_binding_t led_pin[] = {
        {
            .gpio_num = LEDG_GPIO_PIN,                                                              // 绑定绿色灯引脚
            .duty_percent = 50,                                                                     // 初始化占空比50%
            .phase_invert = 0,                                                                      // 相位状态（正相 默认）
            .state = PIN_STATE_ACTIVE,                                                              // 初始状态为活跃，开始后立即输出PWM
            .pulse_target = 10000,                                                                  // 设定目标为10000个脉冲
            .post_state = PIN_STATE_PAUSED_HOLD                                                     // 达到目标脉冲后强制输出低电平
        },
        { 
            .gpio_num = PWM16_GPIO_PIN,                                                             // 绑定16引脚
            .duty_percent = 50,                                                                     // 初始占空比设为 25%
            .state = PIN_STATE_ACTIVE,                                                              // 初始状态为“活跃”
            .pulse_target = 0,                                                                      // 【核心特性】设为 0，表示无限输出，不受脉冲计数限制
            .post_state = PIN_STATE_PAUSED_HIGH                                                     // 达到目标脉冲后强制高电平
        },
        { 
            .gpio_num = PWM17_GPIO_PIN,                                                             // 绑定17引脚
            .duty_percent = 50,                                                                     // 初始占空比设为 75%
            .phase_invert = 0,                                                                      // 相位状态（正相 默认）
            .state = PIN_STATE_ACTIVE,                                                              // 初始状态为“活跃”
            .pulse_target = 10000,                                                                  // 设为 10000，表示输出1000个脉冲
            .post_state = PIN_STATE_PAUSED_HOLD                                                     // 达到目标脉冲后保持原电平状态
        },
        { 
            .gpio_num = PWM18_GPIO_PIN,                                                             // 绑定18引脚
            .duty_percent = 50,                                                                     // 初始占空比设为 50%
            .phase_invert = 1,                                                                      // 相位状态（反相）
            .state = PIN_STATE_ACTIVE,                                                              // 初始状态为“活跃”
            .pulse_target = 10000,                                                                  // 设为 10000，表示输出1000个脉冲
            .post_state = PIN_STATE_PAUSED_HIGH                                                     // 达到目标脉冲后强制高电平
        }
    };
    static generic_timer_task_t pwm_timer;                                                          // 在栈上或静态区分配一个定时器任务结构体
    pwm_timer.name = "MultiPWM_Task";                                                               // 给定时器起个名字，方便在日志中追踪    
    rv = framework_timer_create(&pwm_timer, 10, led_pin, ARRAY_SIZE(led_pin));                      // 基础频率 10 Hz (即 1ms 一个周期)
    if (rv != RV_OK) {
        ESP_LOGE(TAG, "定时器创建失败，错误码: %d", remove);
        return;
    }
    
    ESP_LOGI(TAG, "定时器创建成功，等待启动指令...");
    vTaskDelay(pdMS_TO_TICKS(2000)); 
    ESP_LOGI(TAG, "系统初始化完毕，手动启动定时器！");

    TRY2(framework_timer_start(&pwm_timer));                                                        // 调用框架的启动函数，此时底层 esp_timer 开始以 1us 的节拍触发回调
    
    //ESP_LOGI(TAG, "动态修改 GPIO33 占空比为 50%");
    // 在回调函数外部安全地修改内存变量，下一次回调触发时自动生效
    //framework_pin_set_duty(&pwm_timer, PWM16_GPIO_PIN, 50);                                         // 设置 GPIO33 引脚输出的PWM为50%占空比
    //framework_pin_set_duty(&pwm_timer, PWM17_GPIO_PIN, 50);                                         // 设置 GPIO34 引脚输出的PWM为50%占空比

    // 模拟 3 秒后，动态改变整体输出频率
    vTaskDelay(pdMS_TO_TICKS(3000));
    ESP_LOGI(TAG, "将所有引脚的频率从 10Hz 提升到 100Hz");
    TRY2(framework_timer_set_freq(&pwm_timer, 100));                                                // 把频率设置为 100HZ
    restart_timer(10000000);                                                                        // 喂狗，不然就会重启芯片
    
    // 模拟 3 秒后，动态改变整体输出频率
    vTaskDelay(pdMS_TO_TICKS(3000));
    ESP_LOGI(TAG, "将所有引脚的频率从 100Hz 提升到 1kHz");
    TRY2(framework_timer_set_freq(&pwm_timer, 1000));

    // 模拟 3 秒后，动态改变整体输出频率
    vTaskDelay(pdMS_TO_TICKS(3000));
    ESP_LOGI(TAG, "将所有引脚的频率从 1kHz 提升到 5kHz");
    TRY2(framework_timer_set_freq(&pwm_timer, 5000));
    restart_timer(10000000);                                                                        // 喂狗，不然就会重启芯片

    // 模拟 3 秒后，动态改变整体输出频率
    vTaskDelay(pdMS_TO_TICKS(3000));
    ESP_LOGI(TAG, "将所有引脚的频率从 5kHz 提升到 10kHz");
    TRY2(framework_timer_set_freq(&pwm_timer, 10000));

    // 模拟 3 秒后，改变占空比和输入新的脉冲目标数，并降低到5KHZ，因为要设置25%和75%占空比测试
    vTaskDelay(pdMS_TO_TICKS(3000));
    TRY2(framework_timer_set_freq(&pwm_timer, 5000));
    ESP_LOGI(TAG, "将所有引脚的频率从 10kHz 提升到 5kHz,改变 GPIO33 为 25 占空比，改变 GPIO34 为 75 占空比");
    static pin_update_t pin_update[] = {
        {
            .gpio_num = PWM16_GPIO_PIN,
            .new_duty_percent = 25,
            .new_pulse_target = 0
        },
        {
            .gpio_num = PWM17_GPIO_PIN,
            .new_duty_percent = 75,
            .new_pulse_target = 10000
        },
        {
            .gpio_num = PWM18_GPIO_PIN,
            .new_duty_percent = 75,
            .new_pulse_target = 10000
        }
    };
    TRY2(framework_batch_update(&pwm_timer, pin_update, ARRAY_SIZE(pin_update)));
    
}