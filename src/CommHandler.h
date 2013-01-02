/*
 * Copyright (C) 2009 Ionut Dediu <deionut@yahoo.com>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// CommHandler.h

#ifndef __COMM_HANDLER_H__
#define __COMM_HANDLER_H__

#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#define INVALID_SOCKET -1

class CSmartEngine;

typedef enum AcceptResultCode
{
    ACCEPT_OK = 0,
    ACCEPT_RETRY = 1,
    ACCEPT_ERROR = 2
} AcceptResultCode;

typedef enum SmartCamPacketType
{
    PACKET_JPEG_HEDAER = 0,
    PACKET_JPEG_DATA = 1
} SmartCamPacketType;

#define DEFAULT_PAKET_MAX_LEN 4096

class CCommHandler
{
public:
    CCommHandler(CSmartEngine* pSmartEngine);
    virtual ~CCommHandler();
    int Initialize();
    void Cleanup();
    int StartInetServer(int port);
    int StartBtServer();
    void StopServer();
    AcceptResultCode AcceptBtClient();
    AcceptResultCode AcceptInetClient();
    int Disconnect();
    int RcvPacket();
    bool IsConnected();
    unsigned char* GetRcvPacket();
    unsigned int GetRcvPacketLen();
    SmartCamPacketType GetRcvPacketType();

private:
    // Methods:
    void RegisterBtService(uint8_t rfcommChannel);
    int DynamicBtBind(int sock, struct sockaddr_rc* sockaddr, uint8_t* port);
    // Data:
    CSmartEngine* pSmartEngine;
    bool isConnected;
    // sockets:
    int serverSocket;
    int clientSocket;
    // BT SDP:
    sdp_record_t* sdpRecord;
    sdp_session_t* sdpSession;

    unsigned char* rcvPacket;
    unsigned int rcvPacketLen;
    unsigned int rcvPacketMaxLen;
    SmartCamPacketType rcvPacketType;
};

#endif//__COMM_HANDLER_H__
