/**
*Function：把h264和AAC封包为RTP包
*Author：FreeFly
*Email：863973433@qq.com
*Time: 2016.11.9
**/
#pragma once
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <winsock2.h> 
using namespace std;
#define MAX_RTP_PKT_LENGTH            1400  
#define H264_PAYLOADTYPE              96 
#define AAC_PAYLOADTYPE               97 
#define AU_HEADER_SIZE                4

//NALU_INFO
typedef struct  
{  
    int startcodeprefix_len;     
    unsigned len;                
    unsigned max_size;           
    int forbidden_bit;           
    int nal_reference_idc;       
    int nal_unit_type;           
    char *buf;                   
    unsigned short lost_packets; 
} NALU_t;  

//RTP Header 
typedef struct tagRTPHEAHER
{
	unsigned char V:2;//版本
	unsigned char P:1;//填充
	unsigned char X:1;//扩展
	unsigned char CC:4;//CSRC计数
	unsigned char M:1;//
	unsigned char PT:7; //负载类型
	unsigned short nSeqNum; //序列号
	unsigned int  unTimestamp;
	unsigned int  unSSRC;//同步源

}RTP_HEAHER;

//Nalu Header
typedef struct tagNALUHEAHER
{
	unsigned char F:1; 
	unsigned char NRI:2;
	unsigned char Type:5;

}NALU_HEAHER;

//FU INDICATOR
typedef struct tagFUINDICATOR
{
	unsigned char F:1; 
	unsigned char NRI:2;
	unsigned char Type:5;

}FU_INDICATOR;
//Fu Header
typedef struct tagFUHEADER
{
	unsigned char S:1;
	unsigned char E:1;
	unsigned char R:1;
	unsigned char Type:5;

}FU_HEADER;

class RtpPack
{
public:
	RtpPack(void);
	~RtpPack(void);
public: 
    bool SetH264RtpPack ( unsigned char *NAL_Buf, unsigned long NAL_Size);
    bool SetAACRtpPack  ( unsigned char *AAC_Buf, unsigned long AAC_Size);
	bool SendH264RtpPack(char *rtp_buf, unsigned long rtp_size);
	bool SendAACRtpPack (char *aac_buf, unsigned long aac_size);
private:
	bool FindStartCode2(unsigned char *pdata );
	bool FindStartCode3(unsigned char *pdata );
	void PrintNaluTypeAndLen(NALU_t *n); 
	NALU_t *AllocNALU(int buffersize);
	void FreeNALU(NALU_t *n);
	int GetOneNALU (NALU_t *nalu,unsigned char *pdata ,unsigned long nal_size);  
private:
	RTP_HEAHER        *rtp_hdr;  
	RTP_HEAHER        *rtp_AAChdr; 
	NALU_HEAHER       *nalu_hdr;  
	FU_INDICATOR      *fu_ind;  
	FU_HEADER         *fu_hdr;
};
