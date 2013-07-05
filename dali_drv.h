/*
 * dali_drv.h
 *
 *  Created on: Jan 31, 2012
 *      Author: eu
 */

#ifndef DALI_DRV_H_
#define DALI_DRV_H_
// Global definitions

#define NULL    ((void *)0)
#define FALSE   (0)
#define TRUE    (1)

typedef unsigned char  BYTE;	/* 0     to 255    */
typedef unsigned short WORD;	/* 0     to 2^16-1 */
typedef unsigned long  LONG;	/* 0     to 2^32-1 */
typedef unsigned int   BOOL;

extern WORD forward;
extern BYTE answer;
extern BYTE f_dalitx;
extern BYTE f_dalirx;

extern void DALI_Init(void);
extern void DALI_Send(void);

#define INITIALISE     0xA500                      //<! command for starting initialization mode
#define RANDOMISE      0xA700                      //<! command for generating a random address
#define TE             834/2                       //<! half bit time = 417 usec
#define MIN_TE         TE - 60                     //<! minimum half bit time
#define MAX_TE         TE + 60                     //<! maximum half bit time
#define MIN_2TE        2*TE - 60                   //<! minimum full bit time
#define MAX_2TE        2*TE + 60                   //<! maximum full bit time


#define SET_DALI_TIMER_COMPARE_22TE()      (OCR1A = 9174)               //<! receive slave answer, timeout of 22TE = 9,174 msec
#define SET_DALI_TIMER_COMPARE_WAIT_ONE()  (OCR1A = TE)                 //!< Sets the timer compare register to one period.

#define RESET_DALI_TIMER()              (TCNT1 = 0x00)                  //<! Clear Timer1
#define DISABLE_DALI_INTERRUPT()        (TIMSK1 &= ~(1<< OCIE1A))       //<! Disable Output Compare A Match interrupt
#define ENABLE_DALI_INTERRUPT()         (TIMSK1 |= 1<< OCIE1A)          //<! Enable Output Compare A Match interrupt
#define CLEAR_DALI_TIMER_ON_COMPARE_MATCH()   (TCCR1B |= (1<< WGM12))   //<! Set timer control register to clear timer on compare match (CTC).

#define CLEAR_DALI_PENDING_CAPTURE()    (TIFR1 |= _BV(ICF1))            //<! Reset any pending captures
#define ENABLE_DALI_CAPT_INTR()         (TIMSK1 |= 1<< ICIE1)           //<! Enable capture interrupts
#define DISABLE_DALI_CAPT_INTR()        (TIMSK1 &= ~(1<< ICIE1))        //<! Disable capture interrupts
#define SET_DALI_FALLING_EDGE()         (TCCR1B &= ~(1<< ICES1))        //<! Set capture for the next falling edge
#define SET_DALI_RAISING_EDGE()         (TCCR1B |= (1<< ICES1))         //<! Set capture for the next raising edge

#define START_DALI_TIMER()              (TCCR1B |= 1<< CS11)            //<! Start Timer1: DIV8 prescaling: Fuse internal 8Mhz Clk => ~1Mhz)
/*(0 << CS12) | (0 << CS11) | (1 << CS10))      // prescale /1 */
#define STOP_DALI_TIMER()               (TCCR1B &= ~(1<< CS11))         //<! Stop Timer1

#if defined (__AVR_ATmega644__) || defined (__AVR_ATmega644A__)
/* Set the DALI TX pin as outputs*/
#define SETUP_DALI_TX_DIR  DDRD |= _BV(PD4)

/* Set the DALI RX pins as input */
#define SETUP_DALI_RX_DIR \
{ \
          DDRD &= ~(_BV(PD3)); \
          DDRD &= ~(_BV(PD6)); \
}

#define SET_DALI_TX()       (PORTD |= 1<<PD4)          //<! Set the DALI TX pin high
#define RESET_DALI_TX()     (PORTD &= !(1<<PD4))       //<! Set the DALI TX pin low

#define DALI_RX()           (PIND & (1<<PD3))          //<! Check the DALI RX pin

#endif /*Endif def ATmega644*/

#endif /* DALI_DRV_H_ */
