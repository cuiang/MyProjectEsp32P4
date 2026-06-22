/**
 ****************************************************************************************************
 * @file        esp_time_framework.c
 * @version     V1.0
 * @date        2026-06-17
 * @brief       ESP_TIME定时器
 ****************************************************************************************************
 */

#include "esp_time_framework.h"

/**
 * @brief 内部统一回调分发器（定时器每次触发时都会执行此函数）
 * @package *arg : 定时器任务结构体的指针
 * @return 无
 */
static void internal_timer_callback(void *arg)
{
    generic_timer_task_t *task = (generic_timer_task_t *)arg;                                       // 将传入的 void 指针强制转换为定时器任务结构体指针
    if (!task || !task->bindings)                                                                   // 安全检查：如果任务或绑定的引脚为空
        return;                                                                                     // 直接返回(回调函数不能带返回值)

    task->current_tick++;                                                                           // 全局 Tick 计数器递增（代表时间推进了一步）
    if (task->current_tick >= task->period_ticks)                                                   // 如果达到了一个完整周期的 Tick 数
        task->current_tick = 0;                                                                     // 则重置为 0（开始新周期）

    // 遍历所有绑定的 GPIO，独立计算占空比
    for (uint8_t i = 0; i < task->bind_count; i++)                                                  // 遍历所有绑定的 GPIO，独立计算并输出占空比
    {
        gpio_binding_t *pin = &task->bindings[i];                                                   // 获取当前遍历到的引脚配置
        
        // 如果处于暂停状态，不更新电平
        if (pin->state != PIN_STATE_ACTIVE)                                                         // 状态机判断：如果该引脚处于暂停状态
            continue;                                                                               // 直接跳过，不更新电平
        
        uint32_t high_ticks = ((uint64_t)task->period_ticks * pin->duty_percent) / 100;             // 根据占空比计算高电平持续的 Tick 数 (例如：周期100，占空比25%，则高电平占25个Tick)
        uint8_t level = (task->current_tick < high_ticks) ? 1 : 0;                                  // 判断当前 Tick 是否在高电平区间内，决定输出 1(高) 还是 0(低)
        
        // 检测脉冲完成条件
        if (pin->pulse_target > 0)                                                                  // 当 pulse_target > 0 时，表示开启了脉冲计数模式
        {
            // 仅在每次波形从低电平跳变到高电平时，计数 +1            
            if (task->current_tick == 0 && level == 1)                                              // current_tick == 0 且当前处于高电平区间
            {
                pin->pulse_count++;                                                                 // 计数 +1
                
                if (pin->pulse_count >= pin->pulse_target)                                          // 如果达到了目标脉冲数，立即触发暂停机制
                {
                    pin->state = pin->post_state;                                                   // 切换到指定的暂停状态
                    
                    // 根据暂停状态立即应用物理电平
                    if (pin->state == PIN_STATE_PAUSED_LOW) 
                        gpio_set_level(pin->gpio_num, 0);
                    else if (pin->state == PIN_STATE_PAUSED_HIGH) 
                        gpio_set_level(pin->gpio_num, 1);
                    // 如果是 HOLD 状态，什么都不做，保持当前电平
                    continue; // 本次循环直接结束，不再更新电平
                }
            }
        }

        gpio_set_level(pin->gpio_num, level);                                                       // 将计算出的电平写入物理 GPIO 寄存器
    }
}

