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

// CommHandler.cpp

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

#include "CommHandler.h"
#include "SmartEngine.h"
#include "UIHandler.h"

// Constructor
CCommHandler::CCommHandler(CSmartEngine* pEngine):
    pSmartEngine(pEngine),
    isConnected(false),
    serverSocket(INVALID_SOCKET),
    clientSocket(INVALID_SOCKET),
    sdpRecord(NULL),
    sdpSession(NULL),
    rcvPacket(NULL),
    rcvPacketLen(0),
    rcvPacketMaxLen(0)
{
}

// Destructor
CCommHandler::~CCommHandler()
{
}

int CCommHandler::Initialize()
{
    rcvPacket = new unsigned char[DEFAULT_PAKET_MAX_LEN];
    if(rcvPacket == NULL)
        return -1;
    rcvPacketMaxLen = DEFAULT_PAKET_MAX_LEN;
    return 0;
}

int CCommHandler::StartInetServer(int port)
{
    int flags = 0;
    struct sockaddr_in sin;
    // Initialize the addr
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);

    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(serverSocket == INVALID_SOCKET)
    {
        CUIHandler::Msg("Could not create inet socket: %d\n(%s)", errno, strerror(errno));
        return -1;
    }

    // Bind the socket to the address returned
    if(bind(serverSocket, (struct sockaddr*)&sin, sizeof(sin)) < 0)
    {
        CUIHandler::Msg("Could not bind inet socket: %d\n(%s)", errno, strerror(errno));
        close(serverSocket);
        return -1;
    }
    if(listen(serverSocket, 1) < 0)
    {
        CUIHandler::Msg("Could not listen on inet socket: %d\n(%s)", errno, strerror(errno));
        close(serverSocket);
        return -1;
    }
    flags = fcntl(serverSocket, F_GETFL, NULL);
    if(flags < 0)
    {
        CUIHandler::Msg("Could not retrieve socket flags: %d\n(%s)", errno, strerror(errno));
        return -1;
    }
    flags |= O_NONBLOCK;
    fcntl(serverSocket, F_SETFL, flags);

    return 0;
}

void CCommHandler::RegisterBtService(uint8_t rfcommChannel)
{
    uint8_t svc_uuid_int[] = { 0xB9, 0xDE, 0xC6, 0xD2, 0x29, 0x30, 0x43, 0x38, 0xA0, 0x79, 0xAA, 0xE5, 0x60, 0x05, 0x32, 0x38 };
    const char* service_name = "SmartCam";
    const char* service_dsc = "Smartphone Webcam";
    const char* service_prov = "Deion";

    uuid_t root_uuid, l2cap_uuid, rfcomm_uuid, svc_uuid;
    sdp_list_t *l2cap_list = 0,
    *rfcomm_list = 0,
    *root_list = 0,
    *proto_list = 0,
    *access_proto_list = 0;
    sdp_data_t* channel = 0, *psm = 0;

    sdpRecord = sdp_record_alloc();

    // set the general service ID
    sdp_uuid128_create(&svc_uuid, &svc_uuid_int);
    sdp_set_service_id(sdpRecord, svc_uuid);

    // make the service record publicly browsable
    sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
    root_list = sdp_list_append(0, &root_uuid);
    sdp_set_browse_groups(sdpRecord, root_list);

    // set l2cap information
    sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
    l2cap_list = sdp_list_append(0, &l2cap_uuid);
    proto_list = sdp_list_append(0, l2cap_list);

    // set rfcomm information
    sdp_uuid16_create(&rfcomm_uuid, RFCOMM_UUID);
    channel = sdp_data_alloc(SDP_UINT8, &rfcommChannel);
    rfcomm_list = sdp_list_append(0, &rfcomm_uuid);
    sdp_list_append(rfcomm_list, channel);
    sdp_list_append(proto_list, rfcomm_list);

    // attach protocol information to service record
    access_proto_list = sdp_list_append(0, proto_list);
    sdp_set_access_protos(sdpRecord, access_proto_list);

    // set the name, provider, and description
    sdp_set_info_attr(sdpRecord, service_name, service_prov, service_dsc);

    int err = 0;

    // connect to the local SDP server, register the service record
    bdaddr_t any = {0, 0, 0, 0xff, 0xff, 0xff};
    bdaddr_t local = {0, 0, 0, 0xff, 0xff, 0xff};
    sdpSession = sdp_connect(&any, &local, SDP_RETRY_IF_BUSY);
    err = sdp_record_register(sdpSession, sdpRecord, 0);
    if(err)
    {
        perror("sdp_record_register");
    }

    // cleanup
    sdp_data_free(channel);
    sdp_list_free(l2cap_list, 0);
    sdp_list_free(rfcomm_list, 0);
    sdp_list_free(root_list, 0);
    sdp_list_free(access_proto_list, 0);
}

int CCommHandler::DynamicBtBind(int sock, struct sockaddr_rc* sockaddr, uint8_t* port)
{
    int status = 0;
    for(*port = 1; *port <= 30; *port++)
    {
        sockaddr->rc_channel = htons(*port);
        status = bind(sock, (struct sockaddr*) sockaddr, sizeof(*sockaddr));
        if(status == 0)
            return 0;
    }
    errno = EINVAL;
    return -1;
}

