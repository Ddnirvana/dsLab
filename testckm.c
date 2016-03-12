#include <stdlib.h>
#include <stdio.h>

unsigned char ip_hdr_8[] ={
	0x45,0x00,
	0x00,0x3c,
	0x00,0x00,
	0x00,0x00,
	0x40,0x01,
	0x00,0x00,//cksm
	0x00,0xa8,
	0x01,0x25,
	0xda,0x3c,
	0x06,0x82,
	0x11
};

unsigned int checksum_8(unsigned int cksum, char *data,unsigned int size){
	char num=0;
	unsigned char *p=data;
	if ((NULL == data) || (0==size))
		return cksum;
	while (size>1) {
		cksum+=((unsigned short) p[num]<<8 & 0xff00) | (unsigned short)p[num+1] & 0xff;
		size -=2;
		num +=2;
	}
	if (size>0)
	{
		cksum +=((unsigned short)p[num] <<8) & 0xffff;
		num+=1;
	}

	while (cksum >> 16)
		cksum=(cksum & 0xffff) +(cksum>>16);
	return ~cksum;
}
void main()
{
	unsigned short chksum;
	chksum = checksum_8(0,ip_hdr_8,sizeof(ip_hdr_8));
	printf("checksum =0x%04x \n",chksum);
	ip_hdr_8[11] = chksum >>8;
	ip_hdr_8[12] = chksum & 0xff;

	chksum = checksum_8(0,ip_hdr_8,21);
	if (chksum)
		printf("chksum is incorrect\n");
	else
		printf("chksum is correct\n");
}