/**
 * @brief 内部统一回调分发器（定时器每次触发时都会执行此函数,每秒触发1000，0000次（1MHZ））
 * @package *arg : 定时器任务结构体的指针
 * @return 无
 *
static void internal_timer_callback(void *arg)
{
    generic_timer_task_t *task = (generic_timer_task_t *)arg;                                       // 将传入的 void 指针强制转换为定时器任务结构体指针
    if (!task || !task->period_ticks == 0)                                                          // 安全检查：如果任务或绑定的引脚为空
        return;                                                                                     // 直接返回(回调函数不能带返回值)

    task->current_tick++;                                                                           // 全局 Tick 计数器递增（代表时间推进了一步）    
    uint64_t current_phase = ((uint64_t)task->current_tick * 100) / task->period_ticks;             // 计算当前时间对应的逻辑相位 (0.0 ~ 1.0 的固定点表示)，使用 64位整数防止溢出
    // ==== 处理停止输出 PWM 的 GPIO 引脚 ====
    if (current_phase >= 100)                                                                       // 如果完成了一个完整周期 (current_phase >= 100)
    {
        task->current_tick = 0;                                                                     // 重置计数器

        // === 周期结束逻辑 ===
        // 检查是否需要停止脉冲输出
        for(uint8_t i = 0; i < task->bind_count; i++)
        {
            gpio_binding_t *pin = &task->bindings[i];
            if (pin->pulse_target > 0 && pin->pulse_count >= pin->pulse_target)                     // 如果设置了目标脉冲数，且已达到，如果是0则一直输出脉冲
            {
                if (pin->post_state == PIN_STATE_PAUSED_LOW)                                        // 如果先前设定的GPIO脚脉冲结束后为强制低电平
                    gpio_set_level(pin->gpio_num, 0);                                               // 则设置为低电平
                else if (pin->post_state == PIN_STATE_PAUSED_HIGH)                                  // 如果先前设置的GPIO脚脉冲结束后为强制高电平
                    gpio_set_level(pin->gpio_num, 1);                                               // 则设置为高电平，否则保持现在的电平
                pin->state = pin->post_state;                                                       // 更新GPIO当前状态为设置的状态
            }
        }
    }

    // ==== 处理所有绑定的 GPIO 引脚，进行 PWM 翻转 ====
    for (uint8_t i = 0; i < task->bind_count; i++)                                                  // 遍历所有绑定的 GPIO，独立计算并输出占空比
    {
        gpio_binding_t *pin = &task->bindings[i];                                                   // 获取当前遍历到的引脚配置
        
        // 如果引脚处于暂停状态，不更新电平
        if (pin->state != PIN_STATE_ACTIVE)                                                         // 状态机判断：如果该引脚处于暂停状态
            continue;                                                                               // 直接跳过，不更新电平

        pin->error_accumulator += pin->duty_percent;                                                // 累加占空比
        
        if (pin->error_accumulator >= 100)                                                          // 判断是否输出高电平
        {
            gpio_set_level(pin->gpio_num, 1);
            pin->error_accumulator -= 100; // 满 100 清零，多余的留到下一次
            
            // 只有当电平变高时，才算一个新的脉冲周期开始
            // 但为了简单，我们可以在每个周期结束时计数（上面已经处理）
            // 或者在这里：当上一次是低，这次是高，才算一个脉冲
            // 我们利用 last_level 来检测上升沿
            if (pin->last_level == 0)
                pin->pulse_count++;
            pin->last_level = 1;
        }
        else
        {
            gpio_set_level(pin->gpio_num, 0);                                                       // 否则设置引脚输出低电平
            pin->last_level = 0;
        }
    }
}
*/
/**
 * @brief 初始化并绑定 GPIO
 * @param *task : 定时器任务指针
 * @param freq_hz : 定时器输出PWM频率
 * @param *bindings ： 需要绑定的GPIO引脚数组指针
 * @param count : 绑定引脚数量
 * @return 返回值或错误码
 */
