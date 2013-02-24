/**
 * @file Cosa/VWI.cpp
 * @version 1.0
 *
 * @section License
 * Copyright (C) 2008-2013, Mike McCauley (Author)
 * Copyright (C) 2013, Mikael Patel (Cosa C++ Port)
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
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA  02111-1307  USA
 *
 * @section Description
 * VWI (Virtual Wire Interface) an Cosa library that provides features
 * to send short messages, without addressing, retransmit or
 * acknowledgment, a bit like UDP over wireless, using ASK (Amplitude 
 * Shift Keying). Supports a number of inexpensive radio transmitters
 * and receivers. All that is required is transmit data, receive data
 * and (for transmitters, optionally) a PTT transmitter enable.
 *
 * This file is part of the Arduino Che Cosa project.
 */

#include "Cosa/VWI.hh"
#include "Cosa/RTC.hh"
#include <util/crc16.h>

const uint8_t 
VWI::symbols[] PROGMEM = {
  0xd,  0xe,  0x13, 0x15, 0x16, 0x19, 0x1a, 0x1c, 
  0x23, 0x25, 0x26, 0x29, 0x2a, 0x2c, 0x32, 0x34
};

uint8_t VWI::s_mode = 0;

uint16_t 
VWI::CRC(uint8_t* ptr, uint8_t count)
{
  uint16_t crc = 0xffff;
  while (count-- > 0) 
    crc = _crc_ccitt_update(crc, *ptr++);
  return (crc);
}

uint8_t 
VWI::symbol_6to4(uint8_t symbol)
{
  for (uint8_t i = 0; i < 16; i++)
    if (symbol == pgm_read_byte(&symbols[i]))
      return (i);
  return (0);
}

/** Current transmitter/receiver for interrupt handler access */
static VWI::Transmitter* transmitter = 0;
static VWI::Receiver* receiver = 0;

/**
 * Common function for setting timer ticks @ prescaler values for
 * speed. Returns prescaler index into {0, 1, 8, 64, 256, 1024} array
 * and sets nticks to compare-match value if lower than max_ticks
 * returns 0 & nticks = 0 on fault 
 */
static uint8_t 
_timer_calc(uint16_t speed, uint16_t max_ticks, uint16_t *nticks) 
{
  // Clock divider (prescaler) values - 0/3333: error flag
  uint16_t prescalers[] = { 0, 1, 8, 64, 256, 1024, 3333 };

  // Index into array & return bit value
  uint8_t prescaler = 0; 

  // Calculate by ntick overflow
  unsigned long ulticks;
  
  // Div-by-zero protection
  if (speed == 0) goto err;

  // Test increasing prescaler (divisor), decreasing until no overflow.
  for (prescaler = 1; prescaler < 7; prescaler += 1) {
    // Amount of time per CPU clock tick (in seconds)
    float clock_time = (1.0 / (float(F_CPU) / float(prescalers[prescaler])));
    // Fraction of second needed to xmit one bit
    float bit_time = ((1.0 / float(speed)) / 8.0);
    // number of prescaled ticks needed to handle bit time @ speed
    ulticks = long(bit_time / clock_time);
    // Test if ulticks fits in nticks bitwidth (with 1-tick safety margin)
    if ((ulticks > 1) && (ulticks < max_ticks)) {
      break; // found prescaler
    }
    // Won't fit, check with next prescaler value
  }

  // Check for error
  if ((prescaler == 6) || (ulticks < 2) || (ulticks > max_ticks)) 
    goto err;
  
  *nticks = ulticks;
  return (prescaler);

 err:
  *nticks = 0;
  return (0);
}

