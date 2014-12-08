/**
 * @file Cosa/Board/LowPowerLab/Moteino_mega.hh
 * @version 1.0
 *
 * @section License
 * Copyright (C) 2014, Mikael Patel
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * This file is part of the Arduino Che Cosa project.
 */

#ifndef COSA_BOARD_LOWPOWERLAB_MOTEINO_MEGA_HH
#define COSA_BOARD_LOWPOWERLAB_MOTEINO_MEGA_HH

/**
 * Compiler warning on unused varable.
 */
#if !defined(UNUSED)
#define UNUSED(x) (void) (x)
#endif

/**
 * Cosa pin symbol and hardware definitions for the ATmega1284P based
 * board Lowpowerlab Moteino Mega. Cosa does not use pin numbers as
 * Arduino/Wiring, instead strong data type is used (enum types) for
 * the specific pin classes; DigitalPin, AnalogPin, PWMPin, etc. 
 *
 * The static inline functions, SFR, BIT and UART, rely on compiler
 * optimizations to be reduced.  
 *
 * @section Board
 * @code
 *                         LowPowerLab Moteino Mega
 *
 *                           +-------------------+
 *             AREF      AR  | []           * [] |  23      D23/PC7
 *       PA7/D31/A7      A7  | []             [] |  22      D22/PC6
 *       PA6/D30/A6      A6  | []             [] |  21      D21/PC5/TDI
 *       PA5/D29/A5      A5  | []             [] |  20      D20/PC4/TDO
 *       PA4/D28/A4      A4  | []             [] |  19      D19/PC3/TMS
 *       PA3/D27/A3      A3  | []             [] |  18      D18/PC2/TCK
 *       PA2/D26/A2      A2  | []             [] |  17 SDA  D17/PC1/SDA
 *       PA1/D25/A1      A1  | []             [] |  16 SCL  D16/PC0/SCL
 *       PA0/D24/A0      A0  | []           * [] |  15 LED  D15/PD7/PWM7
 *           PB0/D0       0  | []             [] |  14      D14/PD6/PWM6
 *           PB1/D1       1  | []             [] |  13      D13/PD5/PWM5
 *      INT2/PB2/D2    i2 2  | [] *           [] |  12      D12/PD4/PWM4
 *      PWM0/PB3/D3       3  | []         S1> [] |  11 i1   D11/PD3/TX1/INT1
 *      PWM1/PB4/D4    SS 4  | [] *       S1< [] |  10 i0   D10/PD2/RX1/INT0
 *      MOSI/PB5/D5    MO 5  | [] *       S0> [] |  9       D9/PD1/TX0
 * PWM2/MISO/PB6/D6    MI 6  | [] *       S1< [] |  8       D8/PD0/RX0
 *  PWM3/SCK/PB7/D7   SCK 7  | [] *           [] |  RST     RESET
 *             3.3V    VOUT  | []             [] |  VOUT    3.3V
 *          3.3-16V     VIN  | []             [] |  VIN     3.3-16V
 *                      GND  | [] o-o-o-o-o-o [] |  GND
 *                           +-------------------+
 *                                | | | | | |
 *                   DTR/RTS -----^ | | | | |
 *                        TX -------^ | | | |
 *                        RX ---------^ | | |
 *                       VIN -----------^ | |
 *                           -------------^ |
 *                       GND ---------------^
 *
 * * = shared by radio if present
 * @endcode
 */
class Board {
  friend class Pin;
  friend class UART;
private:
  /**
   * Do not allow instances. This is a static singleton; name space.
   */
  Board() {}

  /**
   * Return Special Function Register for given Arduino pin number.
   * @param[in] pin number.
   * @return special register pointer.
   */
  static volatile uint8_t* SFR(uint8_t pin)
    __attribute__((always_inline))
  { 
    return (pin < 8  ? &PINB : 
            pin < 16 ? &PIND : 
            pin < 24 ? &PINC :
                       &PINA);
  }

  /**
   * Return bit position for given Arduino pin number in Special
   * Function Register. 
   * @param[in] pin number.
   * @return pin bit position.
   */
  static uint8_t BIT(uint8_t pin)
    __attribute__((always_inline))
  {
    return (pin & 0x7);
  }
  
