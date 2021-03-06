#include "msp430g2553.h"
#include "ltkrn.h"
#include "uart.h"
#include "kout.h"

#define OS_THREAD_STACK 0x30
#define MAIN_THREAD_STACK 0x30

#define main_thread_stack (void*)(KRN_STACKFRAME - 1 * MAIN_THREAD_STACK)
#define btn_thread_stack  (void*)(KRN_STACKFRAME - 2 * MAIN_THREAD_STACK)
#define io_thread_stack   (void*)(KRN_STACKFRAME - 3 * MAIN_THREAD_STACK)

static krn_thread thr_main, thr_btn, thr_io;

void krn_uthread_idle (void)
{
	int16_t sys_timeout;
	sys_timeout = krn_timer_cnt + KRN_FREQ * 10;
	while(1){
		if(sys_timeout >  krn_timer_cnt) {
			_BIS_SR(LPM0_bits + GIE);
		} else {
			P1OUT |= BIT6;
			DCOCTL  = CALDCO_1MHZ;
			_BIS_SR(LPM4_bits + GIE);
			DCOCTL  = CALDCO_16MHZ;
			sys_timeout = krn_timer_cnt + KRN_FREQ * 10;
		}
	}
}

static NO_REG_SAVE void main_thread_func (void)
{
	while(1){
		krn_sleep(50);
		P1OUT ^= BIT0;
		uart_write("Hello world!\n", 14);
	}
}

static NO_REG_SAVE void btn_thread_func (void)
{
	char str[16];
	while(1){
		uart_write("Hallo vaarlden!\n", 16);
		krn_sleep(100);
		//P1OUT ^= BIT6;
	    kout_uart("sec=");
		kout_uart(kout_u32d(str + 12, krn_timer_cnt / KRN_FREQ));
		kout_uart("\n");
	}
}

//EOL filter
int rx_eol(char c) {
	return c < ' ' ? 0 : 1;
}

static NO_REG_SAVE void io_thread_func (void)
{
	char bfr[16];
	char str[16];
	int l;
	uart_rx_callback = rx_eol;
	while(1) {
		l = uart_read(bfr, 16);
		kout_uart("\t\tbfr=");
		uart_write(bfr, l);
		kout_uart("\n");
	    kout_uart("len=");
		kout_uart(kout_u32d(str, l));
		kout_uart("\n");
	}
}

void main (void){
	int i, j, k;
	krn_tmp_stack();
	WDTCTL = WDTPW + WDTHOLD; // Kill watchdog timer
	BCSCTL1 = CALBC1_16MHZ;
	DCOCTL  = CALDCO_16MHZ;
	BCSCTL2  = SELM_0 | DIVM_0 | DIVS_0;
	TACCTL1 = OUTMOD_7; // TACCR1 reset/set
	TACCTL0 = CCIE;
	TACTL = TASSEL_2 + MC_1 + ID_3; // SMCLK, upmode
	TACCR0 = 2000000 / 50 - 1; // PWM Period
	//TACCR1 = 2000; // TACCR1 PWM Duty Cycle
	TACCR1 = 2000000 / 100; // TACCR1 PWM Duty Cycle
	P1DIR |= BIT6 + BIT0;
	//P1SEL |= BIT6;
	P1OUT |= BIT0;
	P1OUT ^= BIT0;
	//button interrupts
	P1IE |= BIT3;                             // P1.3 interrupt enabled
	P1IES |= BIT3;                            // P1.3 Hi/lo edge
	P1IFG &= ~BIT3;                           // P1.3 IFG cleared
	// create threads
	krn_thread_init();
	krn_thread_create(&thr_main, (void*)main_thread_func, (void*)1, 1, main_thread_stack, MAIN_THREAD_STACK);
	krn_thread_create(&thr_btn, (void*)btn_thread_func, (void*)2, 6, btn_thread_stack, MAIN_THREAD_STACK);
	krn_thread_create(&thr_io, (void*)io_thread_func, (void*)3, 1, io_thread_stack, MAIN_THREAD_STACK);
	krn_timer_init(); // start base timer
	uart_init(16000000 / 115200); // init UART
	krn_run(); // run threads
}

#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
{
	if((P1IFG & BIT3) == BIT3) {
		P1OUT &= ~BIT6;
		_BIC_SR_IRQ(LPM4_bits);
	} else {
	}
	P1IFG = 0x00;   // clear interrupt flags
}