void 
VWI::Receiver::PLL()
{
  // Integrate each sample
  if (m_sample) m_integrator++;

  if (m_sample != m_last_sample) {
    // Transition, advance if ramp > TRANSITION otherwise retard
    m_pll_ramp += 
      ((m_pll_ramp < RAMP_TRANSITION) ? RAMP_INC_RETARD : RAMP_INC_ADVANCE);
    m_last_sample = m_sample;
  }
  else {
    // No transition: Advance ramp by standard INC (== MAX/BITS samples)
    m_pll_ramp += RAMP_INC;
  }
  if (m_pll_ramp >= RAMP_MAX) {
    // Add this to the 12th bit of vw_rx_bits, LSB first. The last 12
    // bits are kept 
    m_bits >>= 1;

    // Check the integrator to see how many samples in this cycle were
    // high. If < 5 out of 8, then its declared a 0 bit, else a 1;
    if (m_integrator >= 5)
      m_bits |= 0x800;

    m_pll_ramp -= RAMP_MAX;

    // Clear the integral for the next cycle
    m_integrator = 0; 

    if (m_active) {
      // We have the start symbol and now we are collecting message
      // bits, 6 per symbol, each which has to be decoded to 4 bits
      if (++m_bit_count >= 12) {
	// Have 12 bits of encoded message == 1 byte encoded. Decode
	// as 2 lots of 6 bits into 2 lots of 4 bits. The 6 lsbits are
	// the high nybble.
	uint8_t data = 
	  (symbol_6to4(m_bits & 0x3f)) << 4 | symbol_6to4(m_bits >> 6);
	
	// The first decoded byte is the byte count of the following
	// message the count includes the byte count and the 2
	// trailing FCS bytes. REVISIT: may also include the ACK flag
	// at 0x40.
	if (m_length == 0) {
	  // The first byte is the byte count. Check it for
	  // sensibility. It cant be less than 4, since it includes
	  // the bytes count itself and the 2 byte FCS 
	  m_count = data;
	  if (m_count < 4 || m_count > MESSAGE_MAX) {
	    // Stupid message length, drop the whole thing
	    m_active = false;
	    m_bad++;
	    return;
	  }
	}
	m_buffer[m_length++] = data;
	if (m_length >= m_count) {
	  // Got all the bytes now
	  m_active = false;
	  m_good++;
	  // Better come get it before the next one starts
	  m_done = true;
	}
	m_bit_count = 0;
      }
    }

    // Not in a message, see if we have a start symbol
    else if (m_bits == 0xb38) {
      // Have start symbol, start collecting message
      m_active = true;
      m_bit_count = 0;
      m_length = 0;
      // Too bad if you missed the last message
      m_done = false;
    }
  }
}

bool 
VWI::begin(uint16_t speed, uint8_t mode)
{
  // Number of prescaled ticks needed
  uint16_t nticks;

  // Bit values for CS0[2:0]
  uint8_t prescaler;

  // Set sleep mode
  s_mode = mode;

#if defined(__AVR_ATtiny85__)
  // Figure out prescaler value and counter match value
  prescaler = _timer_calc(speed, (uint8_t)-1, &nticks);
  if (!prescaler) return (0);

  // Turn on CTC mode / Output Compare pins disconnected
  TCCR0A = 0;
  TCCR0A = _BV(WGM01);

  // Convert prescaler index to TCCRnB prescaler bits CS00, CS01, CS02
  TCCR0B = 0;
  // Set CS00, CS01, CS02 (other bits not needed)
  TCCR0B = prescaler; 

  // Number of ticks to count before firing interrupt
  OCR0A = uint8_t(nticks);

  // Set mask to fire interrupt when OCF0A bit is set in TIFR0
  TIMSK |= _BV(OCIE0A);

#else
  // Figure out prescaler value and counter match value
  prescaler = _timer_calc(speed, (uint16_t)-1, &nticks);
  if (!prescaler) return (0);

  // Output Compare pins disconnected, and turn on CTC mode
  TCCR1A = 0; 
  TCCR1B = _BV(WGM12);

  // Convert prescaler index to TCCRnB prescaler bits CS10, CS11, CS12
  TCCR1B |= prescaler;

  // Caution: special procedures for setting 16 bit regs
  // is handled by the compiler
  OCR1A = nticks;

  // Enable interrupt
#ifdef TIMSK1
  TIMSK1 |= _BV(OCIE1A);
#else
  TIMSK |= _BV(OCIE1A);
#endif
#endif

  return (1);
}

VWI::Receiver::Receiver(Board::DigitalPin rx) : 
  InputPin(rx) 
{
  receiver = this;
}

bool 
VWI::Receiver::await(unsigned long ms)
{
  if (ms == 0) {
    while (!m_done);
  }
  else {
    unsigned long start = RTC::millis();
    while (!m_done && ((RTC::millis() - start) < ms));
  }
  return (m_done);
}

bool 
VWI::Receiver::recv(void* buf, uint8_t* len)
{
  uint8_t rxlen;
    
  // Message available?
  if (!m_done) return (0);
    
  // Wait until done is set before reading length then remove
  // bytecount and FCS  
  rxlen = m_length - 3;
    
  // Copy message (good or bad). Skip count byte
  if (*len > rxlen)
    *len = rxlen;
  memcpy(buf, m_buffer + 1, *len);
    
  // OK, got that message thanks
  m_done = false;
    
  // Check the FCS, return goodness
  return (CRC(m_buffer, m_length) == 0xf0b8);
}

