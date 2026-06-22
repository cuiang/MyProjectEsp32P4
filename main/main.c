//========C语言标准库=================================
#include <stdio.h>                                                                                  // 标准C语言输入输出库

//========FreeRTOS库=================================
#include "freertos/FreeRTOS.h"                                                                      // FreeRTOS库
#include "freertos/task.h"                                                                          // FreeRTOS库：用于任务管理

//========ESP-IDF库==================================
#include "nvs_flash.h"                                                                              // ESP-IDF库：NVS（非易失性储存）系统库
#include "esp_system.h"                                                                             // ESP-IDF库：系统功能库
#include "esp_chip_info.h"                                                                          // ESP-IDF库：芯片信息库
#include "esp_flash.h"                                                                              // ESP-IDF库：闪存操作库
#include "esp_log.h"                                                                                // ESP-IDF库：日志功能库

//========自定义的库==================================
#include "wdt.h"                                                                                    // 看门狗
#include "mcu_gpio_setup.h"                                                                         // GPIO引脚定义
#include "gpio_level_switch.h"                                                                      // GPIO引脚控制
#include "gpio_esp_time_test.h"                                                                     // ESP_TIME测试

// 设置 TAG 的名称：为这份文件的文件名
#define FILENAME __FILE__                                                                           // 获取这个文件的文件名（含路径）
#define TAG (FILENAME + (strlen(FILENAME) - strlen(strrchr(FILENAME, '/' ) + 1)))                   // 把 TAG 设置为当前文件的文件名（不含路径）                                                                        // 用于打印日志时标志自己是哪个文件

/**
 * @brief 主程序入口
 * @param 无
 * @retval 无
 */
void app_main(void)
{
    ESP_LOGD(TAG, "》》》当前执行代码行[%d]，函数%s 。", __LINE__, __FUNCTION__);                      // 进入函数前输出调试日志，得知执行到哪个函数
    
    /* 初始化NVS分区 */
    esp_err_t ret;
    ret = nvs_flash_init();                                                                         // 尝试初始化非易失性存储(NVS)
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)                   // 如果发现NVS分区没有空闲页或版本不匹配，则擦除并重新初始化
    {
        ESP_ERROR_CHECK(nvs_flash_erase());                                                         // 擦除默认的 NVS 分区
        ESP_ERROR_CHECK(nvs_flash_init());                                                          // 初始化默认的 NVS 分区
    }

    /* 获取flash大小 */
    uint32_t flash_size;
    ESP_ERROR_CHECK(esp_flash_get_size(NULL, &flash_size));                                         // 获取闪存大小和芯片信息，存储到相应变量中。

    /* 获取芯片信息 */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);                                                                      // 获取芯片信息

    /* 打印芯片信息、flash大小 */
    ESP_LOGI(TAG, "|---------------------------|");
    ESP_LOGI(TAG, "| %-12s | %-10s |", "describe", "explain");
    ESP_LOGI(TAG, "|--------------|------------|");

    if (chip_info.model == CHIP_ESP32P4)
    {
        ESP_LOGI(TAG, "| %-12s | %-10s |", "model", "ESP32P4");                                     // 芯片型号
    }
    
    ESP_LOGI(TAG, "| %-12s | %-d          |", "cores",      chip_info.cores);                       // 获取芯片的内核数量
    ESP_LOGI(TAG, "| %-12s | %-d          |", "revision",   chip_info.revision);                    // 获取芯片的版本号
    ESP_LOGI(TAG, "| %-12s | %-ld         |", "FLASH size", flash_size / (1024 * 1024));            // 获取FLASH大小(MB)
    ESP_LOGI(TAG, "|---------------------------|");

    ESP_LOGI(TAG, "初始化已完成，程序开始进入主循环"); 
    wdt_init(10000000);                                                                             // 初始化看门狗（10000000微秒=10秒）

    return_value rv;     
    rv = gpio_output_init(LEDB_GPIO_PIN, 0, 0, 1);                                                  // 开发板为低电平亮灯    
    ESP_LOGI(TAG,"已执行LED1初始化,返回代码rv = %X", rv);

    // =============启用ESP_TIME绑定两个LED输出呼吸灯效果
    gpio_esp_time_test_fun();                                                                       // 输出PWM测试

    //================================================
    int8_t main_while_int = 0;                                                                       // 主循环计数器
    while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(10));                                                              // 将毫秒转换为系统节拍，延迟10毫秒
        
        if(main_while_int == 80)
            gpio_toggle(LEDB_GPIO_PIN);                                                             // 蓝灯灭
        if(main_while_int == 160)
        {
            gpio_toggle(LEDB_GPIO_PIN);                                                             // 蓝灯灭
            main_while_int = 0;                                                                     // 主循环计数器清0
        }        
        
        restart_timer(10000000);                                                                    // 喂狗，不然就会重启芯片
    }
}
