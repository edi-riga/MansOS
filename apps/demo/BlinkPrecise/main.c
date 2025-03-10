/*
 * Copyright (c) 2008-2012 the MansOS team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of  conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

//-------------------------------------------
// Blink - demo application that blinks a single LED with one second interval
//-------------------------------------------

#include "stdmansos.h"

//#define PERIOD 1000 // milliseconds
//#define PERIOD 60000ul // milliseconds
#define PERIOD 5000ul // milliseconds

uint32_t nextPeriod;
Alarm_t alarm1;

void onAlarm(void *x) {
   redLedToggle();

   // schedule the next alarm *exactly* 1000 milliseconds after this one
   nextPeriod += PERIOD;
   alarmSchedule(&alarm1, nextPeriod - getJiffies());
}

void appMain(void)
{
    redLedToggle();

    // initialize and schedule the alarm
    alarmInit(&alarm1, onAlarm, NULL);
    nextPeriod = PERIOD;
    // jiffie counter starts from 0 (zero) after reset
    alarmSchedule(&alarm1, PERIOD - getJiffies());


    // enter infinite low-power loop
    for (;;) {
        sleep(10);
    }
}
