/**
*Function：把h264和AAC封包为RTP包
*Author：FreeFly
*Email：863973433@qq.com
*Time: 2016.11.9
**/
#include "RtpPack.h"
#pragma comment(lib,"ws2_32.lib") 
RtpPack::RtpPack(void)
{
}
RtpPack::~RtpPack(void)
{
}
//为NALU_t结构体分配内存空间  
NALU_t *RtpPack::AllocNALU(int buffersize)  
{  
    NALU_t *n;   
    if ((n = (NALU_t*)calloc (1, sizeof (NALU_t))) == NULL)  
    {  
        printf("AllocNALU: n");  
        exit(0);  
    }   
    n->max_size=buffersize;   
    if ((n->buf = (char*)calloc (buffersize, sizeof (char))) == NULL)  
    {  
        free (n);  
        printf ("AllocNALU: n->buf");  
        exit(0);  
    }  
    return n;  
}  
//释放  
void RtpPack::FreeNALU(NALU_t *n)  
{  
    if (n)  
    {  
        if (n->buf)  
        {  
            free(n->buf);  
            n->buf=NULL;  
        }  
        free (n);  
    }  
}  
bool RtpPack::FindStartCode2( unsigned char *pdata )
{
	 if(pdata[0]!=0 || pdata[1]!=0 || pdata[2] !=1) 
		 return 0; 
	 else
		 return 1;
}
bool RtpPack::FindStartCode3( unsigned char *pdata )
{
	
	if(pdata[0]!=0 || pdata[1]!=0 || pdata[2] !=0 || pdata[3] !=1) 
		return 0;
	else 
		return 1;  
}
//输出NALU的 长度和类型
void RtpPack::PrintNaluTypeAndLen(NALU_t *n)
{
	if (!n)return;    
    printf(" len: %d  ", n->len);  
    printf("nal_unit_type: %x\n", n->nal_unit_type);
}
int RtpPack::GetOneNALU (NALU_t *nalu ,unsigned char *pdata ,unsigned long nal_size)
{
    nalu->startcodeprefix_len=3; 
    if(FindStartCode2 (pdata))   
    {     
        nalu->startcodeprefix_len = 3;   
    } 
	if(FindStartCode3 (pdata))   
    {     
        nalu->startcodeprefix_len = 4;   
    }
    nalu->len =nal_size - nalu->startcodeprefix_len;      
    memcpy (nalu->buf, &pdata[nalu->startcodeprefix_len], nalu->len);
    nalu->forbidden_bit = nalu->buf[0] & 0x80; //1 bit  
    nalu->nal_reference_idc = nalu->buf[0] & 0x60; // 2 bit  
    nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;// 5 bit  
  
    return nal_size;//有前缀的NALU的长度  
}
bool RtpPack::SetH264RtpPack ( unsigned char *NAL_Buf, unsigned long NAL_Size)
{
	NALU_t *n;  
	n = AllocNALU(MAX_RTP_PKT_LENGTH);
	char* nalu_payload;    
	char sendbuf[1500];  
	int RtpLength=0;
	unsigned short seq_num =0;  
	float framerate=25;  
	unsigned int timestamp_increse=0,ts_current=0;  
	timestamp_increse=(unsigned int)(90000.0 / framerate);  //时间戳，H264的视频设置成90000
	GetOneNALU(n ,NAL_Buf, NAL_Size);
	PrintNaluTypeAndLen(n); 
	memset(sendbuf,0,1500);
	rtp_hdr =(RTP_HEAHER*)&sendbuf[0];   
	//设置RTP HEADER，  
	rtp_hdr->PT  = H264_PAYLOADTYPE;  //负载类型号，  
	rtp_hdr->V   = 2;   //版本号， 
	rtp_hdr->M   = 0;   //标志位， 
	rtp_hdr->unSSRC = htonl(10);     

	if(n->len < 1400)
	{
		//设置rtp M 位；  
		rtp_hdr->M = 1;  
		rtp_hdr->nSeqNum  = htons(seq_num ++); //htons，主机字节序转成网络字节序。  
		
		nalu_hdr =(NALU_HEAHER*)&sendbuf[12]; 
		nalu_hdr->F = n->forbidden_bit;  
		nalu_hdr->NRI=n->nal_reference_idc>>5;  
		nalu_hdr->Type=n->nal_unit_type;  

		nalu_payload=&sendbuf[13];  
		memcpy(nalu_payload,n->buf+1,n->len-1); 

		ts_current = ts_current + timestamp_increse;  
		rtp_hdr->unTimestamp=htonl(ts_current);  
		RtpLength=n->len + 12 ;   
		//发送RTP包
		/***
		* sendbuf      rtp包数据
		* RtpLength    包长度 为nalu长度加 rtp 头的固定长度 12字节
		***/
		SendH264RtpPack(sendbuf, RtpLength);
	}
	else if(n->len > 1400)   
	{  
		int k = 0, last = 0;  
		k = n->len / 1400;
		last = n->len % 1400;
		int t = 0;//用于指示当前发送的是第几个分片RTP包  
		ts_current = ts_current + timestamp_increse;  
		rtp_hdr->unTimestamp = htonl(ts_current);  
		while(t <= k)  
		{  
			rtp_hdr->nSeqNum = htons(seq_num++); //序列号，每发送一个RTP包增1  
			if(!t) 
			{  
				//设置rtp M 位；  
				rtp_hdr->M = 0; 
				
				fu_ind =(FU_INDICATOR*)&sendbuf[12]; 
				fu_ind->F = n->forbidden_bit;  
				fu_ind->NRI = n->nal_reference_idc >> 5;  
				fu_ind->Type = 28;  //FU-A类型。  
	
				fu_hdr =(FU_HEADER*)&sendbuf[13];  
				fu_hdr->E = 0;  
				fu_hdr->R = 0;  
				fu_hdr->S = 1;  
				fu_hdr->Type = n->nal_unit_type;  

				nalu_payload = &sendbuf[14];
				memcpy(nalu_payload,n->buf+1,1400);//去掉NALU头，每次拷贝1400个字节。  

				RtpLength = 1400 + 14;                                                           
				//发送RTP包
				/***
				* sendbuf      rtp包数据
				* RtpLength    包长度 
				***/
				SendH264RtpPack(sendbuf, RtpLength);
				t++;  
			}  	
			else if(k == t)
			{  
				//设置rtp M 位；当前传输的是最后一个分片时该位置1  
				rtp_hdr->M=1;   
				//设置FU INDICATOR,并将这个HEADER填入sendbuf[12]  
				fu_ind =(FU_INDICATOR*)&sendbuf[12]; 
				fu_ind->F=n->forbidden_bit;  
				fu_ind->NRI=n->nal_reference_idc>>5;  
				fu_ind->Type=28;  

				//设置FU HEADER,并将这个HEADER填入sendbuf[13]  
				fu_hdr = (FU_HEADER*)&sendbuf[13];  
				fu_hdr->R = 0;  
				fu_hdr->S = 0;  
				fu_hdr->Type = n->nal_unit_type;  
				fu_hdr->E = 1;  

				nalu_payload = &sendbuf[14];
				memcpy(nalu_payload,n->buf + t*1400 + 1,last-1);  
				RtpLength = last - 1 + 14;      
				//发送RTP包
				/***
				* sendbuf      rtp包数据
				* RtpLength    包长度
				***/
				SendH264RtpPack(sendbuf, RtpLength);
				t++;  		 
			}  
			//既不是第一个分片，也不是最后一个分片的处理。  
			else if(t < k && 0 != t)  
			{  
				//设置rtp M 位；  
				rtp_hdr->M = 0;  
				//设置FU INDICATOR,并将这个HEADER填入sendbuf[12]  
				fu_ind = (FU_INDICATOR*)&sendbuf[12];   
				fu_ind->F = n->forbidden_bit;  
				fu_ind->NRI = n->nal_reference_idc>>5;  
				fu_ind->Type = 28;  

				//设置FU HEADER,并将这个HEADER填入sendbuf[13]  
				fu_hdr =(FU_HEADER*)&sendbuf[13];  

				fu_hdr->R = 0;  
				fu_hdr->S = 0;  
				fu_hdr->E = 0;  
				fu_hdr->Type = n->nal_unit_type;  

				nalu_payload=&sendbuf[14];
				memcpy(nalu_payload, n->buf + t * 1400 + 1,1400);
				RtpLength=1400 + 14;                      

				//发送RTP包
				/***
				* sendbuf      rtp包数据
				* RtpLength    包长度 
				***/
				SendH264RtpPack(sendbuf, RtpLength);
				t++;  
			}  
		}  
	}   
	FreeNALU(n);  
	return true;  
}  
bool RtpPack::SetAACRtpPack( unsigned char *AAC_Buf, unsigned long AAC_Size)
{
	//将AAC的 ADTS头去掉 占七个字节
	unsigned char * aac_payload =AAC_Buf+7;  
	unsigned long payload_size = AAC_Size - 7;
	//加12字节的RTP头
	char sendbuf[1500];
	unsigned short seq_num =0;  
	unsigned int timestamp_increse=0,ts_current=0;  
	timestamp_increse=(unsigned int)(90000.0);  //时间戳，设置成90000
	memset(sendbuf,0,1500);
	rtp_AAChdr =(RTP_HEAHER*)&sendbuf[0];   
	//设置RTP HEADER，  
	rtp_AAChdr->PT  = AAC_PAYLOADTYPE;  //负载类型号，  
	rtp_AAChdr->V   = 2;   //版本号， 
	rtp_AAChdr->M   = 1;   //标志位， 
	rtp_AAChdr->unSSRC = htonl(10);         
	rtp_AAChdr->nSeqNum  = htons(seq_num ++); //htons，主机字节序转成网络字节序。  
	ts_current = ts_current + timestamp_increse;  
	rtp_AAChdr->unTimestamp=htonl(ts_current);  

	//加四个字节的AU_HEADER_SIZE
    uint8_t *packet = new unsigned char[4];  
	memset(packet,0,4);
    packet[0] = 0x00;  
    packet[1] = 0x10;  
    packet[2] = (payload_size & 0x1fe0) >> 5;  
    packet[3] = (payload_size & 0x1f) << 3;  
	memcpy(&sendbuf+12,packet,4); 
	//继续加aac数据
	memcpy(&sendbuf+12+4,aac_payload,payload_size); 
	payload_size = payload_size +12+4;
	//打包完成发送AAC RTP包
	/***
	* sendbuf        rtp包数据
	* payload_size   包长度 
	***/
	SendAACRtpPack(sendbuf, payload_size);
    return true;  
}
bool RtpPack::SendH264RtpPack(char *rtp_buf, unsigned long rtp_size)
{
	rtp_buf;
	rtp_size;
	//progress
	return true;
}
bool RtpPack::SendAACRtpPack(char *rtp_buf, unsigned long rtp_size)
{
	//progress
	return true;
}