  /**
   * Return Pin Change Mask Register for given Arduino pin number.
   * @param[in] pin number.
   * @return pin change mask register pointer.
   */
  static volatile uint8_t* PCIMR(uint8_t pin)
    __attribute__((always_inline))
  { 
    return (pin < 8  ? &PCMSK1 : 
            pin < 16 ? &PCMSK3 : 
            pin < 24 ? &PCMSK2 :
                       &PCMSK0);
  }

  /**
   * Return UART Register for given Arduino serial port.
   * @param[in] port number.
   * @return UART register pointer.
   */
  static volatile uint8_t* UART(uint8_t port)
    __attribute__((always_inline))
  { 
    return (port == 1 ? &UCSR1A : &UCSR0A);
  }

public:
  /**
   * Digital pin symbols
   */
  enum DigitalPin {
    D0 = 0,                     // PB0
    D1,                         // PB1
    D2,                         // PB2
    D3,                         // PB3
    D4,                         // PB4
    D5,                         // PB5
    D6,                         // PB6
    D7,                         // PB7
    D8,                         // PD0
    D9,                         // PD1
    D10,                        // PD2
    D11,                        // PD3
    D12,                        // PD4
    D13,                        // PD5
    D14,                        // PD6
    D15,                        // PD7
    D16,                        // PC0
    D17,                        // PC1
    D18,                        // PC2
    D19,                        // PC3
    D20,                        // PC4
    D21,                        // PC5
    D22,                        // PC6
    D23,                        // PC7
    D24,                        // PA0
    D25,                        // PA1
    D26,                        // PA2
    D27,                        // PA3
    D28,                        // PA4
    D29,                        // PA5
    D30,                        // PA6
    D31,                        // PA7
    LED = D15
  } __attribute__((packed));

  /**
   * Analog pin symbols (ADC channel numbers)
   */
  enum AnalogPin {
    A0 = 0,                     // PA0
    A1,                         // PA1
    A2,                         // PA2
    A3,                         // PA3
    A4,                         // PA4
    A5,                         // PA5
    A6,                         // PA6
    A7                          // PA7
  } __attribute__((packed));

  /**
   * Reference voltage; ARef pin, Vcc or internal 1V1.
   */
  enum Reference {
    APIN_REFERENCE = 0,
    AVCC_REFERENCE = _BV(REFS0),
    A1V1_REFERENCE = _BV(REFS1),
    A2V56_REFERENCE = (_BV(REFS1) | _BV(REFS0))
  } __attribute__((packed));

  /**
   * PWM pin symbols; sub-set of digital pins to allow compile 
   * time checking
   */
  enum PWMPin {
    PWM0 = D3,                  // PB3 => OCR0A
    PWM1 = D4,                  // PB4 => OCR0B
    PWM2 = D6,                  // PB6 => OCR3A
    PWM3 = D7,                  // PB7 => OCR3B
    PWM4 = D12,                 // PD4 => OCR1B
    PWM5 = D13,                 // PD5 => OCR1A
    PWM6 = D14,                 // PD6 => OCR2B
    PWM7 = D15                  // PD7 => OCR2A
  } __attribute__((packed));

  /**
   * External interrupt pin symbols; sub-set of digital pins 
   * to allow compile time checking.
   */
  enum ExternalInterruptPin {
    EXT0 = D10,                 // PD2
    EXT1 = D11,                 // PD3
    EXT2 = D2                   // PB2
  } __attribute__((packed));

  /**
   * Pin change interrupt. Number of port registers.
   */
  enum InterruptPin {
    PCI0 = D24,                 // PA0
    PCI1 = D25,                 // PA1
    PCI2 = D26,                 // PA2
    PCI3 = D27,                 // PA3
    PCI4 = D28,                 // PA4
    PCI5 = D29,                 // PA5
    PCI6 = D30,                 // PA6
    PCI7 = D31,                 // PA7
    PCI8 = D0,                  // PB0
    PCI9 = D1,                  // PB1
    PCI10 = D2,                 // PB2
    PCI11 = D3,                 // PB3
    PCI12 = D4,                 // PB4
    PCI13 = D5,                 // PB5
    PCI14 = D6,                 // PB6
    PCI15 = D7,                 // PB7
    PCI16 = D16,                // PC0
    PCI17 = D17,                // PC1
    PCI18 = D18,                // PC2
    PCI19 = D19,                // PC3
    PCI20 = D20,                // PC4
    PCI21 = D21,                // PC5
    PCI22 = D22,                // PC6
    PCI23 = D23,                // PC7
    PCI24 = D8,                 // PD0
    PCI25 = D9,                 // PD1
    PCI26 = D10,                // PD2
    PCI27 = D11,                // PD3
    PCI28 = D12,                // PD4
    PCI29 = D13,                // PD5
    PCI30 = D14,                // PD6
    PCI31 = D15                 // PD7
  } __attribute__((packed));