const uint8_t 
VWI::Transmitter::header[HEADER_MAX] PROGMEM = {
  0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x38, 0x2c
};

VWI::Transmitter::Transmitter(Board::DigitalPin tx) :
  OutputPin(tx)
{
  transmitter = this;
}

bool 
VWI::Transmitter::begin()
{
  memcpy_P(m_buffer, header, sizeof(header));
  m_index = 0;
  m_bit = 0;
  m_sample = 0;
  m_enabled = true;
  return (1);
}

void 
VWI::Transmitter::await()
{
  while (m_enabled) {
    cli();
    set_sleep_mode(s_mode);
    sleep_enable();
    sei();
    sleep_cpu();
    sleep_disable();
  }
}

bool 
VWI::Transmitter::send(void* buf, uint8_t len)
{
  uint8_t ix = 0;
  uint16_t crc = 0xffff;
  uint8_t *p = transmitter->m_buffer + HEADER_MAX;
  uint8_t *bp = (uint8_t*) buf;
  uint8_t count = len + 3;

  // Check that the message is not too large
  if (len > PAYLOAD_MAX) return (0);

  // Wait for transmitter to become available
  await();

  // Encode the message length
  crc = _crc_ccitt_update(crc, count);
  p[ix++] = pgm_read_byte(&symbols[count >> 4]);
  p[ix++] = pgm_read_byte(&symbols[count & 0xf]);

  // Encode the message into 6 bit symbols. Each byte is converted into 
  // 2 6-bit symbols, high nybble first, low nybble second
  for (uint8_t i = 0; i < len; i++) {
    crc = _crc_ccitt_update(crc, bp[i]);
    p[ix++] = pgm_read_byte(&symbols[bp[i] >> 4]);
    p[ix++] = pgm_read_byte(&symbols[bp[i] & 0xf]);
  }

  // Append the fcs, 16 bits before encoding (4 6-bit symbols after
  // encoding) Caution: VW expects the _ones_complement_ of the CCITT
  // CRC-16 as the FCS VW sends FCS as low byte then hi byte
  crc = ~crc;
  p[ix++] = pgm_read_byte(&symbols[(crc >> 4)  & 0xf]);
  p[ix++] = pgm_read_byte(&symbols[crc & 0xf]);
  p[ix++] = pgm_read_byte(&symbols[(crc >> 12) & 0xf]);
  p[ix++] = pgm_read_byte(&symbols[(crc >> 8)  & 0xf]);

  // Total number of 6-bit symbols to send
  m_length = ix + HEADER_MAX;

  // Start the low level interrupt handler sending symbols
  return (begin());
}

/**
 * This is the interrupt service routine called when timer1
 * overflows. Its job is to output the next bit from the transmitter
 * (every 8 calls) and to call the PLL code if the receiver is enabled.
 */
#ifdef __AVR_ATtiny85__
ISR(TIM0_COMPA_vect)
#else 
ISR(TIMER1_COMPA_vect)
#endif
{
  // Check if the receiver pin should be sampled
  if ((receiver != 0) 
      && (receiver->m_enabled)
      && (transmitter == 0 || !transmitter->m_enabled))
    receiver->m_sample = receiver->read();
    
  // Do transmitter stuff first to reduce transmitter bit jitter due 
  // to variable receiver processing
  if (transmitter != 0) {
    if (transmitter->m_enabled && transmitter->m_sample++ == 0) {
      // Send next bit. Symbols are sent LSB first. Finished sending the
      // whole message? (after waiting one bit period since the last bit)
      if (transmitter->m_index >= transmitter->m_length) {
	transmitter->end();
	transmitter->m_msg_count++;
      }
      else {
	transmitter->write(transmitter->m_buffer[transmitter->m_index] & 
			   (1 << transmitter->m_bit++));
	if (transmitter->m_bit >= 6) {
	  transmitter->m_bit = 0;
	  transmitter->m_index++;
	}
      }
    }
    if (transmitter->m_sample > 7)
      transmitter->m_sample = 0;
  }
  
  if ((receiver != 0) 
      && (receiver->m_enabled)
      && (transmitter == 0 || !transmitter->m_enabled))
    receiver->PLL();
}

