/* This file has been prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief
 *      Contains prototypes of functions that handle communication with a computer using RS232
 *
 * \par Application note:
 *    
 *      
 * \author
 */
#ifndef UART_H_
#define UART_H_
#include <avr/io.h>

//function protoypes


/*!
 * Function uart_init initializes the uart interface.
 * 
 * @ingroup apiGrp
 */
void uart_init(void);

/**
 * 
 * This function provides an easy way to write a chr at the uart interface.
 * 
 * \param c character to be output
 * 
 * @ingroup apiGIO
 */
void uart_putchr(char c);

/*!
 * This function provides an easy yet inefficient way to print a string to the uart
 *the uC will be waiting for the entire string to be send because of the use of uart_putchr
 * 
 * \param s string to be output
 * 
 * @ingroup apiGIO
 */
void uart_puts(const char *s);

/*!this function will print a 4-bit value as a hexadecimal digit to the uart
 *the 4-bit value is right aligned (bits 0-3), bits 4-7 are ignored, and may contain garbage
 */
void uart_puthex(unsigned char n);

/*!this function will print a 8-bit unsigned value as two hexadecimal digits to the uart
 */
void uart_puthex2(unsigned char m);

/*!this function will print a 16-bit unsigned value as four hexadecimal digits to the uart
 */
void uart_puthex4(unsigned int i);

/*! 
 * This function will wait for a character to be received through the UART and it will then
 * return this character in the location given by the pointer passed
 * It has a time- out of 10000 rounds. If this timeout occurs the routine will return 0. If a 
 * character has been succesfully received it will return 1.
 * 
 * \param c output buffer
 * \return 1 if no time out occured
 * 
 * @ingroup apiGIO
 */
 
unsigned char uart_getchr(char *c);

/*! 
 * This function will call the uart_getchr routine for the amount of times given in the
 * length parameter. It will store them in the array past through the first pointer param
 * if a string of the specified lenght has been succesfully received the routine will return 1
 * if not it will return 0
 * 
 * \param s string to be read from serial interface
 * \param length the length of the string
 * 
 * @ingroup apiGIO
 */
unsigned char uart_gets(char *s, unsigned char length);

#endif /*UART_H_*/
