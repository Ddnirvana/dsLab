#include "rdt_struct.h"
#include "checksum.h"
#if 0
/*
 * naive implementation, may have conflict
 */
void checksum(struct packet * pkt){
	pkt->data[5]=1;
	pkt->data[6]=2;
	unsigned int tmp1=0,tmp2=0;
	for (int i=0;i<RDT_PKTSIZE/2;i++)
		tmp1+=pkt->data[i];
	for (int i=RDT_PKTSIZE/2;i<RDT_PKTSIZE;i++)
		tmp2+=pkt->data[i];
	pkt->data[5]=tmp1 & 0xff;
	pkt->data[6]=tmp2 & 0xff;
}

bool check_ckm(struct packet *pkt){
	unsigned int tmp1=0,tmp2=0;
	char ans1,ans2;
	ans1=pkt->data[5];
	ans2=pkt->data[6];
	pkt->data[5]=1;
	pkt->data[6]=2;
	for (int i=0;i<RDT_PKTSIZE/2;i++) tmp1+=pkt->data[i];
	for (int i=RDT_PKTSIZE/2;i<RDT_PKTSIZE;i++) tmp2+=pkt->data[i];
	if (ans1!=(char)(tmp1&0xff)) return false;	
	if (ans2!=(char)(tmp2&0xff)) return false;
	pkt->data[5]=ans1;
	pkt->data[6]=ans2;
	return true;
}
#endif

unsigned int checksum_8(unsigned int cksum, char *data,unsigned int size){
	char num=0;
	unsigned char *p=(unsigned char *)data;
	if ((NULL == data) || (0==size))
		return cksum;
	while (size>1) {
		cksum+=(((unsigned short) p[num])<<8 & 0xff00) | ((unsigned short)p[num+1]) & 0xff;
		size -=2;
		num +=2;
	}
	if (size>0)
	{
		cksum +=(((unsigned short)p[num]) <<8) & 0xffff;
		num+=1;
	}

	while (cksum >> 16)
		cksum=(cksum & 0xffff) +(cksum>>16);
	return ~cksum;
}

void checksum(struct packet *pkt){
	pkt->data[6]=0;
	pkt->data[7]=0;
	pkt->data[5]=0x08;
	pkt->data[8]=0x27;
	unsigned short chksum= checksum_8(0,pkt->data,RDT_PKTSIZE);
	pkt->data[6]=chksum >>8;
	pkt->data[7]=chksum & 0xff;
	unsigned int tmp1=0x08,tmp2=0x27;
	for (int i=0;i<RDT_PKTSIZE/2;i++) tmp1+=pkt->data[i];
	for (int i=RDT_PKTSIZE/2;i<RDT_PKTSIZE;i++) tmp2+=pkt->data[i];
	pkt->data[5]= (tmp1 &0xff);
	pkt->data[8]= (tmp2 &0xff);
}
bool check_ckm(struct packet *pkt){
	char tmp1=pkt->data[5];
	char tmp2=pkt->data[8];
	pkt->data[5]=0x08;
	pkt->data[8]=0x27;
	unsigned short chksum=checksum_8(0,pkt->data,RDT_PKTSIZE);
	if (chksum){
		pkt->data[5]=tmp1;
		pkt->data[8]=tmp2;
		return false;
	}else{
		unsigned int tmp1_t=0x08,tmp2_t=0x27;
		for (int i=0;i<RDT_PKTSIZE/2;i++) tmp1_t+=pkt->data[i];
		for (int i=RDT_PKTSIZE/2;i<RDT_PKTSIZE;i++) tmp2_t+=pkt->data[i];
		if ((tmp1 == (char)(tmp1_t & 0xff)) && ( tmp2==(char)(tmp2_t & 0xff))){
			pkt->data[5]=tmp1;
			pkt->data[8]=tmp2;
			return true;
		}
		pkt->data[5]=tmp1;
		pkt->data[8]=tmp2;
		return false;
	}
}
