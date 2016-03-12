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
 * |<- 1 byte ->|<- 1 byte ->|<- 1 byte->|<- 2 byte ->|<-  the rest ->|
 * |payload size|   seq no   |  ack      |  mesage no |    payload    |
 *
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rdt_struct.h"
#include "rdt_sender.h"


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

struct Dd_VTImer{
	unsigned id;
	double timer;
	Dd_VTimer *next;
};
struct Dd_VTimer * f_timer;
struct Dd_VTimer * l_timer;
//static 
//char * dd_sendwind;
struct Dd_windows* dd_sendwind;
//static long 
//int dd_windSize;
static int dd_ready;
static unsigned int tot_message=0;
static int tot_packets=0;
/* ADD BY Dd TO add a message into the gloabl window*/
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
void msgoutwind(unsigned int msgid){
	struct Dd_windows *tmpwind=dd_sendwind;
	struct Dd_windows *prewind=NULL;
	if (!dd_sendwind) return;
	while (tmpwind){
		if (tmpwind->id==msgid)
		{
			if (prewind)
				prewind->next=tmpwind->next;
			else
				dd_sendwind=tmpwind->next;
			free(tmpwind->data);
			free(tmpwind);
			break;
		}
		prewind=tmpwind;
		tmpwind=tmpwind->next;
	}
}
void add_VTimer(unsigned id){
	struct Dd_VTimer * tmptimer=(struct Dd_VTimer *)malloc(sizeof(struct Dd_VTimer));
	tmptimer->id=id;
	tmptimer->timer=GetSimulationTime()+0.3;

}

/* sender initialization, called once at the very beginning */
void Sender_Init()
{
    dd_sendwind=NULL;
    dd_ready=0;
    f_timer= NULL
    l_timer= NULL;
    fprintf(stdout, "At %.2fs: sender initializing ...\n", GetSimulationTime());
}

/* sender finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to take this opportunity to release some 
   memory you allocated in Sender_init(). */
void Sender_Final()
{
    //int i=0,j;
    //int size;
    if (dd_sendwind) {
#if 1
	struct Dd_windows * tmpwind=dd_sendwind;
	struct Dd_windows * prewind;
//	int count=0;
	while (tmpwind){
	//	fprintf(stdout,"Log:mesg:%d-->size:%d-->content:\n",tmpwind->size,count++);		
	//	fprintf(stdout,"%s\n",tmpwind->data);
		prewind = tmpwind;
		tmpwind = tmpwind->next;
		free(prewind->data);
		free(prewind);
	}
#endif
    }
    else {
    	fprintf(stderr, "opps, windows is null\n");
    }
    fprintf(stdout,"Dd:TotaL messages amount:%d\n",tot_message);
    fprintf(stdout,"Dd:Total packets need send:%d\n",tot_packets);
    fprintf(stdout, "At %.2fs: sender finalizing ...\n", GetSimulationTime());
}

/* event handler, called when a message is passed from the upper layer at the 
   sender */
void Sender_FromUpperLayer(struct message *msg)
{
  	msgtowind(msg);	
	
#if 1 
    /* 1-byte header indicating the size of the payload */
    int header_size = 5;

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

	/* send it out through the lower layer */
	Sender_ToLowerLayer(&pkt);

	tot_packets++;
    }
#if 1
    if (!Sender_isTimerSet())
    	Sender_StartTimer(0.3);
#endif
#endif
}

/* event handler, called when a packet is passed from the lower layer at the 
   sender */
/* Dd: use to receviced the ack package */
void Sender_FromLowerLayer(struct packet *pkt)
{
	if (pkt->data[2]==1){
		unsigned int msgid=(unsigned char)pkt->data[3]+((unsigned char)pkt->data[4]<<8);
		msgoutwind(msgid);
	}
#if 1
	if (!dd_sendwind)
		Sender_StopTimer();
#endif
}

/* event handler, called when the timer expires */
void Sender_Timeout()
{
#if 1
    int header_size = 5;

    /* maximum payload size */
    int maxpayload_size = RDT_PKTSIZE - header_size;

    /* split the message if it is too big */

    /* reuse the same packet data structure */
    packet pkt;

    /* the cursor always points to the first unsent byte in the message */
    int cursor = 0;
    int seq=0;
    while (dd_sendwind->size-cursor > maxpayload_size) {
	/* fill in the packet */
	pkt.data[0] = maxpayload_size;
	pkt.data[1] = seq++;
	pkt.data[2] = 0;
	pkt.data[3] = dd_sendwind->id &&0xff;
	pkt.data[4] = (dd_sendwind->id>>8) & 0xff;
	memcpy(pkt.data+header_size, dd_sendwind->data+cursor, maxpayload_size);

	/* send it out through the lower layer */
	Sender_ToLowerLayer(&pkt);

	/* move the cursor */
	cursor += maxpayload_size;
    }

    /* send out the last packet */
    if (dd_sendwind->size > cursor) {
	/* fill in the packet */
	pkt.data[0] = dd_sendwind->size-cursor;
	pkt.data[1] = seq++;
	pkt.data[2] = 2;  //label this packet is the final packet in the message
	pkt.data[3] = (dd_sendwind->id) & 0xff;
	pkt.data[4] = (dd_sendwind->id>>8) & 0xff;

	memcpy(pkt.data+header_size, dd_sendwind->data+cursor, pkt.data[0]);

	/* send it out through the lower layer */
	Sender_ToLowerLayer(&pkt);
    }
 #endif
 #if 0
    static int cnt=0;
    if (dd_sendwind){
	    Sender_StartTimer(0.3);
	    fprintf(stderr,"sendwind not empty,set timer auto:%d\n",cnt++);
     }
#endif
}
