/**
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

//
// PC USART
//

#ifdef WIN32
#include <winsock.h>
#endif
#include <hil/usart.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include "usart_hal.h"

//===========================================================
// Variables
//===========================================================

USARTCallback_t usartRecvCb[USART_COUNT];

static bool txEnabled[USART_COUNT];
static bool rxEnabled[USART_COUNT];
static pthread_t rxThread;
static void *rxHandler(void *dummy);

//===========================================================
// Procedures
//===========================================================

uint_t USARTInit(uint8_t id, uint32_t speed, uint8_t conf)
{
    return 0;
}

uint_t USARTSendByte( uint8_t id, uint8_t data)
{
    if (id >= USART_COUNT) return (uint_t)-1u;

    if (txEnabled[id]) {
        printf("%c", data);
        fflush(stdout);
    }
    return 0;
}

uint_t USARTEnableTX( uint8_t id )
{
    if (id >= USART_COUNT) return (uint_t)-1u;

    txEnabled[id] = true;
    return 0;
}

uint_t USARTDisableTX( uint8_t id )
{
    if (id > USART_COUNT) return (uint_t)-1u;

    txEnabled[id] = false;
    return 0;
}

uint_t USARTEnableRX( uint8_t id )
{
    if (id >= USART_COUNT) return (uint_t)-1u;

    if (!rxEnabled[id]) {
        rxEnabled[id] = true;
        pthread_create(&rxThread, NULL, rxHandler, (void *) (uint32_t) id);
    }
    return 0;
}

uint_t USARTDisableRX( uint8_t id )
{
    if (id > USART_COUNT) return (uint_t)-1u;

    if (rxEnabled[id]) {
        rxEnabled[id] = false;
        pthread_join(rxThread, NULL);
    }
    return 0;
}

// simulate serial rx in a separate thread
static void *rxHandler(void *arg)
{
    uint8_t id = (uint8_t) (uint32_t) arg;

    while (rxEnabled[id]) {
        fd_set rfds;
        struct timeval tv;
        int ret;

        FD_ZERO(&rfds);
        FD_SET(0, &rfds);

        tv.tv_sec = 1;
        tv.tv_usec = 0;

        ret = select(1, &rfds, NULL, NULL, &tv);
        if (ret == -1) {
            perror("rxHandler select");
            break;
        }

        if (FD_ISSET(0, &rfds)) {
            char c = 0;
            ssize_t ret = read(0, &c, 1);
            if (ret < 0) {
                perror("rxHandler read");
                break;
            }
            if (usartRecvCb[id]) usartRecvCb[id](c);
        }        
    }
    pthread_exit(NULL);
    return NULL; // make gcc happy
}