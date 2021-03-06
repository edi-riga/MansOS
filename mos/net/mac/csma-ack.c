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

#include "../mac.h"
#include "../net_queue.h"
#include <radio.h>
#include <errors.h>
#include <alarms.h>
#include <print.h>
#include <random.h>
#include <assert.h>
#include <net/radio_packet_buffer.h>
#include <net/net_stats.h>
#include <timing.h>

#define TEST_FILTERS 1

static void initCsmaMac(RecvFunction cb);
static int8_t sendCsmaMac(MacInfo_t *, const uint8_t *data, uint16_t length);
static void pollCsmaMac(void);
static void sendTimerCb(void *);
static bool ackMacBuildHeader(MacInfo_t *mi, uint8_t **header /* out */,
                              uint16_t *headerLength /* out */);

static QueuedPacket_t queuedPackets[MAC_PROTOCOL_QUEUE_SIZE];

MacProtocol_t macProtocol = {
    .name = MAC_PROTOCOL_CSMA_ACK,
    .init = initCsmaMac,
    .send = sendCsmaMac,
    .poll = pollCsmaMac,
    .buildHeader = ackMacBuildHeader,
    .isKnownDstAddress = defaultIsKnownDstAddress,
};

static Alarm_t sendTimer;
static bool sendTimerRunning;
static uint8_t mySeqnum;

// -----------------------------------------------

static void initCsmaMac(RecvFunction recvCb) {
    netQueueInit();

    macProtocol.recvCb = recvCb;

    alarmInit(&sendTimer, sendTimerCb, NULL);
    // turn on radio listening
    radioOn();
}

static int8_t sendCsmaMac(MacInfo_t *mi, const uint8_t *data, uint16_t length) {
    int8_t ret;

#if MAC_FORWARDING_DELAY
    // TODO...
#endif

    INC_NETSTAT(NETSTAT_RADIO_TX, EMPTY_ADDR);
    if (!(mi->flags & MI_FLAG_ACK_REQUESTED)) {
        // PRINTF("send a packet\n");
        // this is a broadcast message or ACK
        ret = radioSendHeader(mi->macHeader, mi->macHeaderLen, data, length);
        if (ret == 0) ret = length;
        return ret;
    }
    // PRINTF("send a packet with ACK expected\n");
    radioSendHeader(mi->macHeader, mi->macHeaderLen, data, length);
    QueuedPacket_t *p = &queuedPackets[0];
    ret = netQueueAddPacket(mi, data, length, p);
    if (ret) return ret;

    //PRINTF("%lu: packet added!\n", getTimeMs());

    p->sendTries = 0;
    p->ackTime = getJiffies() + MAC_PROTOCOL_ACK_TIME;

    if (!sendTimerRunning) {
        sendTimerRunning = true;
        alarmSchedule(&sendTimer, MAC_PROTOCOL_ACK_TIME);
    }
    return length;
}

static void sendTimerCb(void *x) {
    sendTimerRunning = false;

    //PRINTF("sendTimerCb\n");

    QueuedPacket_t *p;

    // remove expired packets
    p = queueHead();
    while (p && p->sendTries == MAC_PROTOCOL_MAX_ATTEMPTS) {
        netQueuePop();
        p = queueHead();
        INC_NETSTAT(NETSTAT_PACKETS_DROPPED_TX, EMPTY_ADDR);
    }

    if (STAILQ_EMPTY(&packetQueue)) return;

    uint32_t now = (uint32_t) getJiffies();
    uint16_t nextTimerTime = MAC_PROTOCOL_ACK_TIME;
    STAILQ_FOREACH(p, &packetQueue, chain) {
        // for this packet ack time has not yet come
        if (timeAfter32(p->ackTime, now)) {
            // adjust nextTimerTime
            uint16_t diff = p->ackTime - now;
            if (diff < nextTimerTime) nextTimerTime = diff;
            break;
        }

        p->sendTries++;
        PRINTF("%lu: ************** retry to send (try %u)\n", now, p->sendTries);
        radioSend(p->data, p->dataLength);
        INC_NETSTAT(NETSTAT_RADIO_TX, EMPTY_ADDR);
        // XXX: not the best way, need to get address more simply
        MacInfo_t mi;
        defaultParseHeader(p->data, p->dataLength, &mi);
        INC_NETSTAT(NETSTAT_PACKETS_RTX, mi.originalSrc.shortAddr);
    }

    sendTimerRunning = true;
    alarmSchedule(&sendTimer, nextTimerTime);
}

