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
        
        // 1.计算基础电平（0或1）
        uint32_t high_ticks = ((uint64_t)task->period_ticks * pin->duty_percent) / 100;             // 根据占空比计算高电平持续的 Tick 数 (例如：周期100，占空比25%，则高电平占25个Tick)
        uint8_t base_level = (task->current_tick < high_ticks) ? 1 : 0;                             // 判断当前 Tick 是否在高电平区间内，决定输出 1(高) 还是 0(低)
        
        // 2.应用相位偏移
        uint8_t final_level = (pin->phase_invert) ? (1 - base_level) : base_level;                  // 如果 phase_invert 为 1 ，输出 ！base_level，否则输出 base_level
        
        if (pin->pulse_target > 0)                                                                  // 当 pulse_target > 0 时，表示开启了脉冲计数模式
        {
            // 仅在每次波形从低电平跳变到高电平时，计数 +1            
            if ((task->current_tick == 0 && base_level == 1) ||                                     // current_tick == 0 并且 当前处于高电平 （处理正相的结束）
                (task->current_tick == 1 && base_level == 0))                                       // current_tick == 1 并且 当前处于低电平 （处理反相的结束）
            {
                pin->pulse_count++;                                                                 // 计数 +1
                
                if (pin->pulse_count >= pin->pulse_target)                                          // 如果达到了目标脉冲数，立即触发暂停机制
                {
                    pin->state = pin->post_state;                                                   // 切换到指定的暂停状态
                    if (pin->state == PIN_STATE_PAUSED_LOW)                                         // 如果设置停止状态为低电平
                        gpio_set_level(pin->gpio_num, 0);                                           // 则设置引脚强制低电平
                    else if (pin->state == PIN_STATE_PAUSED_HIGH)                                   // 如果设置停止状态为高电平
                        gpio_set_level(pin->gpio_num, 1);                                           // 则设置引脚强制高电平
                                                                                                    // 如果是 HOLD 状态，什么都不做，保持当前电平
                    continue;                                                                       // 本次循环直接结束，不再更新电平
                }
            }
        }
        gpio_set_level(pin->gpio_num, final_level);                                                 // 将计算出的电平写入物理 GPIO 寄存器
    }
}

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
    if (!task)
        return ERR_ESP_TIME_TASK_IS_NULL;                                                           // 任务为空
    if (freq_hz < 1  || !bindings || count == 0|| freq_hz > 10000)                                  // 如果频率输入少于1或者大于10000（ESP-IDF 官方文档明确规定最小周期定时器为50us，对应频率为10KHz，所以输入的频率不能大于10Khz
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
        return ERR_ESP_TIME_TASK_IS_NULL;                                                            // 任务为空
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
    if( !task )
        return ERR_ESP_TIME_TASK_IS_NULL;                                                           // 任务为空
    if (freq_hz < 1 || freq_hz > 10000)                                                             // 如果频率输入少于1或者大于10000（ESP-IDF 官方文档明确规定最小周期定时器为50us，对应频率为10KHz，所以输入的频率不能大于10Khz
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
    if (!task)
        return ERR_ESP_TIME_TASK_IS_NULL;                                                           // 任务为空
    if (duty_percent > 100)                                                                         // 占空比大于100
        return ERR_ESP_TIME_SET_DUTY;
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
        return ERR_ESP_TIME_TASK_IS_NULL;                                                           // 返回错误代码
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
        return ERR_ESP_TIME_TASK_IS_NULL;                                                     
    
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

/**
 * @brief 询问引脚PWM总周期数（目标脉冲数）
 * @param *task : 定时器任务指针
 * @param gpio_num : GPIO引脚编号
 * @return int32_t : 返回目标脉冲数 (>0), 0 表示无限循环, -1 表示引脚未找到
 */
int32_t framework_pin_get_pulse_target(generic_timer_task_t *task, gpio_num_t gpio_num) 
{
    if (!task || !task->bindings)                                                                   // 如果定时器与引脚未绑定
        return -1;                                                                                  // 返回未找到
    
    for (uint8_t i = 0; i < task->bind_count; i++)                                                  // 遍历所有绑定的引脚
        if (task->bindings[i].gpio_num == gpio_num)                                                 // 如果绑定的引脚和要查询的引脚相同
            return (int32_t)task->bindings[i].pulse_target;                                         // 则返回该引脚需要输出的总周期数

    return -1;                                                                                      // 遍历完都未找到返回未找到
}

