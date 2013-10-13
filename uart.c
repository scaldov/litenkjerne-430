#include "uart.h"

static krn_mutex uart_mutex_tx;
static krn_mutex uart_mutex_rx;
krn_thread *uart_sleep_thread_tx;
krn_thread *uart_sleep_thread_rx;
volatile char* uart_tx_bfr;
int (*uart_rx_callback)(char c);
volatile int uart_tx_len;
volatile char* uart_rx_bfr;
volatile int uart_rx_len;
volatile int16_t uart_rx_wait_timeout;



void uart_init(int br)
{
	P1SEL  = BIT1 + BIT2;
	P1SEL2 = BIT1 + BIT2;
	UCA0CTL1 |= UCSSEL_2;
	UCA0BR0 = br;
	UCA0BR1 = 0;
	UCA0MCTL = UCBRS0;
	UCA0CTL1 &= ~UCSWRST;
	uart_sleep_thread_tx = 0;
	uart_sleep_thread_rx = 0;
	uart_rx_callback = 0;
	krn_mutex_init(&uart_mutex_tx);
	krn_mutex_init(&uart_mutex_rx);
	IE2 &= ~(UCA0TXIE | UCA0RXIE);
	uart_rx_wait_timeout = 100 * 20;
}

void uart_write(char *bfr, int len)
{
	CRITICAL_STORE;
	if(!len) return;
	krn_mutex_lock(&uart_mutex_tx);
	CRITICAL_START();
	uart_tx_bfr = bfr;
	uart_tx_len = len;
	IE2 |= UCA0TXIE;
	uart_sleep_thread_tx = krn_thread_current;
	krn_thread_lock(uart_sleep_thread_tx);
	krn_dispatch();
	CRITICAL_END();
	krn_mutex_unlock(&uart_mutex_tx);
}

int uart_read(char *bfr, int len)
{
	CRITICAL_STORE;
	if(!len) return 0;
	krn_mutex_lock(&uart_mutex_rx);
	CRITICAL_START();
	uart_rx_bfr = bfr;
	uart_rx_len = len;
	IE2 |= UCA0RXIE;
	uart_sleep_thread_rx = krn_thread_current;
	//krn_thread_lock(uart_sleep_thread_rx);
	//krn_dispatch();
	krn_sleep(uart_rx_wait_timeout);
    uart_sleep_thread_rx = 0;
	len = len - uart_rx_len;
	CRITICAL_END();
	krn_mutex_unlock(&uart_mutex_rx);
	return len;
}

#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void) {
	char c;
	if(IFG2 & UCA0RXIFG) {
		c = *uart_rx_bfr++ = UCA0RXBUF;
		uart_rx_len--;
		if(!(uart_rx_len && (uart_rx_callback ? uart_rx_callback(c) : 1))) {
			IE2 &= ~UCA0RXIE;
			if(uart_sleep_thread_rx) {
				//krn_thread_unlock(uart_sleep_thread_rx);
				krn_thread_wake(uart_sleep_thread_rx);
				krn_thread_move(uart_sleep_thread_rx, krn_thread_current);
				uart_sleep_thread_rx = 0;
				//krn_dispatch(); //uncomment for extra hardness
			}
		}
	}
}

#pragma vector=USCIAB0TX_VECTOR
__interrupt void USCI0TX_ISR(void) {
	if(IFG2 & UCA0TXIFG) {
		UCA0TXBUF = *uart_tx_bfr++;
		uart_tx_len--;
		if(!uart_tx_len) {
			IE2 &= ~UCA0TXIE;
			if(uart_sleep_thread_tx) {
				krn_thread_unlock(uart_sleep_thread_tx);
				krn_thread_move(uart_sleep_thread_tx, krn_thread_current);
				uart_sleep_thread_tx = 0;
				//krn_dispatch(); //uncomment for extra hardness
			}
		}
	}
}

