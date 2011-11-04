/**
 * Copyright (c) 2008-2010 Leo Selavo and the contributors. All rights reserved.
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

#include <unistd.h>
#include "alarm.h"
#include <hil/timers.h>
#include <net/addr.h>

//----------------------------------------------------------
// platform-specific functions, required by alarm in HIL
//----------------------------------------------------------

// disable alarms for a while
void platformTurnAlarmsOff(void) {
// TODO
//    mos_mutex_lock(&alarmMutex);
}

// turn alarms back on
void platformTurnAlarmsOn(void) {
// TODO
//    mos_mutex_unlock(&alarmMutex);
}

//----------------------------------------------------------
// alarm handling functions
//----------------------------------------------------------

void *alarmIntHandler(void *dummy) {
    uint32_t lastTime = getRealTime();
    while (1) {
        usleep(1000);

        uint32_t thisTime = getRealTime();
        uint32_t diff = thisTime - lastTime;

// TODO
//        mos_mutex_lock(&alarmMutex);
        Alarm_t *a = getNextAlarm();
        if (a) {
            if (a->msecs <= diff) {
                a->msecs = 0;
                fireAlarm();
            } else {
                a->msecs -= diff;
            }
        }
// TODO
//        mos_mutex_unlock(&alarmMutex);

        lastTime = thisTime;
    }
    return 0;
}