/**
 * @brief 询问引脚PWM剩余周期数
 * @param *task : 定时器任务指针
 * @param gpio_num : GPIO引脚编号
 * @return int32_t : 返回剩余脉冲数, 0 表示无限循环或已完成, -1 表示引脚未找到
 */
int32_t framework_pin_get_remaining_pulses(generic_timer_task_t *task, gpio_num_t gpio_num) 
{
    if (!task || !task->bindings)                                                                   // 如果定时器与引脚未绑定
        return -1;                                                                                  // 返回未找到
    
    for (uint8_t i = 0; i < task->bind_count; i++)                                                  // 遍历所有绑定的引脚
    {
        gpio_binding_t *pin = &task->bindings[i];                                                   // 引脚指针指向该引脚
        if (pin->gpio_num == gpio_num)                                                              // 如果遍历到的引脚是需要询问的引脚
        {
            if (pin->pulse_target == 0 || pin->state != PIN_STATE_ACTIVE)                           // 如果引脚的目标（总）脉冲周期是无限循环或者不在输出脉冲状态
                return 0;                                                                           // 则返回0
            if (pin->pulse_count >= pin->pulse_target)                                              // 如果引脚的当前输出脉冲数≥目标脉冲数
                return 0;                                                                           // 则返回0
            return (int32_t)(pin->pulse_target - pin->pulse_count);                                 // 否则返回剩余周期数
        }
    }
    return -1;                                                                                      // 未找到
}

/**
 * @brief 询问引脚PWM占空比
 * @param *task : 定时器任务指针
 * @param gpio_num : GPIO引脚编号
 * @return int32_t : 返回占空比 (0-100), -1 表示引脚未找到
 */
int32_t framework_pin_get_duty(generic_timer_task_t *task, gpio_num_t gpio_num)
{
    if (!task || !task->bindings)                                                                   // 如果定时器与引脚未绑定
        return -1;                                                                                  // 返回未找到
    
    for (uint8_t i = 0; i < task->bind_count; i++)                                                  // 遍历所有绑定的引脚
        if (task->bindings[i].gpio_num == gpio_num)                                                 // 如果绑定的引脚和要查询的引脚相同
            return (int32_t)task->bindings[i].duty_percent;                                         // 返回询问引脚的占空比
    return -1;                                                                                      // 未找到
}

/**
 * @brief 询问引脚的逻辑状态（如：活跃、暂停低、暂停高）
 * @param *task : 定时器任务指针
 * @param gpio_num : GPIO引脚编号
 * @return pin_state_t : 返回状态枚举，如果未找到引脚，返回 -1
 */
pin_state_t framework_pin_get_state(generic_timer_task_t *task, gpio_num_t gpio_num)
{
    if (!task || !task->bindings)                                                                   // 如果定时器与引脚未绑定
        return -1;                                                                                  // 返回未找到
    
    for (uint8_t i = 0; i < task->bind_count; i++)                                                  // 遍历所有绑定的引脚
        if (task->bindings[i].gpio_num == gpio_num)                                                 // 如果绑定的引脚和要查询的引脚相同
            return task->bindings[i].state;                                                         // 直接返回结构体里记录的状态
    return -1;                                                                                      // 未找到引脚
}

/**
 * @brief 询问引脚当前的物理电平（高/低）
 * @param *task : 定时器任务指针
 * @param gpio_num : GPIO引脚编号
 * @return pin_state_t
 * @note 注意：此函数仅在 PIN_STATE_ACTIVE 状态下有效，它基于数学推算而非直接读取寄存器。
 */
pin_state_t framework_pin_get_physical_level(generic_timer_task_t *task, gpio_num_t gpio_num)
{
    if (!task || !task->bindings)                                                                   // 如果定时器与引脚未绑定
        return PIN_STATE_NOT_INIT;                                                                  // 返回引脚未初始化
    
    for (uint8_t i = 0; i < task->bind_count; i++)                                                  // 遍历所有绑定的引脚
    {
        if (task->bindings[i].gpio_num == gpio_num)                                                 // 如果绑定的引脚和要查询的引脚相同
            return task->bindings[i].state;                                                         // 直接返回结构体里记录的状态
    }
    return PIN_STATE_NOT_INIT;                                                                      // 返回引脚未初始化
}

