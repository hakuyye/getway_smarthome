/**
 ****************************************************************************************************
 * @file        bsp_uart.h
 * @author      普中科技
 * @version     V1.0
 * @date        2024-06-05
 * @brief       串口通信实验
 * @license     Copyright (c) 2024-2034, 深圳市普中科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:普中-Hi3861
 * 在线视频:https://space.bilibili.com/2146492485
 * 公司网址:www.prechin.cn
 * 购买地址:
 *
 */

#ifndef BSP_UART_H
#define BSP_UART_H

#define SCAN "AT+SCAN=1\r\n""
#define DEL "AT+DISCONN=1\r\n"
#define CONN "AT+CONN="
#define AUTO_CFG "AT+AUTO_CFG=1\r\n"
#define AUTO_MAC "AT+AUTO_MAC="
#define AUTO_DEL "AT+AUTO_DEL="

#include "cmsis_os2.h"
#include "hi_io.h"
#include "hi_gpio.h"

//管脚定义
#define UART0_TX_PIN            HI_IO_NAME_GPIO_11
#define UART0_TX_GPIO_FUN       HI_IO_FUNC_GPIO_11_UART2_TXD

#define UART0_RX_PIN            HI_IO_NAME_GPIO_12
#define UART0_RX_GPIO_FUN       HI_IO_FUNC_GPIO_12_UART2_RXD


//函数声明
void uart0_init(uint32_t buad);
void uart0_send_data(uint8_t *dat,uint8_t len);
uint32_t uart0_read_data(uint8_t *dat,uint8_t len);

#endif
