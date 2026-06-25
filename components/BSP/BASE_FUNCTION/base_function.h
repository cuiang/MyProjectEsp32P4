/**
 ****************************************************************************************************
 * @file        __BASE_FUNCTION_H_.h
 * @version     V1.0
 * @date        2026-06-24
 ****************************************************************************************************
 */

#ifndef __BASE_FUNCTION_H_
#define __BASE_FUNCTION_H_


// 设置 TAG 的名称：为这份文件的文件名
#define FILENAME __FILE__                                                                           // 获取这个文件的文件名（含路径）
#define TAG (FILENAME + (strlen(FILENAME) - strlen(strrchr(FILENAME, '/' ) + 1)))                   // 把 TAG 设置为当前文件的文件名（不含路径）

/**
 * @brief 计算数组的元素个数
 * @param arr 数组名
 * @return 数组元素个数
 */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif