/********************************************************************
 ;
 ;  DALI master
 ;
 ;  For transmit, this module uses GPIO - P0.28 (DALI send pin)
 ;  DALI forward frame format:
 ;
 ;      | S |        8 address bits         |        8 command bits         | stop  |
 ;      | 1 | 1 | 0 | 0 | 0 | 0 | 0 | 1 | 1 | 0 | 1 | 1 | 1 | 1 | 0 | 0 | 0 |   |   |
 ;
 ;   ---+ +-+ +---+ +-+ +-+ +-+ +-+   +-+ +---+   +-+ +-+ +-+ +---+ +-+ +-+ +------------
 ;      | | | |   | | | | | | | | |   | | |   |   | | | | | | |   | | | | | |
 ;      +-+ +-+   +-+ +-+ +-+ +-+ +---+ +-+   +---+ +-+ +-+ +-+   +-+ +-+ +-+
 ;
 ;      |2TE|2TE|2TE|2TE|2TE|2TE|2TE|2TE|2TE|2TE|2TE|2TE|2TE|2TE|2TE|2TE|2TE|  4TE  |
 ;
 ;
 ;  For receive, this module uses T0-CAP0 input (capture and interrupt on both edges)
 ;  CAP0.0 (P0.30) is connected to P0.29 (to check high / low level by software)
 ;  DALI slave backward frame format:
 ;
 ;                    | S |         8 data bits           | stop  |
 ;                    | 1 | 1 | 0 | 0 | 0 | 0 | 0 | 1 | 1 |   |   |
 ;
 ;    +---------------+ +-+ +---+ +-+ +-+ +-+ +-+   +-+ +-------------
 ;    |               | | | |   | | | | | | | | |   | | |
 ;   -+               +-+ +-+   +-+ +-+ +-+ +-+ +---+ +-+
 ;
 ;    |4 + 7 to 22 TE |2TE|2TE|2TE|2TE|2TE|2TE|2TE|2TE|2TE|  4TE  |
 ;
 ;  2TE = 834 usec (1200 bps)
 ;
 *******************************************************************************************/

#include "dali_drv.h"
#include <avr/interrupt.h>

static int low_time;                               //<! captured puls low time
static int high_time;                              //<! captured puls high time

static BYTE value;                                 //<! used for dali send bit
BYTE position;                              //<! keeps track of sending bit position
static BYTE previous;                              //<! previous received bit
static WORD frame;                                 //<! holds the received slave backward frame
static BYTE f_repeat;                              //<! flag command shall be repeated
static BYTE f_busy;                                //<! flag DALI transfer busy
BYTE f_error;                               //<! flag DALI transfer error
WORD f_dbg;

/*
extern WORD forward;                               //<! DALI forward frame
extern BYTE answer;                                //<! DALI slave answer
extern BYTE f_dalitx;
extern BYTE f_dalirx;
*/

static void
DALI_Shift_Bit(BYTE val)
{
  if (frame & 0x100) // frame full ?
    {
      frame = 0; // yes, ERROR
      f_error = 1;
      //TIMSK &= ~0x20; //stop capture interrupts
    }
  else
    frame = (frame << 1) | val; // shift bit
}

/************************************************************************
 ; DALI_Decode (we only take action at a rising edge)
 ;
 ; Half(prev) Bit   Low Time        High Time      Action     New Half Bit
 ; -------------------------------------------------------------------
 ;     0               0               0          Shift 0         0
 ;     0               0               1          -ERROR-         *
 ;     0               1               0          Shift 0,1       1
 ;     0               1               1          -ERROR-         *
 ;     1               0               0          Shift 1         1
 ;     1               0               1          Shift 0         0
 ;     1               1               0          -ERROR-         *
 ;     1               1               1          Shift 0,1       1
 ;
 ***********************************************************************/
static void
DALI_Decode(void)
{
  BYTE action;

  action = previous << 2;

  if ((high_time > MIN_2TE) && (high_time < MAX_2TE))
    action = action | 1; // high_time = long
  else if (!((high_time > MIN_TE) && (high_time < MAX_TE)))
    {
      frame = 0; // DALI ERROR
      f_error = 1;
      //TIMSK &= ~0x20; //stop capture interrupts
      return;
    }

  if ((low_time > MIN_2TE) && (low_time < MAX_2TE))
    action = action | 2; // low_time = long
  else if (!((low_time > MIN_TE) && (low_time < MAX_TE)))
    {
      frame = 0; // DALI ERROR
      f_error = 1;
      //TIMSK &= ~0x20; //stop capture interrupts
      return;
    }

  switch (action)
    {
  case 0:
    DALI_Shift_Bit(0); // short low, short high, shift 0
    break;
  case 1:
    frame = 0; // short low, long high, ERROR
    f_error = 1;
    //TIMSK &= ~0x20; //stop capture interrupts
    break;
  case 2:
    DALI_Shift_Bit(0); // long low, short high, shift 0,1
    DALI_Shift_Bit(1);
    previous = 1; // new half bit is 1
    break;
  case 3:
    frame = 0; // long low, long high, ERROR
    f_error = 1;
    //TIMSK &= ~0x20; //stop capture interrupts
    break;
  case 4:
    DALI_Shift_Bit(1); // short low, short high, shift 1
    break;
  case 5:
    DALI_Shift_Bit(0); // short low, long high, shift 0
    previous = 0; // new half bit is 0
    break;
  case 6:
    frame = 0; // long low, short high, ERROR
    f_error = 1;
    //TIMSK &= ~0x20; //stop capture interrupts
    break;
  case 7:
    DALI_Shift_Bit(0); // long low, long high, shift 0,1
    DALI_Shift_Bit(1);
    break;
  default:
    break; // invalid
    }
}