#if TEST_FILTERS

//
// filter out unwanted communications
//
static bool filterPass(MacInfo_t *mi)
{
    // PRINTF("filterPass: immedDst=%#04x, src=%#04x\n",
    //         mi->immedDst.shortAddr,
    //         mi->originalSrc.shortAddr);

    // accept messages only from motes that are using this one as the nexthop
    if (mi->immedDst.shortAddr != 0
            && mi->immedDst.shortAddr != localAddress) {
        return false;
    }

    // switch (localAddress) {
    // case 0x0079:
    //     if (mi->originalDst.shortAddr == 0x4876) return false;
    //     break;
    // case 0x4876:
    //     if (mi->originalDst.shortAddr == 0x0079) return false;
    //     break;
    // }
    return true;
}

#else

#define filterPass(mi) true

#endif

static void sendAck(MacInfo_t *mi)
{
    //PRINTF("send ack to seqnum %u\n", mi->seqnum);
    invertDirection(mi);
    mi->flags = MI_FLAG_IS_ACK; // only this flag and nothing more
    mi->macHeader[1] |= FCF_EXT_IS_ACK;
    sendCsmaMac(mi, NULL, 0);
    INC_NETSTAT(NETSTAT_PACKETS_ACK_TX, mi->originalDst.shortAddr);
}

static bool matchPacketBySeqnum(QueuedPacket_t *p, void *userData)
{
    uint8_t seqnum = (uint8_t) (uint16_t) userData;
    uint8_t packetSeqnum = getMacHeaderSeqnum(p->data);
    return seqnum == packetSeqnum;
}

static void pollCsmaMac(void)
{
    INC_NETSTAT(NETSTAT_RADIO_RX, EMPTY_ADDR);
    if (isRadioPacketReceived()) {
        //PRINTF("got a packet from radio, size=%u\n", radioPacketBuffer->receivedLength);
        //debugHexdump(radioPacketBuffer->buffer, radioPacketBuffer->receivedLength);

        // XXX: stack overflow possible if stack size is too small!
        MacInfo_t mi;
        uint8_t *data = defaultParseHeader(radioPacketBuffer->buffer,
                radioPacketBuffer->receivedLength, &mi);
        if (data) {
            if (mi.flags & MI_FLAG_IS_ACK) {
                //PRINTF("got ack to a packet with seqnum %u\n", mi.seqnum);
                INC_NETSTAT(NETSTAT_PACKETS_ACK_RX, mi.originalSrc.shortAddr);
                netQueueRemovePacket(
                        matchPacketBySeqnum, (void *) (uint16_t) mi.seqnum);
            }
            else if (macProtocol.recvCb && filterPass(&mi)) {
                //INC_NETSTAT(NETSTAT_PACKETS_RECV, mi.originalSrc.shortAddr);  // done @dv.c
                // call user callback
                macProtocol.recvCb(&mi, data,
                        radioPacketBuffer->receivedLength - mi.macHeaderLen);
                // Send MAC-layer ACK. How do we know whether the packet needs one?
                // Simple: it is not ACK itself and has nonzero sequence number.
                if (mi.seqnum != 0) {
                    sendAck(&mi);
                }
            } else{
                INC_NETSTAT(NETSTAT_PACKETS_DROPPED_RX, EMPTY_ADDR);
            }
        } else{
            INC_NETSTAT(NETSTAT_PACKETS_DROPPED_RX, EMPTY_ADDR);
        }
    }
    else if (isRadioPacketError()) {
        INC_NETSTAT(NETSTAT_PACKETS_DROPPED_RX, EMPTY_ADDR);
        PRINTF("got an error from radio: %s\n",
                strerror(-radioPacketBuffer->receivedLength));
    }
    radioBufferReset();
}

static bool ackMacBuildHeader(MacInfo_t *mi, uint8_t **header /* out */,
                              uint16_t *headerLength /* out */)
{
    // tell that ACK is requested
    if (!isBroadcast(&mi->originalDst) && !isUnspecified(&mi->originalDst)) {
        mi->flags |= MI_FLAG_ACK_REQUESTED;
        // packets for which ACK is requested must have nonzero sequence number set
        if (mySeqnum == 0) mySeqnum++;
        mi->seqnum = mySeqnum++;
    }

    // the rest is as usual
    return defaultBuildHeader(mi, header, headerLength);
}
