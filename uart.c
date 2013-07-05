#include "uart.h"
#include <avr/io.h>
#include <avr/interrupt.h>

void uart_init(void) {

    PRR &= ~(_BV(PRUSART0));  // Power enable the USART

    //!for Clock of 8MHz, 9600baud  8e2 (use 25 for 19200)
    UBRR0 = 51;
    //UCSR0A = (1<<U2X0);
    UCSR0A &= ~(_BV(RXC0));
    UCSR0B =(1<<RXEN0) | (1<<TXEN0); //enable uart
    UCSR0C =(1<<USBS0)| (3<<UCSZ00) | (1<<UPM01);
}

void uart_putchr(char c) {
    while(!(UCSR0A & (1<<UDRE0)))
        ;	//wait till previous tx is done
    UDR0=c;							//write the byte
}


void uart_puts(const char *s) {
    while((*s != 0)) {
        uart_putchr(*s++);
    }
}

void uart_puthex(unsigned char n)
{
  unsigned char c;
  c = n & 0xf;
  if (c < 10)
    uart_putchr(0x30 + c);
  else
    uart_putchr(0x61 - 10 + c);
}

void uart_puthex2(unsigned char m)
{
  uart_puthex(m>>4);
  uart_puthex(m);
}

void uart_puthex4(unsigned int i)
{
  uart_puthex(i>>12);
  uart_puthex(i>>8);
  uart_puthex(i>>4);
  uart_puthex(i);
}

unsigned char uart_getchr(char *c) {
    unsigned int i=0;
    char *l;

    l = c;
    while(!(UCSR0A & (1<<RXC0))) {
        i++;
        if (i>=40000) {  //10000 * 5inst @8mhz = 6.2ms timeout
            return 0;
        }
    }
    *l = UDR0;
    return 1;
}

unsigned char uart_gets(char *s, unsigned char length) {
    unsigned char i=0;

    for(i=0;i<length;i++) {
        if (!uart_getchr(s++)) {
            return 0;
        }
    }
    return 1;
}
