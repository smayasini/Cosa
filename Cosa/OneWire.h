/**
 * @file Cosa/OneWire.h
 * @version 1.0
 *
 * @section License
 * Copyright (C) 2012, Mikael Patel
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
 * One-wire device driver support class. Allows device rom search
 * and connection to multiple devices on one-wire bus.
 *
 * This file is part of the Arduino Che Cosa project.
 */

#ifndef __COSA_ONEWIRE_H__
#define __COSA_ONEWIRE_H__

#include "Cosa/Pins.h"
#include "Cosa/IOStream.h"
#include "Cosa/Trace.h"

class OneWire : private IOPin {
private:
  uint8_t _crc;

public:
  /**
   * ROM Commands and Size
   */
  enum {
    SEARCH_ROM = 0xF0,
    READ_ROM = 0x33,
    MATCH_ROM = 0x55,
    SKIP_ROM = 0xCC,
    ALARM_SEARCH = 0xEC
  };
  static const uint8_t ROM_MAX = 8;
  static const uint8_t ROMBITS = ROM_MAX * CHARBITS;

  /**
   * Driver for device connected to a one-wire pin
   */
  class Driver {
    friend class OneWire;
  protected:
    enum {
      FIRST = -1,
      ERROR = -1,
      LAST = ROMBITS
    };
    uint8_t _rom[ROM_MAX];
    OneWire* _pin;

  public:
    /**
     * Construct one wire device driver.
     * @param[in] pin one wire bus.
     */
    Driver(OneWire* pin) : _pin(pin) {}

    /**
     * Search device rom given the last position of discrepancy.
     * Negative value for start from the beginning.
     * @param[in] last position of discrepancy.
     * @return position of difference or negative error code.
     */
    int8_t search_rom(int8_t last = FIRST);

    /**
     * Read device rom. This can only be used when there is only
     * one slave on the bus.
     * @return true(1) if successful otherwise false(0).
     */
    bool read_rom();

    /**
     * Match device rom. Address the slave device with the
     * rom code. Device specific function command should follow.
     * May be used to verify rom code.
     * @return true(1) if successful otherwise false(0).
     */
    bool match_rom();

    /**
     * Skip device rom for boardcast or single device access.
     * Device specific function command should follow.
     * @return true(1) if successful otherwise false(0).
     */
    bool skip_rom();

    /**
     * Connect to one-wire device with given family code and index.
     * @param[in] family device family code.
     * @param[in] index device order.
     * @return true(1) if successful otherwise false(0).
     */
    bool connect(uint8_t family, uint8_t index);

  public:
    /**
     * Print device rom to output stream. Default stream
     * is the trace stream.
     * @param[in] stream to print rom to.
     */
    void print_rom(IOStream& stream = trace);
  };

  /**
   * Act as slave device connected to a one-wire pin
   */
  class Device : public InterruptPin {
    friend class OneWire;
  private:
    // One-wire slave pin mode
    enum Mode {
      OUTPUT_MODE = 0,
      INPUT_MODE = 1
    };

    // One-wire slave states
    enum State {
      IDLE_STATE,
      RESET_STATE,
      PRESENCE_STATE,
      ROM_STATE,
      FUNCTION_STATE
    };

    /**
     * Set slave device pin input/output mode.
     * @param[in] mode pin mode.
     */
    void set_mode(Mode mode)
    {
      synchronized {
	if (mode == OUTPUT_MODE)
	  *DDR() |= _mask; 
	else
	  *DDR() &= ~_mask; 
      }
    }

    /**
     * Set slave device pin.
     */
    void set() 
    { 
      synchronized {
	*PORT() |= _mask; 
      }
    }

    /**
     * Clear slave device pin.
     */
    void clear() 
    { 
      synchronized {
	*PORT() &= ~_mask; 
      }
    }

    /**
     * Read the given number of bits from the one wire bus (master).
     * Default number of bits is 8. Returns the value read LSB aligned.
     * or negative error code.
     * @param[in] bits to be read.
     * @return value read.
     */
    int read(uint8_t bits = CHARBITS);

    /**
     * Write the given value to the one wire bus. The bits are written
     * from LSB to MSB. Return true(1) if successful otherwise false(0).
     * @param[in] value.
     * @param[in] bits to be written.
     */
    bool write(uint8_t value, uint8_t bits);

    /**
     * Slave device event handler function. Handle presence pulse and
     * rom/function command parsing.
     * @param[in] it the target object.
     * @param[in] type the type of event.
     * @param[in] value the event value.
     */
    static void service_request(Thing* it, uint8_t type, uint16_t value);

    /**
     * Slave device interrupt handler function. Detect reset and initiate
     * presence pulse. Push service_request event for further handling.
     * @param[in] pin reference of changed pin.
     * @param[in] env interrupt handler environment. 
     */
    static void interrupt_handler(InterruptPin* pin, void* env);
    friend void interrupt_handler(InterruptPin* pin, void* env);

  protected:
    uint8_t* _rom;
    volatile uint32_t _time;
    volatile uint8_t _crc;
    volatile State _state;

  public:
    // Slave function codes
    enum {
      STATUS = 0x11
    };

    /**
     * Construct one wire slave device connected to the given pin and
     * rom identity code. Note: crc is generated automatically.
     * @param[in] pin number.
     * @param[in] rom identity number.
     */
    Device(uint8_t pin, uint8_t* rom) : 
      InterruptPin(pin, 
		   InterruptPin::ON_CHANGE_MODE, 
		   interrupt_handler),
      _rom(rom),
      _time(0),
      _crc(0),
      _state(IDLE_STATE)
    {
      set_event_handler(service_request);
    }
  };
  
  /**
   * Construct one wire bus connected to the given pin.
   * @param[in] pin number.
   */
  OneWire(uint8_t pin) : IOPin(pin) {}

  /**
   * Reset the one wire bus and check that at least one device is
   * presence.
   * @return true(1) if successful otherwise false(0).
   */
  bool reset();

  /**
   * Read the given number of bits from the one wire bus (slave).
   * Default number of bits is 8. Returns the value read LSB aligned.
   * The internal CRC is updated (see begin() and end()).
   * @param[in] bits to be read.
   * @return value read.
   */
  uint8_t read(uint8_t bits = CHARBITS);

  /**
   * Write the given value to the one wire bus. The bits are written
   * from LSB to MSB. 
   * @param[in] value.
   * @param[in] bits to be written.
   */
  void write(uint8_t value, uint8_t bits = CHARBITS);

  /**
   * Begin a read sequence with CRC.
   */
  void begin() { _crc = 0; }

  /**
   * End a read sequence and return the generated CRC. If the
   * read block contains a CRC last the returned value will be
   * zero(0).
   * @return generated CRC.
   */
  uint8_t end() { return (_crc); }

  /**
   * Print list of connected devices on given stream.
   * Default stream is the trace stream.
   * @param[in] stream to print rom to.
   */
  void print_devices(IOStream& stream = trace);
};

#endif
