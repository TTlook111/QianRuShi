#ifndef __UART1_H__
#define __UART1_H__

void at_uart_init(void);
void at_uart_write(char ch);
void at_uart_set_input_call(void (*c)(char ch));

#endif