/**
 * @brief 批量修改多个引脚的占空比和总周期数
 * @param *task : 定时器任务指针
 * @param updates : 指向更新信息数组的指针
 * @param update_count : 数组长度
 * @return return_value : 错误码
 */
return_value framework_batch_update(generic_timer_task_t *task, const pin_update_t *updates, 
                                   uint8_t update_count) 
{
    if (!task || !updates || update_count == 0)
        return ERR_ESP_TIME_TASK_IS_NULL;

    for (uint8_t idx = 0; idx < update_count; idx++)                                                // 遍历传入的数组
    {
        // 1. 找到需要更新的引脚
        const pin_update_t *update = &updates[idx];                                                 // 创建指向需要更新的引脚指针
        gpio_binding_t *target_pin = NULL;

        for (uint8_t i = 0; i < task->bind_count; i++)                                              // 查找对应的引脚
        {
            if (task->bindings[i].gpio_num == update->gpio_num)                                     // 找到需要更新的引脚
            {
                target_pin = &task->bindings[i];                                                    // 指针指向该绑定的引脚
                break;
            }
        }

        if (!target_pin)                                                                            // 如果找不到需要设置的引脚
        {
            ESP_LOGW(TAG, "引脚 %d 未在定时器中找到，跳过。", update->gpio_num);
            continue;
        }

        // 2. 处理占空比更新
        if (update->new_duty_percent <= 100)
        {
            target_pin->duty_percent = update->new_duty_percent;
        }
        else
        {
            ESP_LOGW(TAG, "引脚 %d 所设置的占空比 %f 大于100，跳过此引脚设置。", update->gpio_num, 
                update->new_duty_percent);
        }

        // 3. 处理周期数 (脉冲数) 更新        
        if (update->new_pulse_target == 0)                                                          // 如果新目标为0，表示无限循环
        {
            target_pin->pulse_target = 0;
            if (target_pin->state != PIN_STATE_ACTIVE)                                              // 如果当前是停止状态
            {
                target_pin->state = PIN_STATE_ACTIVE;                                               // 需要恢复为活跃状态才能继续输出
                                                                                                    // 注意：这里不改变物理电平，等待下个回调根据占空比决定电平
            }
        }         
        else 
        {            
            if (target_pin->state != PIN_STATE_ACTIVE)                                              // 如果当前已经停止（即上一轮计数已完成）
            {                
                target_pin->pulse_count = 0;                                                        // 1. 重置计数器                
                target_pin->pulse_target = update->new_pulse_target;                                // 2. 设置新的目标
                target_pin->state = PIN_STATE_ACTIVE;                                               // 3. 恢复活跃状态，重新开始输出
                ESP_LOGD(TAG, "引脚 %d 重置计数器，重新开始输出 %d 个周期", update->gpio_num, 
                    update->new_pulse_target);
            }             
            else                                                                                    // 如果新目标大于当前已计数的数量
            {
                target_pin->pulse_target = update->new_pulse_target;                                // 正常更新目标周期数
                                                                                                    // 状态保持 ACTIVE，无需改变
            }

            // --- 检查是否需要立即停止 ---            
            if (update->new_pulse_target <= target_pin->pulse_count)                                // 如果新输入的周期数小于等于当前已输出的周期数（包括刚重置为0的情况）
            {                
                target_pin->state = target_pin->post_state;                                         // 立即停止输出
                if (target_pin->state == PIN_STATE_PAUSED_LOW)                                      // 如果设置了停止输出低电平
                    gpio_set_level(update->gpio_num, 0);                                            // 立即强制输出低电平
                else if (target_pin->state == PIN_STATE_PAUSED_HIGH)                                // 如果设置了停止输出高电平
                    gpio_set_level(update->gpio_num, 1);                                            // 立即强制输出高电平
                                                                                                    // 如果是 HOLD 状态，则保持原样
                ESP_LOGD(TAG, "引脚 %d 新周期 %d <= 已输出 %d，立即停止", 
                         update->gpio_num, update->new_pulse_target, target_pin->pulse_count);
            }
        }
    }

    return RV_OK;
}