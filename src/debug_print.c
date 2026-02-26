#include "debug_print.h"
#include "hal_data.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

static volatile bool g_uart7_tx_complete_flag = false;
static volatile bool g_uart7_rx_complete_flag = false;

void uart7_callback(uart_callback_args_t * p_args)
{
    switch (p_args->event)
    {
    case UART_EVENT_TX_COMPLETE:
        g_uart7_tx_complete_flag = true;
        break;
    case UART_EVENT_RX_COMPLETE:
        g_uart7_rx_complete_flag = true;
        break;
    default:
        break;
    }
}

void uart7_print(const char *format, ...)
{
    char buffer[128];

     // 安全检查：避免空指针
    if (format == NULL)
    {
        return;
    }

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    // 获取格式化后的字符串长度
    uint32_t str_len = strlen(buffer);
    if (str_len == 0)
    {
        return;
    }

    /* Transmit the formatted string over UART */
    g_uart7_tx_complete_flag = false;
    R_SCI_UART_Write(&g_uart7_ctrl, (uint8_t *)buffer, str_len);

    while (!g_uart7_tx_complete_flag)
    {
        /* Wait for transmission to complete */
    }
}

// static void uart7_wait_for_tx(void)
// {
//     while (!g_uart7_tx_complete_flag)
//     {
//         /* Wait for transmission to complete */
//     }
// }

// static void uart7_wait_for_rx(void)
// {
//     while (!g_uart7_rx_complete_flag)
//     {
//         /* Wait for reception to complete */
//     }
// }

// int fputc(int ch, FILE * f)
// {
//     (void)f;

//     /* 启动发送字符 */
//     g_uart7.p_api->write(g_uart7.p_ctrl, (uint8_t const * const)&ch, 1);

//     /* 等待发送完毕 */
//     uart7_wait_for_tx();

//     return ch;
// }

// /* 重写这个函数,重定向scanf */
// int fgetc(FILE *f)
// {
//     uint8_t ch;

//     (void)f;

//     /* 启动接收字符 */
//     g_uart7.p_api->read(g_uart7.p_ctrl, &ch, 1);

//     /* 等待接收完毕 */
//     uart7_wait_for_rx();

//     /* 回显 */
//     {
//         fputc(ch, NULL);

//         /* 回车之后加换行 */
//         if (ch == '\r')
//         {
//             fputc('\n', NULL);;
//         }
//     }

//     return (int)ch;
// }
