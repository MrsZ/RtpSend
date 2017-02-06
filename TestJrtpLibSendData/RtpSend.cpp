/**
*Author:FreeFly
*send rtp 
*/
#include "RtpSend.h"
CRtpSend::CRtpSend(void)
{
	bInitRTP = false;
}
CRtpSend::~CRtpSend(void)
{
	CloseSocket();
}
//////////////////////////////////////////////////////////////////////////
void CRtpSend::SetRTPParam(string ip,int port)
{
	rtp_ip = ip;
	rtp_port = port;
}
bool CRtpSend::InitRTP()
{
	//rtp_ip = ip;
	//rtp_port = port;
	if ( InitSocket())
	{
		//RTP+RTCP库初始化SOCKET环境
		RTPUDPv4TransmissionParams transparams;

		RTPSessionParams sessparams;

		sessparams.SetOwnTimestampUnit(1.0/90000.0);	   //时间戳单位	

		sessparams.SetAcceptOwnPackets(true);
		uint16_t portbase = rtp_port*2; //PORT_LOCAL;
		transparams.SetPortbase(portbase);		
		int status;	
		status = sess.Create(sessparams,&transparams);	

		if (!checkerror(status))
		{
			return false;
		}
		uint32_t destip;
		destip = inet_addr(rtp_ip.c_str());
		destip = ntohl(destip);
		uint16_t destport = rtp_port;
		RTPIPv4Address addr(destip,rtp_port);

		status = sess.AddDestination(addr);

		if (!checkerror(status))
		{
			return false;
		}
		sess.SetDefaultPayloadType(96);
		sess.SetDefaultMark(false);
		sess.SetDefaultTimestampIncrement(3600);
		bInitRTP = true;
		return true;
	}
	return false;
}
bool CRtpSend::InitSocket()
{
	int Error;
	WORD VersionRequested;
	WSADATA WsaData;
	VersionRequested=MAKEWORD(2,2);
	Error=WSAStartup(VersionRequested,&WsaData); //启动WinSock2
	if(Error!=0)
	{
		return false;
	}
	else
	{
		if(LOBYTE(WsaData.wVersion)!=2||HIBYTE(WsaData.wHighVersion)!=2)
		{
			WSACleanup();
			return false;
		}
	}
	return true;
}
 void CRtpSend::SendRTPH2641(unsigned char* m_h264Buf,int buflen)   
{  
	if (!bInitRTP)
	{
		if (!InitRTP())
		{
			printf("InitRTP fail\n");
			return;
		}
	}
    unsigned char *pSendbuf; //发送数据指针  
    pSendbuf = m_h264Buf;  
    int status = 0;  
    char sendbuf[1430];   //发送的数据缓冲  
    memset(sendbuf,0,1430);  

    printf("send packet length %d \n",buflen);  
  
    if ( buflen <= MAX_RTP_PKT_LENGTH )  
    {    
		//设置标志位Mark为1  
        sess.SetDefaultMark(true);  
        memcpy(sendbuf,pSendbuf,buflen);
        status = sess.SendPacket((void *)sendbuf,buflen + 1,96,true,3600);  
        checkerror(status);  
    }    
    else if(buflen > MAX_RTP_PKT_LENGTH)  
    {  
        //设置标志位Mark为0  
        sess.SetDefaultMark(false);  
        //printf("buflen = %d\n",buflen);  
        //得到该需要用多少长度为MAX_RTP_PKT_LENGTH字节的RTP包来发送  
        int k=0,l=0;    
        k = buflen / MAX_RTP_PKT_LENGTH;  
        l = buflen % MAX_RTP_PKT_LENGTH;  
        int t=0;//用指示当前发送的是第几个分片RTP包  
  
        char nalHeader = pSendbuf[0]; // NALU 头
        while( t < k || ( t==k && l>0 ) )    
        {    
            if( (0 == t ) || ( t<k && 0!=t ) )//第一包到最后包的前一包  
            {  
                memcpy(sendbuf,&pSendbuf[t*MAX_RTP_PKT_LENGTH],MAX_RTP_PKT_LENGTH);  
                status = sess.SendPacket((void *)sendbuf,MAX_RTP_PKT_LENGTH,96,false,0); 
                checkerror(status);  
                t++;  
            }  
            //最后一包  
            else if( ( k==t && l>0 ) || ( t== (k-1) && l==0 ))  
            {  
                //设置标志位Mark为1  
                sess.SetDefaultMark(true);  
   
                int iSendLen;  
                if ( l > 0)  
                {  
                    iSendLen = buflen - t*MAX_RTP_PKT_LENGTH;  
                }  
                else  
                    iSendLen = MAX_RTP_PKT_LENGTH; 
     
                memcpy(sendbuf,&pSendbuf[t*MAX_RTP_PKT_LENGTH],iSendLen);  
                status = sess.SendPacket((void *)sendbuf,iSendLen,96,true,3600);  

                checkerror(status);  
                t++;  
            }  
        }  
    }  
} 

