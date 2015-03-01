#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "utilities.h"
#include "main.h"
#include "link.h"
#include "man.h"
#include "switch.h"

#define PIPEREAD 0
#define PIPEWRITE 1

#define MAXPACKETS 100
#define MAXBUFFER 100
#define TENMILLISEC 10000

#define NONEXIST -1
#define QFULL 98
#define EMPTY_ADDR 0xffff

void switchInitState(switchState *swistate, int physid);
void switchInitRcvPacketBuff(packetBuffer *packetbuff);
void switchInitSendPacketBuff(packetBuffer *packetbuff);

/* not sure if we need these two functions*/
void switchUploadPacket(switchState * swistate, char fname[], char replymsg[]);
void switchDownloadPacket(switchState * swistate, char fname[], char replymsg[]);

/* transmit from switch to des. if linkID is in table, send via linkID
 * otherwise, send to all host. Have to fix to not send to the source host, 
 * maybe add a int variable in the function input to track which input link is.
 * don't know exactly how.
 */
void switchTransmitPacket(switchState * swistate, int linkID)
{
	int i;
	if(linkID != NONEXIST) {// entry existed in forward table, need to find correct outlink 
		for (i = 0; k < swistate->numlinks; i++) { //search for correct link
			if(swistate->linkout[i].linkID == linkID){
				linkSend(&swistate->linkout[i], &(swistate->rcvPacketBuff));
				break;
			}
		}
	}
	else {		//can't find linkID in forward table 
		for(i = 0; i < swistate->numlinks; i++){	//send to all host
			linkSend(&(swistate->linkout[i]), &(swistate->rcvPacketBuff));
		}
	}
}

void switchMain(switchState *swistate)
{
	char buffer[1000];
	char word[1000];
	int value;
	char replymsg[1000];
	packetBuffer tmpbuff;
	forwardingTable table;
	packetqueue *packetq;
	packetBuffer transingpacket;
	int i,j;	//index i for each host <-> switch, j for each entry in forwardtable
	int linkID;

//init packet queue
	packetq.head = 0;
	packetq.tail = -1;

	while(1) {
		for(i = 0; i <swistate->numlinks; i++) {
			linkReceive(&(sstate->linkin[i]), &tmpbuff);
			if(tmpbuff.valid == 1 && tmpbuff.new == 1){
				for(j = 0; j < table.count; j++){
					table.linkID[j] = swistate->linkout[i].linkID;
					break;
				}
				if(j == table.count) {
					table.valid[j] = 1;
					table.destinationAddr[j] = tmpbuff.srcaddr;
					table.linkID[j] = swistate->linkout[i].linkID;
					table.count ++;
				}
				AppendQ(packetq, tmpbuff, i);
			}
		}
		if((packetq->tail + 1) != packetq->head){ //serve a pcket from head of Q
			transingpacket = ServeQ(packetq);
		}
		else {			//wait and check again
			usleep(TENMILLISEC);
		}
		if(transingpakcet.valid == 1 && transingpacket.new == 1){
			swistate->packetrcvPackBuff = transingpacket;
			swistate->rcvPacketBuff.new = 1;
			swistate->rcvPacketBuff.valid = 1;

			for(j = 0; j< table.count; j++) {
				if(table.destinationAddr[j] == transingpacket.dstaddr){
					linkID = table.linkID[j];
					break;	//fill the table with new entry
				}
			}
			if(j == table.count)
				linkID = NONEXIST;	//entry not found, means new link
			switchTransmitPacket(swistate, buffer, linkID);	//send to linkID
			swistate->rcvPacketBuff.new = 0;	// indicate packet sent, no new 
		}
		usleep(TENMILLISEC);
	}
}

void switchInit(switchState *swistate)
{
	switchInitState(swistate, physid);
	switchInitRcvPacketBuff(&(swistate->rcvPacketBuff));
	switchInitSendPacketBuff(&(swistate->rcvPacketBuff));
	rcvflag = 0;
	numlinks = 0;
}

void switchInitState(switchState *swistate, int physid)
{
	swistate->physid = physid;
	swistate->rcvPacketBuff.valid = 0;
	swistate->rcvPacketBuff.new = 0;
}

void switchInitRcvPacketBuff(packetBuffer * packetbuff)
{
	packetbuff->valid = 0;
	packetbuff->new = 0;
}

void switchInitSendPacketBuff(packetBuffer *packetbuff)
{
	packetbuff->valid = 0;
	packetbff->new = 0;
}

void AppendQ(packetqueue *q, packetBuffer newpacket)
{
	if (q->head == QFULL){
		printf("packet queue is full.\n");
		return;
	}
	else {
		q->tail = (a->tail+1) % MAXPACKETS;

		q->packet[q->tail].srcaddr = newpacket.srcaddr;
		q->packet[q->tail].dstaddr = newpacket.dstaddr;
		q->packet[q->tail].length = newpacket.length;
		strcpy(q->packet[q->tail].payload, newpacket.payload);
		q->packet[q->tail].valid = q->packet[q->head].valid;
		q->packet[q->tail].new = q->pqcket[q->head].new;
	}
}

packetBuffer ServeQ(packetqueue *q)
{
	packetBuffer packet;
	if(q->tail + 1 == q->head){
		packet.valid = 0; //to return a invalid packet indicating error
	}
	else {
		packet.srcaddr = q->packet[q->head].srcaddr;
		packet.dstaddr = q->packet[q->head].dstaddr;
		packet.length = q->packet[q->head].length;
		strcpy(packet.payload, q->packet[q->head].payload);
		packet.valid = q->packet[q->head].valid;
		packet.new = q->pqcket[q->head].new;

		q->head = (q->head +1) % MAXPACKETS;
	}
	return packet;
}
