/*
 * FILE: rdt_sender.cc
 * DESCRIPTION: Reliable data transfer sender.
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


/*
 * packet layout:
 * |<- 1 byte ->|<- 1 byte ->|<- 1 byte->|<- 2 byte ->|<-2 byte->|<-  the rest ->|
 * |payload size|   seq no   |  ack      |  mesage no |  checksum|payload    |
 *
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rdt_struct.h"
#include "rdt_sender.h"
#include "checksum.h"

/*
 * add by Dd
 * origin from then message struct
 * changed it to a chain of all the message
 */
#if 1
struct Dd_windows{ 
	unsigned int id;
	int size;
	char *data;
	struct Dd_windows * next;
};
#endif

struct Dd_VTimer{
	unsigned int id;
	double timer;
	struct Dd_VTimer *next;
};
struct Dd_VTimer * f_timer;
struct Dd_VTimer * l_timer;
struct Dd_windows* dd_sendwind;

static unsigned int tot_message=0;
static int tot_packets=0;
FILE * send_log;
/* ADD BY Dd TO add a message into the gloabl window*/
/* add message to the end of the wind, but we can make sure it's upforward */
void msgtowind(struct message * msg)
{
	struct Dd_windows* tmpwind= (struct Dd_windows*) malloc(sizeof(struct Dd_windows));
	
	tmpwind->id=tot_message++;
	tmpwind->size=msg->size;
	tmpwind->data=(char *) malloc(tmpwind->size);
	memcpy(tmpwind->data,msg->data,msg->size);
	tmpwind->next = NULL;
	
	if (!dd_sendwind)
		dd_sendwind = tmpwind;
	else{
		struct Dd_windows *i;
		for (i=dd_sendwind;i->next !=NULL;i=i->next);
		i->next= tmpwind;
	}
}

/* remove a message from the windows with the message id*/
void msgoutwind(unsigned int msgid){
	struct Dd_windows *tmpwind=dd_sendwind;
	struct Dd_windows *prewind=NULL;
	while (tmpwind){
		if (tmpwind->id==msgid)
		{
			if (prewind)
				prewind->next=tmpwind->next;
			else
				dd_sendwind=tmpwind->next;
			if (tmpwind->data) free(tmpwind->data);
			free(tmpwind);
			break;
		}
		prewind=tmpwind;
		tmpwind=tmpwind->next;
	}
}

/*
 * not used by Now
 */
void add_VTimer(unsigned int id){
	struct Dd_VTimer * tmptimer=(struct Dd_VTimer *)malloc(sizeof(struct Dd_VTimer));
	tmptimer->id=id;
	tmptimer->timer=GetSimulationTime()+0.3;
	tmptimer->next=NULL;
	if (!f_timer) {
		f_timer=tmptimer;
		l_timer=tmptimer;
	}else{
		l_timer->next=tmptimer;
		l_timer=tmptimer;
	}
    	if (!Sender_isTimerSet())
    		Sender_StartTimer(f_timer->timer-GetSimulationTime());
}

/* sender initialization, called once at the very beginning */
void Sender_Init()
{
    dd_sendwind=NULL;
    f_timer= NULL;
    l_timer= NULL;
    send_log=fopen("send_log","w");
    fprintf(stdout, "At %.2fs: sender initializing ...\n", GetSimulationTime());
}

/* sender finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to take this opportunity to release some 
   memory you allocated in Sender_init(). */
void Sender_Final()
{
    if (dd_sendwind) {
    	fprintf(stderr, "opps, sned windows is null\n");
	
	struct Dd_windows * tmpwind=dd_sendwind;
	struct Dd_windows * prewind;
	while (tmpwind){
		prewind = tmpwind;
		tmpwind = tmpwind->next;
		if (prewind->data) free(prewind->data);
		free(prewind);
	}
    }
    fprintf(stdout,"Dd:TotaL messages amount:%d\n",tot_message);
    fprintf(stdout,"Dd:Total packets need send:%d\n",tot_packets);
    fprintf(stdout, "At %.2fs: sender finalizing ...\n", GetSimulationTime());
    fclose(send_log);
}

/* event handler, called when a message is passed from the upper layer at the 
   sender */
