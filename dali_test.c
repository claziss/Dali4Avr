/*
 * dali_test.c
 *
 *  Created on: Feb 12, 2012
 *      Author: eu
 */

#include <avr/interrupt.h>
#include "dali_drv.h"
#include "uart.h"

WORD forward  = 0;                                 // DALI forward frame
BYTE answer   = 0;                                 // DALI slave answer
BYTE f_dalitx = 0;
BYTE f_dalirx = 0;
extern BYTE f_error;
extern WORD f_dbg;
extern BYTE position;

void SetOutReport(BYTE *rep)                       // OutReport received from USB host
{
    forward = (rep[0] << 8) | rep[1];
    f_dalitx = 1;                                  // set DALI send flag
}

unsigned char uart_getchr_blk(void) {
    while(!(UCSR0A & (1<<RXC0))) {}
    return UDR0;
}

void ReadUartCMD(BYTE *rep)
{
  uart_puts("\nEnter a hexadecimal number");
  // Now parse the string and interpret it as a hexadecimal number, terminated by \r
  BYTE nodeID = 0, cnt = 0;
  BYTE result = 0; // Pending;
  while (result != 1)
    {
      BYTE c = uart_getchr_blk();

      if ((c >= '0') && (c <= '9'))
        {
          nodeID = (nodeID << 4) + (c - '0');
        }
      else if ((c >= 'a') && (c <= 'f'))
        {
          nodeID = (nodeID << 4) + (c - 'a' + 0xa);
        }
      else if ((c >= 'A') && (c <= 'F'))
        {
          nodeID = (nodeID << 4) + (c - 'A' + 0xa);
        }
      else if (c == '\r')
        {
          rep[cnt++] = nodeID;
          if (cnt == 2)
            result = 1; // OK
          else
            nodeID = 0;
        }
      else
        {
          uart_puts("\nPlease enter a number in hexadecimal format");
          result = -1; // Error
        }
    }
  uart_puts("\nOK:");
  uart_puthex2(rep[0]);
  uart_puthex2(rep[1]);
  uart_putchr('\n');
  SetOutReport(rep);
  uart_puthex4(forward);
  uart_putchr('\n');
}

#include <util/delay.h>

static inline void delayms(uint16_t millis) __attribute__((always_inline));
static inline void delayms(uint16_t millis) {
  while ( millis ) {
    _delay_ms(1);
    millis--;
  }
}


int main(void)
{
  BYTE cmdDALI[2];

  DALI_Init();
  uart_init();
  DDRC |= 1<<PC1; /* set LED to output */
  PORTC |= 1<<PC1; /* LED off */

  sei(); // Enable all the interrupts
  uart_puts("\nDALI demo online\n");
  while (1)
    {
      ReadUartCMD(cmdDALI);
      if (f_dalitx) // flag set from USB or the DALI module
        {
          f_dalitx = 0; // clear DALI send flag
          f_dalirx = 0; // clear DALI receive (answer) flag
          DALI_Send(); // DALI send data to slave(s)
          //PORTC &= !(1<<PC1); /* LED on */
        }
      delayms(5000);
      if (f_dalirx)
        {
          uart_puts("\n DALI answer:");
          uart_puthex2(answer);
          uart_putchr('\n');
        }
      if (f_error)
        {
          uart_puts("\n DALI error\n");
        }
      uart_puthex4(f_dbg);
      uart_putchr('\n');
      uart_puthex2(position);
      uart_putchr('\n');
      PORTC |= 1<<PC1; /* LED off */
      delayms(5000);

    }
}
