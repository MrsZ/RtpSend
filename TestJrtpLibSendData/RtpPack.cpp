/**
*Function����h264��AAC���ΪRTP��
*Author��FreeFly
*Email��863973433@qq.com
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
//ΪNALU_t�ṹ������ڴ�ռ�  
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
//�ͷ�  
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
//���NALU�� ���Ⱥ�����
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
  
    return nal_size;//��ǰ׺��NALU�ĳ���  
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
	timestamp_increse=(unsigned int)(90000.0 / framerate);  //ʱ�����H264����Ƶ���ó�90000
	GetOneNALU(n ,NAL_Buf, NAL_Size);
	PrintNaluTypeAndLen(n); 
	memset(sendbuf,0,1500);
	rtp_hdr =(RTP_HEAHER*)&sendbuf[0];   
	//����RTP HEADER��  
	rtp_hdr->PT  = H264_PAYLOADTYPE;  //�������ͺţ�  
	rtp_hdr->V   = 2;   //�汾�ţ� 
	rtp_hdr->M   = 0;   //��־λ�� 
	rtp_hdr->unSSRC = htonl(10);     

	if(n->len < 1400)
	{
		//����rtp M λ��  
		rtp_hdr->M = 1;  
		rtp_hdr->nSeqNum  = htons(seq_num ++); //htons�������ֽ���ת�������ֽ���  
		
		nalu_hdr =(NALU_HEAHER*)&sendbuf[12]; 
		nalu_hdr->F = n->forbidden_bit;  
		nalu_hdr->NRI=n->nal_reference_idc>>5;  
		nalu_hdr->Type=n->nal_unit_type;  

		nalu_payload=&sendbuf[13];  
		memcpy(nalu_payload,n->buf+1,n->len-1); 

		ts_current = ts_current + timestamp_increse;  
		rtp_hdr->unTimestamp=htonl(ts_current);  
		RtpLength=n->len + 12 ;   
		//����RTP��
		/***
		* sendbuf      rtp������
		* RtpLength    ������ Ϊnalu���ȼ� rtp ͷ�Ĺ̶����� 12�ֽ�
		***/
		SendH264RtpPack(sendbuf, RtpLength);
	}
	else if(n->len > 1400)   
	{  
		int k = 0, last = 0;  
		k = n->len / 1400;
		last = n->len % 1400;
		int t = 0;//����ָʾ��ǰ���͵��ǵڼ�����ƬRTP��  
		ts_current = ts_current + timestamp_increse;  
		rtp_hdr->unTimestamp = htonl(ts_current);  
		while(t <= k)  
		{  
			rtp_hdr->nSeqNum = htons(seq_num++); //���кţ�ÿ����һ��RTP����1  
			if(!t) 
			{  
				//����rtp M λ��  
				rtp_hdr->M = 0; 
				
				fu_ind =(FU_INDICATOR*)&sendbuf[12]; 
				fu_ind->F = n->forbidden_bit;  
				fu_ind->NRI = n->nal_reference_idc >> 5;  
				fu_ind->Type = 28;  //FU-A���͡�  
	
				fu_hdr =(FU_HEADER*)&sendbuf[13];  
				fu_hdr->E = 0;  
				fu_hdr->R = 0;  
				fu_hdr->S = 1;  
				fu_hdr->Type = n->nal_unit_type;  

				nalu_payload = &sendbuf[14];
				memcpy(nalu_payload,n->buf+1,1400);//ȥ��NALUͷ��ÿ�ο���1400���ֽڡ�  

				RtpLength = 1400 + 14;                                                           
				//����RTP��
				/***
				* sendbuf      rtp������
				* RtpLength    ������ 
				***/
				SendH264RtpPack(sendbuf, RtpLength);
				t++;  
			}  	
			else if(k == t)
			{  
				//����rtp M λ����ǰ����������һ����Ƭʱ��λ��1  
				rtp_hdr->M=1;   
				//����FU INDICATOR,�������HEADER����sendbuf[12]  
				fu_ind =(FU_INDICATOR*)&sendbuf[12]; 
				fu_ind->F=n->forbidden_bit;  
				fu_ind->NRI=n->nal_reference_idc>>5;  
				fu_ind->Type=28;  

				//����FU HEADER,�������HEADER����sendbuf[13]  
				fu_hdr = (FU_HEADER*)&sendbuf[13];  
				fu_hdr->R = 0;  
				fu_hdr->S = 0;  
				fu_hdr->Type = n->nal_unit_type;  
				fu_hdr->E = 1;  

				nalu_payload = &sendbuf[14];
				memcpy(nalu_payload,n->buf + t*1400 + 1,last-1);  
				RtpLength = last - 1 + 14;      
				//����RTP��
				/***
				* sendbuf      rtp������
				* RtpLength    ������
				***/
				SendH264RtpPack(sendbuf, RtpLength);
				t++;  		 
			}  
			//�Ȳ��ǵ�һ����Ƭ��Ҳ�������һ����Ƭ�Ĵ���  
			else if(t < k && 0 != t)  
			{  
				//����rtp M λ��  
				rtp_hdr->M = 0;  
				//����FU INDICATOR,�������HEADER����sendbuf[12]  
				fu_ind = (FU_INDICATOR*)&sendbuf[12];   
				fu_ind->F = n->forbidden_bit;  
				fu_ind->NRI = n->nal_reference_idc>>5;  
				fu_ind->Type = 28;  

				//����FU HEADER,�������HEADER����sendbuf[13]  
				fu_hdr =(FU_HEADER*)&sendbuf[13];  

				fu_hdr->R = 0;  
				fu_hdr->S = 0;  
				fu_hdr->E = 0;  
				fu_hdr->Type = n->nal_unit_type;  

				nalu_payload=&sendbuf[14];
				memcpy(nalu_payload, n->buf + t * 1400 + 1,1400);
				RtpLength=1400 + 14;                      

				//����RTP��
				/***
				* sendbuf      rtp������
				* RtpLength    ������ 
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
	//��AAC�� ADTSͷȥ�� ռ�߸��ֽ�
	unsigned char * aac_payload =AAC_Buf+7;  
	unsigned long payload_size = AAC_Size - 7;
	//��12�ֽڵ�RTPͷ
	char sendbuf[1500];
	unsigned short seq_num =0;  
	unsigned int timestamp_increse=0,ts_current=0;  
	timestamp_increse=(unsigned int)(90000.0);  //ʱ��������ó�90000
	memset(sendbuf,0,1500);
	rtp_AAChdr =(RTP_HEAHER*)&sendbuf[0];   
	//����RTP HEADER��  
	rtp_AAChdr->PT  = AAC_PAYLOADTYPE;  //�������ͺţ�  
	rtp_AAChdr->V   = 2;   //�汾�ţ� 
	rtp_AAChdr->M   = 1;   //��־λ�� 
	rtp_AAChdr->unSSRC = htonl(10);         
	rtp_AAChdr->nSeqNum  = htons(seq_num ++); //htons�������ֽ���ת�������ֽ���  
	ts_current = ts_current + timestamp_increse;  
	rtp_AAChdr->unTimestamp=htonl(ts_current);  

	//���ĸ��ֽڵ�AU_HEADER_SIZE
    uint8_t *packet = new unsigned char[4];  
	memset(packet,0,4);
    packet[0] = 0x00;  
    packet[1] = 0x10;  
    packet[2] = (payload_size & 0x1fe0) >> 5;  
    packet[3] = (payload_size & 0x1f) << 3;  
	memcpy(&sendbuf+12,packet,4); 
	//������aac����
	memcpy(&sendbuf+12+4,aac_payload,payload_size); 
	payload_size = payload_size +12+4;
	//�����ɷ���AAC RTP��
	/***
	* sendbuf        rtp������
	* payload_size   ������ 
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