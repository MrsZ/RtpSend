/**
*Author:FreeFly
*send rtp 
*/
#pragma once
#include <memory.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "rtpsession.h"
#include "rtpudpv4transmitter.h"
#include "rtpipv4address.h"
#include "rtpsessionparams.h"
#include "rtperrors.h"
#ifndef WIN32
	#include <netinet/in.h>
	#include <arpa/inet.h>
#else
	#include <winsock2.h>
#endif // WIN32
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
using namespace std;
#define MAX_RTP_PKT_LENGTH     1360
using namespace jrtplib;
class CRtpSend
{
public:
	CRtpSend(void);
	~CRtpSend(void);
public:
	void SetRTPParam(string ip,int port);
	bool InitRTP();
	bool InitSocket();
	void SendRTPH264(unsigned char* m_h264Buf,int buflen);
	void SendRTPH2641(unsigned char* m_h264Buf,int buflen);
	void CloseSocket();
	bool checkerror(int rtperr);
	bool bInitRTP;
	string rtp_ip;
	int rtp_port;
	RTPSession sess;
};

