// Copyright 2016, Takashi Toyoshima <toyoshim@gmail.com>
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of the authors nor the names of its contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
#include "SCTimer.h"

#include <LPC8xx.h>

#include "BuildConfig.h"

// Constant variables to improve readability.
enum {
  CLK_SCT = (1 << 8),
  CLK_IOCON = (1 << 18),

  RESET_SCT_N = (1 << 8),

  SCT_CONFIG_16BIT = (0 << 0),
  SCT_CONFIG_CLKMODE_BUS = (0 << 1),
  SCT_CONFIG_AUTOLIMIT_L = (1 << 17),

  SCT_CTRL_HALT = (1 << 2),
  SCT_CTRL_PRESCALE0 = (0 << 5),

  SCT_EVENT_STATE_ON_STATE0 = (1 << 0),

  SCT_EVENT_CTRL_MATCHSEL0 = (0 << 0),
  SCT_EVENT_CTRL_MATCHSEL1 = (1 << 0),
  SCT_EVENT_CTRL_MATCHSEL2 = (2 << 0),
  SCT_EVENT_CTRL_HEVENT_L = (0 << 4),
  SCT_EVENT_CTRL_MATCH = (1 << 12),

  SCT_EVENT0 = (1 << 0),
  SCT_EVENT1 = (1 << 1),
  SCT_EVENT2 = (1 << 2),

  SCT_RES_CLR = 2,

  PIO_MODE_INACTIVE = (0 << 3),
  PIO_I2C_DISABLE = (1 << 8),

  PIO_D0 = (1 << 8),
  PIO_D1 = (1 << 9),
  PIO_D2 = (1 << 10),
  PIO_D3 = (1 << 11),
  PIO_D4 = (1 << 12),
  PIO_D5 = (1 << 13),
  PIO_D6 = (1 << 14),
  PIO_D7 = (1 << 15),
  PIO_DATA =
      (PIO_D0 | PIO_D1 | PIO_D2 | PIO_D3 | PIO_D4 | PIO_D5 | PIO_D6 | PIO_D7)
};

void SCTimerPWMInit(uint16_t range) {
  LPC_SYSCON->SYSAHBCLKCTRL |= CLK_SCT | CLK_IOCON;

  LPC_SYSCON->PRESETCTRL &= ~RESET_SCT_N;
  LPC_SYSCON->PRESETCTRL |= RESET_SCT_N;

  LPC_SCT->CONFIG = SCT_CONFIG_16BIT | SCT_CONFIG_CLKMODE_BUS | SCT_CONFIG_AUTOLIMIT_L;
  LPC_SCT->CTRL_L = SCT_CTRL_HALT | SCT_CTRL_PRESCALE0;

  LPC_SCT->MATCH_L[0] = LPC_SCT->MATCHREL_L[0] = range;
  LPC_SCT->MATCH_L[1] = LPC_SCT->MATCHREL_L[1] = 0;
  LPC_SCT->MATCH_L[2] = LPC_SCT->MATCHREL_L[2] = 0;

  LPC_SCT->EVENT[0].STATE = SCT_EVENT_STATE_ON_STATE0;
  LPC_SCT->EVENT[0].CTRL = SCT_EVENT_CTRL_MATCHSEL0 | SCT_EVENT_CTRL_HEVENT_L | SCT_EVENT_CTRL_MATCH;

  LPC_SCT->EVENT[1].STATE = SCT_EVENT_STATE_ON_STATE0;
  LPC_SCT->EVENT[1].CTRL = SCT_EVENT_CTRL_MATCHSEL1 | SCT_EVENT_CTRL_HEVENT_L | SCT_EVENT_CTRL_MATCH;

  LPC_SCT->EVENT[2].STATE = SCT_EVENT_STATE_ON_STATE0;
  LPC_SCT->EVENT[2].CTRL = SCT_EVENT_CTRL_MATCHSEL2 | SCT_EVENT_CTRL_HEVENT_L | SCT_EVENT_CTRL_MATCH;

  LPC_SCT->OUT[0].SET = SCT_EVENT1;
  LPC_SCT->OUT[0].CLR = SCT_EVENT2;
  LPC_SCT->RES = SCT_RES_CLR;

  LPC_SCT->EVEN = SCT_EVENT1;
  NVIC_EnableIRQ(SCT_IRQn);

  LPC_SCT->CTRL_L = SCT_CTRL_PRESCALE0;

#ifdef BUILD_K007340
  LPC_IOCON->PIO0_8 = PIO_MODE_INACTIVE;   // D0
  LPC_IOCON->PIO0_9 = PIO_MODE_INACTIVE;   // D1
  LPC_IOCON->PIO0_10 = PIO_I2C_DISABLE;    // D2, should be pulled-up externally
  LPC_IOCON->PIO0_11 = PIO_I2C_DISABLE;    // D3, should be pulled-up externally
  LPC_IOCON->PIO0_12 = PIO_MODE_INACTIVE;  // D4
  LPC_IOCON->PIO0_13 = PIO_MODE_INACTIVE;  // D5
  LPC_IOCON->PIO0_14 = PIO_MODE_INACTIVE;  // D6
  LPC_IOCON->PIO0_15 = PIO_MODE_INACTIVE;  // D7

  LPC_GPIO_PORT->CLR0 |= PIO_DATA;
  LPC_GPIO_PORT->DIR0 |= PIO_DATA;
#endif
}

void SCT_IRQHandler() {
  static uint16_t next = 0;
  LPC_SCT->MATCHREL_L[2] = next;
#ifdef BUILD_K007340
  LPC_GPIO_PORT->MASK0 = ~0xfe00;
  LPC_GPIO_PORT->MPIN0 = next << 6;
#endif
  next = SCTimerPWMUpdate();
  LPC_SCT->EVFLAG = SCT_EVENT1;
}

__attribute__ ((weak)) uint16_t SCTimerPWMUpdate() { return 0; }
