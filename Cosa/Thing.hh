/**
 * @file Cosa/Thing.hh
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
 * The Cosa class hiearchy root object; Thing. Basic event
 * handler.
 *
 * This file is part of the Arduino Che Cosa project.
 */

#ifndef __COSA_THING_HH__
#define __COSA_THING_HH__

#include "Cosa/Types.h"

class Thing {

public:
  /**
   * Event handler function prototype.
   * @param[in] it the target object.
   * @param[in] type the type of event.
   * @param[in] value the event value.
   */
  typedef void (*EventHandler)(Thing* it, uint8_t type, uint16_t value);

protected:
  EventHandler m_callback;
  Thing* m_succ;
  Thing* m_pred;

public:
  Thing(EventHandler callback = 0) : 
    m_callback(callback),
    m_succ(this),
    m_pred(this)
  {}
  
  /**
   * Set the event handler for this thing.
   * @param[in] fn event handler.
   */
  void set_event_handler(EventHandler fn) 
  { 
    m_callback = fn; 
  }

  /**
   * Get the event handler for this thing.
   * @return event handler.
   */
  EventHandler get_event_handler() 
  { 
    return (m_callback);
  }

  /**
   * Return successor in sequence.
   * @return next thing.
   */
  Thing* get_succ() 
  {
    return (m_succ);
  }

  /**
   * Return predecessor in sequence.
   * @return previous thing.
   */
  Thing* get_pred() 
  {
    return (m_pred);
  }

  /**
   * Attach given thing as predecessor.
   * @param[in] it thing to add.
   */
  void attach(Thing* it)
  {
    synchronized {
      it->m_succ = this;
      it->m_pred = this->m_pred;
      this->m_pred->m_succ = it;
      this->m_pred = it;
    }
  }

  /**
   * Detach this thing from any things.
   */
  void detach()
  {
    synchronized {
      m_succ->m_pred = m_pred;
      m_pred->m_succ = m_succ;
      m_succ = this;
      m_pred = this;
    }
  }

  /**
   * Trampoline function for event dispatch.
   * @param[in] type the event type.
   * @param[in] value the event value.
   */
  void on_event(uint8_t type, uint16_t value)
  {
    if (m_callback != 0) m_callback(this, type, value);
  }

  /**
   * Trampoline function for event dispatch.
   * @param[in] type the event type.
   * @param[in] value the event value.
   */
  void on_event(uint8_t type, void* value)
  {
    if (m_callback != 0) m_callback(this, type, (uint16_t) value);
  }
};

#endif
