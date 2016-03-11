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
 * |<- 1 byte ->|<- 1 byte ->|<- 1 byte ->|<-  the rest ->|
 * |payload size|   seq no   |  mesage no |    payload    |
 *
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rdt_struct.h"
#include "rdt_sender.h"

static char * dd_windows;
static long int dd_windSize;
static int dd_ready;
/* sender initialization, called once at the very beginning */
void Sender_Init()
{
    dd_windows=NULL;
    dd_windSize=0;
    dd_ready=0;
    fprintf(stdout, "At %.2fs: sender initializing ...\n", GetSimulationTime());
}

/* sender finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to take this opportunity to release some 
   memory you allocated in Sender_init(). */
void Sender_Final()
{
    int i=0,j;
    int size;
    if (dd_windows) {
    	while(i<dd_windSize){
		fprintf(stdout,"Log:mesg:%d-->content:\n",dd_windows[i++]);
		size=*((int *)(dd_windows+i));
		for (j=0;j<size;j++){
			fprintf(stdout,"%c",dd_windows[i++]);
		}
	}

    	free(dd_windows);
    }
    else {
    	fprintf(stderr, "opps, windows is null\n");
    }
    fprintf(stdout, "At %.2fs: sender finalizing ...\n", GetSimulationTime());
}

/* event handler, called when a message is passed from the upper layer at the 
   sender */
void Sender_FromUpperLayer(struct message *msg)
{
    static char messno=0;
    char * tmpch;
    if (!dd_ready){
    	dd_windSize+=msg->size+4+1;
	if (dd_windows){
		tmpch=(char *)realloc(dd_windows,dd_windSize);
		dd_windows=tmpch;
	}
	else
		dd_windows=(char *)malloc(dd_windSize);
	memcpy(dd_windows+dd_windSize-msg->size+4,msg->data,msg->size);
	*((int*)(dd_windows+dd_windSize-msg->size+1))=msg->size;
	messno=(messno+1) & 0xff;
	*(dd_windows+dd_windSize-msg->size) = messno;
//	if (messno==0) 
	dd_ready=1;
	return;
    }

#if 0
    /* 1-byte header indicating the size of the payload */
    int header_size = 1;

    /* maximum payload size */
    int maxpayload_size = RDT_PKTSIZE - header_size;

    /* split the message if it is too big */

    /* reuse the same packet data structure */
    packet pkt;

    /* the cursor always points to the first unsent byte in the message */
    int cursor = 0;

    while (msg->size-cursor > maxpayload_size) {
	/* fill in the packet */
	pkt.data[0] = maxpayload_size;
	memcpy(pkt.data+header_size, msg->data+cursor, maxpayload_size);

	/* send it out through the lower layer */
	Sender_ToLowerLayer(&pkt);

	/* move the cursor */
	cursor += maxpayload_size;
    }

    /* send out the last packet */
    if (msg->size > cursor) {
	/* fill in the packet */
	pkt.data[0] = msg->size-cursor;
	memcpy(pkt.data+header_size, msg->data+cursor, pkt.data[0]);

	/* send it out through the lower layer */
	Sender_ToLowerLayer(&pkt);
    }
#endif
}

/* event handler, called when a packet is passed from the lower layer at the 
   sender */
/* Dd: use to receviced the ack package */
void Sender_FromLowerLayer(struct packet *pkt)
{
}

/* event handler, called when the timer expires */
void Sender_Timeout()
{
	
}
