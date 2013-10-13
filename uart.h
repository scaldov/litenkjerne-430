#ifndef __uart_h__
#define __uart_h__

#include "ltkrn.h"

extern void uart_init(int br);
extern void uart_write(char *bfr, int len);
extern int uart_read(char *bfr, int len);
extern int (*uart_rx_callback)(char c);
#endif