/*! \brief  Timer capture interrupt service routine
 *
 */
ISR(TIMER1_CAPT_vect)
{
  RESET_DALI_TIMER();
  f_dbg++;
  if (TCCR1B & _BV(ICES1))
    {
      if (!DALI_RX())
        PORTC &= !(1<<PC1); /* LED on */
    }
  else
    {
      if (DALI_RX())
        PORTC &= !(1<<PC1); /* LED on */
    }

  if (DALI_RX()) // check rising or falling edge
    {
      /* Handle the rising edge*/
      if (frame != 0) // not first pulse ?
        {
          low_time = ICR1; // rising, so capture low time
          DALI_Decode(); // decode received bit
        }
      else
        {
          previous = 1; // first pulse, so shift 1
          DALI_Shift_Bit(1);
        }
      SET_DALI_FALLING_EDGE();  //wait for the next falling edge
    }
  else
    {
      /* Handle the falling edge*/
      high_time = ICR1; // falling, so capture high time
      SET_DALI_RAISING_EDGE(); //wait for the next rising edge
    }
}

/*! \brief  Timer compare interrupt service routine
 *
 *  This interrupt is triggered when the compare
 *  register equals the timer. It increments the
 *  counter and handles the transmission of each
 *  bit.
 */
ISR(TIMER1_COMPA_vect)
{
  // reset timer
  if (value)
    SET_DALI_TX(); // DALI output pin high
  else
    RESET_DALI_TX(); // DALI output pin low

  if (position == 0) // 0TE second half of start bit = 1
    {
      value = 1;
    }
  else if (position < 33) // 1TE - 32TE, so address + command
    {
      value = (forward >> ((32 - position) / 2)) & 1;
      if (position & 1)
        value = !value; // invert if first half of data bit
    }
  else if (position == 33) // 33TE start of stop bit (4TE)
    { // and start of minimum settling time (7TE)
      value = 1;
    }
  else if (position == 44) // 44TE, end of stopbits and settling time
    {
      if (!DALI_RX())
          f_error = 1;
      SET_DALI_TIMER_COMPARE_22TE();  // receive slave answer, timeout of 22TE = 9,174 msec
      SET_DALI_FALLING_EDGE();        //wait for the falling edge next capture
      CLEAR_DALI_PENDING_CAPTURE();   //reset any pending captures
      ENABLE_DALI_CAPT_INTR();        //enable capture interrupts
    }
  else if (position == 45) // end of transfer
    {
      /* stop and reset timer */
      DISABLE_DALI_INTERRUPT();
      DISABLE_DALI_CAPT_INTR();
      STOP_DALI_TIMER();
      RESET_DALI_TIMER();

      if (frame & 0x100) // backward frame (answer) completed ?
        {
          answer = (BYTE) frame; // OK ! save answer
          f_dalirx = 1; // and set flag to signal application
        }
      frame = 0; // reset receive frame
      f_busy = 0; // end of transmission
      if (f_repeat) // repeat forward frame ?
        f_dalitx = 1; // yes, set flag to signal application
    }
  position++;
}

void
DALI_Send(void)
{
  if (f_repeat) // repeat last command ?
    {
      f_repeat = 0;
    }
  else if ((forward & 0xE100) == 0xA100 || (forward & 0xE100) == 0xC100)
    {
      if ((forward & 0xFF00) == INITIALISE || forward == RANDOMISE)
        {
          f_repeat = 1; // special command shall be repeated within 100 ms
        }
    }
  else if ((forward & 0x1FF) >= 0x120 && (forward & 0x1FF) <= 0x180)
    {
      f_repeat = 1; // configuration command shall be repeated within 100 ms
    }

  while (f_busy)
    ; // Wait until dali port is idle

  frame = 0;
  value = 0; // first half of start bit = 0
  position = 0;
  f_error = 0;
  f_busy = 1; // Activate the timer module to transfer
  f_dbg = 0;

  SET_DALI_TIMER_COMPARE_WAIT_ONE();    // ~2400 Hz
  DISABLE_DALI_CAPT_INTR();             // disable capture interrupt
  ENABLE_DALI_INTERRUPT();              // Enable interrupt on match, CTC
  RESET_DALI_TIMER();                   // reset timer
  START_DALI_TIMER();                   // enable timer. I assume the interrupts are enabled
}

void
DALI_Init(void)
{
  CLEAR_DALI_TIMER_ON_COMPARE_MATCH();
  RESET_DALI_TIMER();
  SET_DALI_TIMER_COMPARE_WAIT_ONE();
  f_busy = 0;
  f_error = 0;

  SETUP_DALI_TX_DIR; /* set DALi TX to output */
  SET_DALI_TX();

  /*TODO: set the CAPTURE input*/
  /* CAPTURE pin is set as input*/
  SETUP_DALI_RX_DIR;

}