int CCommHandler::StartBtServer()
{
    int flags = 0;
    struct sockaddr_rc localAddr = { 0 };
    socklen_t opt = sizeof(localAddr);

    // allocate server socket
    serverSocket = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

    // bind socket to 1st available port of the first available local bluetooth adapter
    bdaddr_t any = {0, 0, 0, 0xff, 0xff, 0xff};
    localAddr.rc_family = AF_BLUETOOTH;
    localAddr.rc_bdaddr = any;
    uint8_t port = 0;
    if(DynamicBtBind(serverSocket, &localAddr, &port))
    {
        perror("smartcam: dynamic_bind_rc");
        close(serverSocket);
        return -1;
    }
    printf("smartcam: port = %d\n", port);
    // advertise bt service
    RegisterBtService(port);
    if(listen(serverSocket, 1) < 0)
    {
        CUIHandler::Msg("Could not listen on bt socket: %d\n(%s)", errno, strerror(errno));
        close(serverSocket);
        return -1;
    }
    flags = fcntl(serverSocket, F_GETFL, NULL);
    if(flags < 0)
    {
        CUIHandler::Msg("Could not retrieve socket flags: %d\n(%s)", errno, strerror(errno));
        return -1;
    }
    flags |= O_NONBLOCK;
    fcntl(serverSocket, F_SETFL, flags);
    return 0;
}

AcceptResultCode CCommHandler::AcceptBtClient()
{
    struct sockaddr_rc remAddr = { 0 };
    socklen_t opt = sizeof(remAddr);
    // accept one connection
    if((clientSocket = accept(serverSocket, (struct sockaddr*) &remAddr, &opt)) == INVALID_SOCKET)
    {
        if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
        {
            return ACCEPT_RETRY;
        }
        CUIHandler::Msg("Could not accept bt connection on socket: %d\n(%s)", errno, strerror(errno));
        close(serverSocket);
        return ACCEPT_ERROR;
    }

    char buf[255] = {0};
    ba2str(&remAddr.rc_bdaddr, buf);
    printf("smartcam: accepted bt connection from %s\n", buf);

    isConnected = 1;
    pSmartEngine->OnConnected();
    return ACCEPT_OK;
}

AcceptResultCode CCommHandler::AcceptInetClient()
{
    struct sockaddr_in remAddr = { 0 };
    socklen_t opt = sizeof(remAddr);
    // accept one connection
    if((clientSocket = accept(serverSocket, (struct sockaddr*) &remAddr, &opt)) == INVALID_SOCKET)
    {
        if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
        {
            return ACCEPT_RETRY;
        }
        CUIHandler::Msg("Could not accept inet connection on socket: %d\n(%s)", errno, strerror(errno));
        close(serverSocket);
        return ACCEPT_ERROR;
    }

    char* remAddrStr = inet_ntoa(remAddr.sin_addr);
    if(remAddrStr != NULL)
    {
        printf("smartcam: accepted inet connection from %s\n", remAddrStr);
    }
    else
    {
        printf("smartcam: accepted inet connection, but inet_ntoa() failed ...\n");
    }

    isConnected = 1;
    pSmartEngine->OnConnected();
    return ACCEPT_OK;
}

int CCommHandler::Disconnect()
{
    isConnected = 0;
    // close the sockets (if opened)
    if(clientSocket != INVALID_SOCKET)
    {
        close(clientSocket);
        clientSocket = INVALID_SOCKET;
    }
    return 0;
}

int CCommHandler::RcvPacket()
{
    int retCode = 0;
    unsigned char header[4] = {0};

    retCode = recv(clientSocket, (char*) header, 4, 0);
    if((retCode == 0) || (retCode == -1))
    {
        Disconnect();
        pSmartEngine->OnDisconnected();
        return -1;
    }

    rcvPacketType = (SmartCamPacketType) (header[0]);
    rcvPacketLen = ((unsigned int)header[1] << 16) | ((unsigned int)header[2] << 8) | ((unsigned int)header[3]);
    if(rcvPacketMaxLen < rcvPacketLen)
    {
        delete[] rcvPacket;
        rcvPacketMaxLen = rcvPacketLen + rcvPacketLen/3;
        rcvPacket = new unsigned char[rcvPacketMaxLen];
    }

    int rcvdBytesCount = 0;
    while(rcvdBytesCount < rcvPacketLen)
    {
        retCode = recv(clientSocket, ((char*) rcvPacket) + rcvdBytesCount, rcvPacketLen - rcvdBytesCount, 0);
        // Connection closed or socket error
        if((retCode == 0) || (retCode == -1))
        {
            Disconnect();
            pSmartEngine->OnDisconnected();
            return -1;
        }
        // All went well, advance the byte count
        else
        {
            rcvdBytesCount += retCode;
        }
    }

    return 0;
}

void CCommHandler::StopServer()
{
    if(sdpRecord != NULL)
    {
        sdp_record_unregister(sdpSession, sdpRecord);
        sdpRecord = NULL;
    }
    if(sdpSession != NULL)
    {
        sdp_close(sdpSession);
        sdpSession = NULL;
    }

    // close the sockets (if open)
    if(clientSocket != INVALID_SOCKET)
    {
        close(clientSocket);
        clientSocket = INVALID_SOCKET;
    }
    if(serverSocket != INVALID_SOCKET)
    {
        close(serverSocket);
        serverSocket = INVALID_SOCKET;
    }
}

void CCommHandler::Cleanup()
{
    StopServer();
    if(rcvPacket != NULL)
    {
        delete[] rcvPacket;
        rcvPacket = NULL;
    }
    rcvPacketLen = 0;
    rcvPacketMaxLen = 0;
}

bool CCommHandler::IsConnected()
{
    return isConnected;
}

unsigned char* CCommHandler::GetRcvPacket()
{
    return rcvPacket;
}

unsigned int CCommHandler::GetRcvPacketLen()
{
    return rcvPacketLen;
}

SmartCamPacketType CCommHandler::GetRcvPacketType()
{
    return rcvPacketType;
}
