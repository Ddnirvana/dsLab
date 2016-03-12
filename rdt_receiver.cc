/*
 * FILE: rdt_receiver.cc
 * DESCRIPTION: Reliable data transfer receiver.
 * NOTE: This implementation assumes there is no packet loss, corruption, or 
 *       reordering.  You will need to enhance it to deal with all these 
 *       situations.  In this implementation, the packet format is laid out as 
 *       the following:
 *       
 *       |<-  1 byte  ->|<-             the rest            ->|
 *       | payload size |<-             payload             ->|
 *
 *       The first byte of each packet indicates the size of the payload
 *       (excluding this single-byte header)
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rdt_struct.h"
#include "rdt_receiver.h"

#define FLAG_SIZE 100
struct Dd_windows2{
	unsigned int id;
	int size;
	char *data;
	char flag[FLAG_SIZE];
	int lastseq;
	struct Dd_windows2 * next;
};
struct Dd_windows2 * dd_recwind;
static unsigned int dd_waitfor=0;
static int tot_acks=0;
FILE * rec_log;
void checkup();
/*
 * add by Dd to record the packet into the message windows
 */
void packtomsg(struct packet * pkt)
{
	unsigned int msgid= (unsigned char)pkt->data[3] + (((unsigned char)pkt->data[4])<<8);
	if (msgid<dd_waitfor) return; // to avoid add some packets in the wind again
	struct Dd_windows2 * tmpwind=dd_recwind;
	for (;(tmpwind) && (tmpwind->id!=msgid);tmpwind=tmpwind->next);
	while (tmpwind){
		if (tmpwind->id==msgid) break;
		tmpwind=tmpwind->next;
	}
	if (!tmpwind){
		//fprintf(rec_log,"new message is in:%d\n",msgid);
		//checkup();
		tmpwind=(struct Dd_windows2*) malloc(sizeof(struct Dd_windows2));
		tmpwind->size=0;
		tmpwind->id=msgid;
		tmpwind->data=NULL;
		tmpwind->next=NULL;
		for (int j=0;j<FLAG_SIZE;tmpwind->flag[j]=0,j++);
		tmpwind->lastseq=-1;
		if (!dd_recwind) dd_recwind=tmpwind;
		else {
			struct Dd_windows2 *i=dd_recwind;
			struct Dd_windows2 *pre=NULL;
			for (;(i)&&(i->id<msgid) ;pre=i, i=i->next);
		//	if (pre) fprintf(rec_log,"pre:%d",pre->id);
		//	if (i) fprintf(rec_log,"next:%d",i->id);
		//	fprintf(rec_log,"\n");
			if (!pre){
				tmpwind->next= dd_recwind;
				dd_recwind=tmpwind;
			}else {
				tmpwind->next=pre->next;
				pre->next = tmpwind;
			}
				
		}
	}
	int pac_seq= pkt->data[1];
	int header_size = 5;
	int maxpayload_size= RDT_PKTSIZE- header_size;
	int packetsize=pkt->data[0];
	int targetSize= maxpayload_size * pac_seq + packetsize;
	if (tmpwind->flag[pac_seq]) return; //to avoid add packet again
	if (tmpwind->size < targetSize){
		tmpwind->size=targetSize;
		tmpwind->data=(char *)realloc(tmpwind->data,targetSize);
	}
	memcpy(tmpwind->data+maxpayload_size * pac_seq,pkt->data+ header_size,packetsize);
	tmpwind->flag[pac_seq] =1;
	if (pkt->data[2]==2) tmpwind->lastseq=pac_seq;
}

/* receiver initialization, called once at the very beginning */
void Receiver_Init()
{
    dd_recwind=NULL;
    rec_log=fopen("rec_log","w");
    fprintf(stdout, "At %.2fs: receiver initializing ...\n", GetSimulationTime());
}

/*
 * add by Dd to check if the windows is consistent up
 */
void checkup()
{
	struct Dd_windows2 * tmpwind=dd_recwind;
	fprintf(rec_log,"check up\n");
	unsigned int least=0;
	bool isvalid=0;
	while (tmpwind){
		fprintf(rec_log,"%u\n",tmpwind->id);
		if (tmpwind->id>=least) least=tmpwind->id;
		else isvalid=1;
		tmpwind=tmpwind->next;
	}
	if (isvalid) fprintf(rec_log,"opps, up consistent is violate!\n");
	fflush(rec_log);
}