  /** 
   * Size of pin maps.
   */
  enum {
    ANALOG_PIN_MAX = 8,
    DIGITAL_PIN_MAX = 32,
    EXT_PIN_MAX = 3,
    PCI_PIN_MAX = 32,
    PWM_PIN_MAX = 8
  };

  /**
   * Pins used for TWI interface (in port C, bit 0-1, D16-D17)
   */
  enum TWIPin {
    SDA = 1,                    // PC1/D17
    SCL = 0                     // PC0/D16
  } __attribute__((packed));

 /**
   * Pins used for SPI interface (in port B, bit 4-7, D4-D7)
   */
  enum SPIPin {
    SS = 4,                     // PB4/D4
    MOSI = 5,                   // PB5/D5
    MISO = 6,                   // PB6/D6
    SCK = 7                     // PB7/D7
  } __attribute__((packed));

  /**
   * Auxiliary
   */
  enum {
    VBG = (_BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1)),
    UART_MAX = 2,
    EXT_MAX = 3,
    PCMSK_MAX = 4,
    PCINT_MAX = 32
  } __attribute__((packed));
};

/**
 * Redefinition of symbols to allow generic code.
 */
#define USART_UDRE_vect USART0_UDRE_vect
#define USART_RX_vect USART0_RX_vect 
#define USART_TX_vect USART0_TX_vect

/**
 * Forward declare interrupt service routines to allow them as friends.
 */
extern "C" {
  void ADC_vect(void) __attribute__ ((signal));
  void ANALOG_COMP_vect(void) __attribute__ ((signal));
  void INT0_vect(void) __attribute__ ((signal));
  void INT1_vect(void) __attribute__ ((signal));
  void INT2_vect(void) __attribute__ ((signal));
  void PCINT0_vect(void) __attribute__ ((signal));
  void PCINT1_vect(void) __attribute__ ((signal));
  void PCINT2_vect(void) __attribute__ ((signal));
  void PCINT3_vect(void) __attribute__ ((signal));
  void SPI_STC_vect(void) __attribute__ ((signal));
  void TIMER0_COMPA_vect(void) __attribute__ ((signal));
  void TIMER0_COMPB_vect(void) __attribute__ ((signal));
  void TIMER0_OVF_vect(void) __attribute__ ((signal));
  void TIMER1_CAPT_vect(void) __attribute__ ((signal));
  void TIMER1_COMPA_vect(void) __attribute__ ((signal));
  void TIMER1_COMPB_vect(void) __attribute__ ((signal));
  void TIMER1_OVF_vect(void) __attribute__ ((signal));
  void TIMER2_COMPA_vect(void) __attribute__ ((signal));
  void TIMER2_COMPB_vect(void) __attribute__ ((signal));
  void TIMER2_OVF_vect(void) __attribute__ ((signal));
  void TIMER3_CAPT_vect(void) __attribute__ ((signal));
  void TIMER3_COMPA_vect(void) __attribute__ ((signal));
  void TIMER3_COMPB_vect(void) __attribute__ ((signal));
  void TIMER3_OVF_vect(void) __attribute__ ((signal));
  void TWI_vect(void) __attribute__ ((signal));
  void WDT_vect(void) __attribute__ ((signal));
  void USART_RX_vect(void) __attribute__ ((signal));
  void USART_TX_vect(void) __attribute__ ((signal));
  void USART_UDRE_vect(void) __attribute__ ((signal));
  void USART1_RX_vect(void) __attribute__ ((signal));
  void USART1_UDRE_vect(void) __attribute__ ((signal));
  void USART1_TX_vect(void) __attribute__ ((signal));
}
#endif
