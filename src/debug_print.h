#ifndef __DEBUG_PRINT_H__
#define __DEBUG_PRINT_H__

void uart7_print(const char *format, ...);
#define print(...) uart7_print(__VA_ARGS__) // 定义print宏，重定向到uart7_print函数

#endif