/* receiver finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to use this opportunity to release some 
   memory you allocated in Receiver_init(). */
void Receiver_Final()
{
    struct Dd_windows2 * tmpwind=NULL;
    //checkup();
    if(dd_recwind){
    	while (dd_recwind){
    		tmpwind=dd_recwind;
		dd_recwind=dd_recwind->next;
		free(tmpwind->data);
		free(tmpwind);
    	}
    	fprintf(rec_log,"opps,windows is not empty\n");
    }
    fprintf(stdout,"Dd: total ack packets:%d\n",tot_acks);
    fprintf(stdout, "At %.2fs: receiver finalizing ...\n", GetSimulationTime());
    fclose(rec_log);
}


/*
 * add by Dd
 * receiver to send back tha ack packet back to sender
 */
void Receiver_SendBackAck(unsigned int  msgid){
	//int header_size=5;
	//int maxpayload_size = RDT_PKTSIZE -header_size;
	int acknum=1;
	packet pkt;

	pkt.data[0] = 0;
	pkt.data[1] = 0;
	pkt.data[2] = 1;
	pkt.data[3] = msgid &&  0xff;
	pkt.data[4] = (msgid>>8) & 0xff;
	for (int i=0;i<acknum;i++){
		Receiver_ToLowerLayer(&pkt);
		tot_acks++;
	}
}
int couldSendUp(){
	if (!dd_recwind) return false;
	if ((dd_recwind->id==dd_waitfor) && (dd_recwind->lastseq==-2))
	{
		fprintf(rec_log,"messgae :%d sent up\n",dd_waitfor);
		dd_waitfor++;
		return true;
	}
	return false;
}
void checkfinish(){
	struct Dd_windows2 * tmpwind=dd_recwind;
	while (tmpwind){
		if (tmpwind->lastseq>=0)
		{
			int i;
			for (i=0;i<=tmpwind->lastseq;i++) if (!tmpwind->flag[i]) break;
			if (i>tmpwind->lastseq) {
				Receiver_SendBackAck(tmpwind->id);
				tmpwind->lastseq=-2;
			}
		}
		tmpwind=tmpwind->next;
	}
	while (couldSendUp()){
    		/* construct a message and deliver to the upper layer */
	    struct message *msg = (struct message*) malloc(sizeof(struct message));
	    ASSERT(msg!=NULL);
	    msg->size = dd_recwind->size;
	    /* sanity check in case the packet is corrupted */
	    if (msg->size<0) msg->size=0;
	    msg->data = (char*) malloc(msg->size);
	    ASSERT(msg->data!=NULL);
	    memcpy(msg->data, dd_recwind->data, msg->size);
	    Receiver_ToUpperLayer(msg);

	    /* don't forget to free the space */
	    if (msg->data!=NULL) free(msg->data);
	    if (msg!=NULL) free(msg);
	    struct Dd_windows2 * tmpwind=dd_recwind;
	    dd_recwind=dd_recwind->next;
	    if (tmpwind->data) free(tmpwind->data);
	    free(tmpwind);
	}
}
/* event handler, called when a packet is passed from the lower layer at the 
   receiver */
void Receiver_FromLowerLayer(struct packet *pkt)
{
	packtomsg(pkt);
	checkfinish();
#if 0  
    /* 1-byte header indicating the size of the payload */
    int header_size = 1;

    /* construct a message and deliver to the upper layer */
    struct message *msg = (struct message*) malloc(sizeof(struct message));
    ASSERT(msg!=NULL);

    msg->size = pkt->data[0];

    /* sanity check in case the packet is corrupted */
    if (msg->size<0) msg->size=0;
    if (msg->size>RDT_PKTSIZE-header_size) msg->size=RDT_PKTSIZE-header_size;

    msg->data = (char*) malloc(msg->size);
    ASSERT(msg->data!=NULL);
    memcpy(msg->data, pkt->data+header_size, msg->size);
    Receiver_ToUpperLayer(msg);

    /* don't forget to free the space */
    if (msg->data!=NULL) free(msg->data);
    if (msg!=NULL) free(msg);
#endif
}