void Sender_FromUpperLayer(struct message *msg)
{
    msgtowind(msg);	
	
    /* 1-byte header indicating the size of the payload */
    int header_size = 7;

    /* maximum payload size */
    int maxpayload_size = RDT_PKTSIZE - header_size;

    /* split the message if it is too big */

    /* reuse the same packet data structure */
    packet pkt;

    /* the cursor always points to the first unsent byte in the message */
    int cursor = 0;
    int seq=0;
    while (msg->size-cursor > maxpayload_size) {
	/* fill in the packet */
	pkt.data[0] = maxpayload_size;
	pkt.data[1] = seq++;
	pkt.data[2] = 0;
	pkt.data[3] = (tot_message-1) & 0xff;
	pkt.data[4] = ((tot_message-1)>>8) & 0xff;
	memcpy(pkt.data+header_size, msg->data+cursor, maxpayload_size);

	checksum(&pkt);
	/*check if checksum is added correct! */
	if (!check_ckm(&pkt)) fprintf(stderr,"opps,checksum calc is wrong\n");

	/* send it out through the lower layer */
	Sender_ToLowerLayer(&pkt);

	/* move the cursor */
	cursor += maxpayload_size;

	tot_packets++;
    }

    /* send out the last packet */
    if (msg->size > cursor) {
	/* fill in the packet */
	pkt.data[0] = msg->size-cursor;
	pkt.data[1] = seq++;
	pkt.data[2] = 2;  //label this packet is the final packet in the message
	pkt.data[3] = (tot_message-1) & 0xff;
	pkt.data[4] = ((tot_message-1)>>8) & 0xff;
	memcpy(pkt.data+header_size, msg->data+cursor, pkt.data[0]);

	checksum(&pkt);
	/* send it out through the lower layer */
	Sender_ToLowerLayer(&pkt);

	tot_packets++;
    }
    if (!Sender_isTimerSet())
    	Sender_StartTimer(0.3);
}

/* event handler, called when a packet is passed from the lower layer at the 
   sender */
/* Dd: use to receviced the ack package */
void Sender_FromLowerLayer(struct packet *pkt)
{
	if (!check_ckm(pkt)) return;
	if (pkt->data[2]==1){
		unsigned int msgid=(unsigned char)pkt->data[3]+(((unsigned char)pkt->data[4])<<8);
		fprintf(send_log,"get ack, message id:%d-->%d,%d\n",msgid,pkt->data[3],pkt->data[4]);
		msgoutwind(msgid);
	}
	if (!dd_sendwind)
		Sender_StopTimer();
	else
		Sender_StartTimer(0.3);
}

/* event handler, called when the timer expires */
void Sender_Timeout()
{
#if 0
    if (!f_timer) return;
    unsigned int nowid=l_timer->id;
#endif
    struct Dd_windows * tmpwind=dd_sendwind;
#if 0
    while (tmpwind){
    	if (tmpwind->id==nowid) break;
	tmpwind=tmpwind->next;
    }
    if (!tmpwind) return;
    tmpwind
#endif
#if 1
    int header_size = 7;
    fprintf(send_log,"time out resent: %dmsg\n",tmpwind->id);
    /* maximum payload size */
    int maxpayload_size = RDT_PKTSIZE - header_size;

    /* split the message if it is too big */

    /* reuse the same packet data structure */
    packet pkt;

    /* the cursor always points to the first unsent byte in the message */
    int cursor = 0;
    int seq=0;
    while (tmpwind->size-cursor > maxpayload_size) {
	/* fill in the packet */
	pkt.data[0] = maxpayload_size;
	pkt.data[1] = seq++;
	pkt.data[2] = 0;
	pkt.data[3] = tmpwind->id & 0xff;
	pkt.data[4] = (tmpwind->id>>8) & 0xff;
	memcpy(pkt.data+header_size, tmpwind->data+cursor, maxpayload_size);
	checksum(&pkt);
	/* send it out through the lower layer */
	Sender_ToLowerLayer(&pkt);

	/* move the cursor */
	cursor += maxpayload_size;
    }

    /* send out the last packet */
    if (tmpwind->size > cursor) {
	/* fill in the packet */
	pkt.data[0] = tmpwind->size-cursor;
	pkt.data[1] = seq++;
	pkt.data[2] = 2;  //label this packet is the final packet in the message
	pkt.data[3] = (tmpwind->id) & 0xff;
	pkt.data[4] = (tmpwind->id>>8) & 0xff;
	memcpy(pkt.data+header_size, tmpwind->data+cursor, pkt.data[0]);
	checksum(&pkt);

	/* send it out through the lower layer */
	Sender_ToLowerLayer(&pkt);
    }
 #endif
    if (dd_sendwind)
    	Sender_StartTimer(0.3);
}