return_value framework_timer_create(generic_timer_task_t *task, uint32_t freq_hz, 
    gpio_binding_t *bindings, uint8_t count)
{
    if (!task || freq_hz < 1 || freq_hz > 10000 || !bindings || count == 0)                         // 如果频率输入少于1或者大于10000（ESP-IDF 官方文档明确规定最小周期定时器为50us，对应频率为10KHz，所以输入的频率不能大于10Khz
        return ERR_ESP_TIME_CREATE_INVALID_ARG;                                                     // 返回错误码

    memset(task, 0, sizeof(generic_timer_task_t));                                                  // 初始化任务结构体
    task->freq_hz = freq_hz;                                                                        // 定时器频率
    task->bindings = bindings;                                                                      // 需要绑定的GPIO引脚（即输出PWM引脚）
    task->bind_count = count;                                                                       // 绑定引脚的数量
    task->name = "ESP_TIME_TIMER";                                                                  // 默认名称

    // 逻辑 Tick 数 = (底层硬件频率) / (目标频率)
    // 例如：底层 100kHz, 目标 50Hz -> 100000 / 50 = 2000 Ticks
    // 这样，每 2000 个 10us 的中断，构成一个 50Hz 的周期 (20ms)
    uint32_t period_ticks = TIMER_FREQ_HZ / freq_hz;
    if (period_ticks == 0) 
        period_ticks = 1;                                                                           // 防止除0
    task->period_ticks = period_ticks;

    // 为 GPIO 写入状态和脉冲计数器
    for(int i=0; i < count; i++)
    {
        bindings[i].state = PIN_STATE_ACTIVE;                                                       // 当前引脚状态 = 活跃
        bindings[i].pulse_count = 0;                                                                // 当前已输出脉冲计数 = 0
    }


    esp_timer_create_args_t timer_args = 
    {
        .callback = internal_timer_callback,                                                        // 回调函数
        .arg = task,                                                                                // 回调函数传入参数（定时器任务配置结构体）
        .name = task->name,                                                                         // 定时器名称（用于调试打印），如果没有则默认为GenTimer
        .dispatch_method = ESP_TIMER_TASK
    };

    esp_err_t err = esp_timer_create(&timer_args, &task->handle);                                   // 调用 ESP-IDF API 创建定时器
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "[%s] 定时器创建失败: %s", task->name, esp_err_to_name(err));
        return ERR_ESP_TIMER_CREATE_FAIL;
    }

    ESP_LOGD(TAG, "[%s] 创建成功, 频率:%dHz, 周期Ticks:%d", task->name, freq_hz, period_ticks);
    return RV_OK;                                          
}

/**
 * @brief 启动定时器
 * @param *task : 定时器任务指针
 */
esp_err_t framework_timer_start(generic_timer_task_t *task) 
{
    if (!task || !task->handle) 
        return ERR_ESP_TIME_START_INVALID_ARG;
    return esp_timer_start_periodic(task->handle, TIMER_PERIOD);                                     // 底层每50us一次回调，这是由于软件定时器确定的
}

/**
 * @brief 动态修改频率（注意：会影响到所有绑定了该条定时器的所有GPIO）
 * @param *task : 定时器任务指针
 * @param freq_hz : 定时器输出PWM频率
 * @return 返回值或错误代码
 */
return_value framework_timer_set_freq(generic_timer_task_t *task, uint32_t freq_hz) 
{
    if (!task || freq_hz < 1 || freq_hz > 10000)                                                    // 如果频率输入少于1或者大于10000（ESP-IDF 官方文档明确规定最小周期定时器为50us，对应频率为10KHz，所以输入的频率不能大于10Khz
        return ERR_ESP_TIME_SET_FREQ_INVALID_ARG;                                                   // 返回错误代码
    
    uint32_t new_period = TIMER_FREQ_HZ / freq_hz;
    if (new_period == 0) 
        new_period = 1;

    task->freq_hz = freq_hz;                                                                        // 定时器频率
    task->period_ticks = new_period;                                                                // 假设基础时基为 1MHz (1us)
    task->current_tick = 0;                                                                         // 改变频率时重置相位，防止毛刺

    ESP_LOGD(TAG, "[%s] 频率更新为 %dHz (Ticks: %d)", task->name, freq_hz, new_period);
    return RV_OK;
}

