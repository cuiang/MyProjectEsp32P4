/**
 ****************************************************************************************************
 * @file        __RETURN_VALUDE_.h
 * @version     V1.0
 * @date        2026-06-16
 * @brief       返回值，整个框架的返回值、警告代码、错误代码均在此设置
 ****************************************************************************************************
 */

#ifndef __RETURN_VALUDE_H_
#define __RETURN_VALUDE_H_

// 设置 TAG 的名称：为这份文件的文件名
#define FILENAME __FILE__
#define TAG (FILENAME + (strlen(FILENAME) - strlen(strrchr(FILENAME, '/' ) + 1)))

// 发生异常后跳转到cleanup定位点
#define TRY1(expr) do { \
    int _res = (expr); \
    if (_res != ESP_OK) { \
        ESP_LOGE(TAG, "Error in %s at line %d: %s, ERROR_CODE = %X", __FUNCTION__, __LINE__, #expr, _res); \
        goto cleanup; \
    } \
} while(0)

// 发生异常后输出异常日志，但不继续后面的程序
#define TRY2(expr) do {\
    int _res = (expr); \
    if (_res != ESP_OK) { \
        ESP_LOGE(TAG, "Error in %s at line %d: %s, Error Code = %X", __FUNCTION__, __LINE__, #expr, _res); \
    } \
} while(0)

typedef int return_value;

#define RV_OK                                   0                                           // 返回正常完成代码
#define RV_FAIL                                 -1                                          // 返回未知错误代码

/* 框架相关返回代码 */ 
// 0xF0：框架正常代码
// 0xD0：框架警告代码
// 0xE0：框架错误代码

/* GPIO相关返回代码 */ 
// 0xF1：GPIO正常代码
// 0xD1：GPIO警告代码
#define WARN_GPIO_INIT_SUCC_SET_LEVEL_FAILURE       0xD101                                  /*!< GPIO引脚初始化成功，但设置电平失败 */
// 0xE1：GPIO错误代码
#define ERR_GPIO_PULLUP_EXCESSIVE                   0xE101                                  /*!< 设置引脚上拉电阻范围超限，应为0或1 */
#define ERR_GPIO_PULLDOWN_EXCESSIVE                 0xE102                                  /*!< 设置引脚下拉电阻范围超限，应为0或1 */
#define ERR_GPIO_INIT_FAILURE                       0xE103                                  /*!< GPIO引脚初始化失败 */
#define ERR_GPIO_SET_LEVEL_FAILURE                  0xE104                                  /*!< GPIO引脚设置电平失败 */

/* ESP_TIME相关返回码 */
// 0xF2：ESP_TIME正常返回码
// 0xD2：ESP_TIME警告代码
// 0xE2：ESP_TIME错误代码
#define ERR_ESP_TIME_CREATE_INVALID_ARG             0xE201                                  /*!< ESP_TIME定时器绑定GPIO函数传入参数无效 */
#define ERR_ESP_TIME_SET_FREQ_INVALID_ARG           0xE202                                  /*!< ESP_TIME定时器修改频率函数传入参数无效，参数值超限 */
#define ERR_ESP_TIME_PIN_SET_DUTY_PIN_NOT_FOUND     0xE203                                  /*!< ESP_TIME定时器修改占空比，传入GPIO引脚编号无效（非绑定的引脚） */
#define ERR_ESP_TIME_TASK_IS_NULL                   0xE204                                  /*!< ESP_TIME定时器传入参数无效，任务为空 */
#define ERR_ESP_TIME_PIN_PAUSE_PIN_NOT_FOUND        0xE205                                  /*!< ESP_TIME定时器暂停引脚PWM输出函数传入GPIO引脚编号无效（非绑定的引脚） */
#define ERR_ESP_TIME_SET_DUTY                       0xE206                                  /*!< ESP_TIME定时器设置占空比函数传入参数无效，参数值超限 */
#define ERR_ESP_TIMER_CREATE_FAIL                   0xE207                                  /*!< ESP_TIME定时器创建失败 */

#endif