void CRtpSend::SendRTPH264(unsigned char* m_h264Buf,int buflen) 
{
	if (!bInitRTP)
	{
		if (!InitRTP())
		{
			printf("InitRTP fail\n");
			return;
		}
	}
	unsigned char *pbuf  = new  unsigned char[buflen];
	memcpy(pbuf,m_h264Buf,buflen);
	char sendbuf[1400];
	memset(sendbuf,0,1400);
	int status;	
	if (buflen<=MAX_RTP_PKT_LENGTH)
	{	
		memcpy(sendbuf,pbuf,buflen);		
		//status = sess.SendPacket((void *)sendbuf,buflen/*,payload,false,timestamp*/);	
		
		status = sess.SendPacket((void *)sendbuf,buflen,96,true,3600);	
		
		checkerror(status);
	}	
	else if(buflen>=MAX_RTP_PKT_LENGTH)
	{
		//TRACE("buflen = %d\n",buflen);
		//得到该nalu需要用多少长度为MAX_RTP_PKT_LENGTH字节的RTP包来发送
		int k=0,l=0;		
		k=buflen/MAX_RTP_PKT_LENGTH;
		l=buflen%MAX_RTP_PKT_LENGTH;
		int t=0;//用于指示当前发送的是第几个分片RTP包		
		char nalHeader =pbuf[0]; // NALU 头
		while(t<k||(t==k&&l>0))				
		{	
			if((0==t)||(t<k&&0!=t))//第一包到最后一包的前一包
			{
				sendbuf[0] = (nalHeader & 0x60)|28;				
				sendbuf[1] = (nalHeader & 0x1f);
				if (0==t)
				{
					sendbuf[1] |= 0x80;
				}
				memcpy(sendbuf+2,&pbuf[t*MAX_RTP_PKT_LENGTH],MAX_RTP_PKT_LENGTH+2);
				status = sess.SendPacket((void *)sendbuf,MAX_RTP_PKT_LENGTH+2,96,false,0);		
				/*printf("%x  \n",sendbuf[0]);
				printf("%x  \n",sendbuf[1]);
				printf("%x  \n",sendbuf[2]);
				printf("%x  \n",sendbuf[3]);*/
				checkerror(status);
				t++;
			}
			//最后一包
			else if((k==t&&l>0)||(t==(k-1)&&l==0))
			{
				int iSendLen;
				if (l>0)
				{
					iSendLen = buflen-t*MAX_RTP_PKT_LENGTH;
				}
				else
					iSendLen = MAX_RTP_PKT_LENGTH;

				sendbuf[0] = (nalHeader & 0x60)|28;				

				sendbuf[1] = (nalHeader & 0x1f);

				sendbuf[1] |= 0x40;
				memcpy(sendbuf+2,&pbuf[t*MAX_RTP_PKT_LENGTH],iSendLen+2);

				
				//TRACE("start = %d,len = %d\n",t*MAX_RTP_PKT_LENGTH,iSendLen);
				status = sess.SendPacket((void *)sendbuf,iSendLen+2,96,true,3600);

				checkerror(status);
				t++;
			}
		}
#ifndef RTP_SUPPORT_THREAD
		status = sess.Poll();
		checkerror(status);
#endif 
		RTPTime::Wait(RTPTime(0,500));   //第一个参数为秒，第二个参数为毫秒
	}
	delete []pbuf;	
}
void CRtpSend::CloseSocket()
{
	if (bInitRTP)
	{
		//发送一个BYE包离开会话，最多等待3秒钟，超时则不发送

		sess.BYEDestroy(RTPTime(3,0),0,0);
#ifdef WIN32

		WSACleanup();

#endif // WIN32

	}
}
bool CRtpSend::checkerror(int rtperr)
{
	if (rtperr < 0)
	{
		printf("ERROR:%s\n",RTPGetErrorString(rtperr));		
		return false;
	}
	return true;
}