/**
 * @brief 动态修改单个 GPIO 占空比，不影响其它在同一个定时器上的其他GPIO的占空比
 * @param *task : 定时器任务指针
 * @param gpio_num : GPIO引脚编号
 * @param duty_percent : 占空比
 * @return 返回值或错误代码
 */
return_value framework_pin_set_duty(generic_timer_task_t *task, gpio_num_t gpio_num, 
    uint32_t duty_percent) 
{
    if (!task || duty_percent > 100)                                                                // 任务为空或者占空比大于100
        return ESP_ERR_INVALID_ARG;
    for (uint8_t i = 0; i < task->bind_count; i++)                                                  // 遍历查找目标引脚
    {
        if (task->bindings[i].gpio_num == gpio_num)                                                 // 找到需要设置的GPIO引脚
        {
            task->bindings[i].duty_percent = duty_percent;                                          // 找到后更新占空比，下一次回调触发时自动生效
            return ESP_OK;
        }
    }
    return ERR_ESP_TIME_PIN_SET_DUTY_PIN_NOT_FOUND;                                                 // 遍历完都找不到对应的引脚，返回错误码
}

/**
 * @brief 暂停单个GPIO的PWM输出，不影响其它在同一个定时器上的其他GPIO
 * @param *task : 定时器任务指针
 * @param gpio_num : GPIO引脚编号
 * @param pause_state : 状态枚举
 * @return 返回值或错误码
 */
return_value framework_pin_pause(generic_timer_task_t *task, gpio_num_t gpio_num, 
    pin_state_t pause_state) 
{
    if (!task)                                                                                      // 如果任务为空
        return ERR_ESP_TIME_PIN_PAUSE_NVALID_ARG;                                                   // 返回错误代码
    for (uint8_t i = 0; i < task->bind_count; i++)                                                  // 遍历查找目标引脚
    {
        if (task->bindings[i].gpio_num == gpio_num)                                                 // 找到需要设置的GPIO引脚
        {
            task->bindings[i].state = pause_state;                                                  // 改变引脚的状态
            
            if (pause_state == PIN_STATE_PAUSED_LOW)                                                // 立即应用暂停电平（不等待下一次回调，实现即时响应）
                gpio_set_level(gpio_num, 0);                                                        // 设置为低电平
            else if (pause_state == PIN_STATE_PAUSED_HIGH)                                          
                gpio_set_level(gpio_num, 1);                                                        // 设置为高电平
            return ESP_OK;
        }
    }
    return ESP_ERR_NOT_FOUND;                                                                       // 遍历完都找不到对应的引脚，返回错误码
}

/**
 * @brief 销毁定时器并处理引脚最终状态
 * @param *task : 定时器任务指针
 * @param final_state : 状态枚举
 * @return 返回值或错误码
 */
return_value framework_timer_destroy(generic_timer_task_t *task, pin_state_t final_state) 
{
    if (!task || !task->handle)                                                                     // 如果任务为空
        return ERR_ESP_TIME_DESTROY_NVALID_ARG;                                                     
    
    esp_timer_stop(task->handle);                                                                   // 停止定时器
    esp_timer_delete(task->handle);                                                                 // 删除定时器
    task->handle = NULL;                                                                            // 清空结构体指针

    for (uint8_t i = 0; i < task->bind_count; i++)                                                  // 遍历所有引脚，根据传入的最终状态设置物理电平
    {
        gpio_num_t pin = task->bindings[i].gpio_num;
        if (final_state == PIN_STATE_PAUSED_LOW) 
            gpio_set_level(pin, 0);
        else if (final_state == PIN_STATE_PAUSED_HIGH) 
            gpio_set_level(pin, 1);
        // 如果是 HOLD 状态，则什么都不做，保持引脚当前电平
        task->bindings[i].state = final_state;                                                      // 设置GPIO脚状态
    }
    return ESP_OK;
}