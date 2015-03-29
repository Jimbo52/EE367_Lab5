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

#define DEBUG 0

#define PIPEREAD 0
#define PIPEWRITE 1

#define MAXBUFFER 100
#define TENMILLISEC 10000

#define NONEXIST -1
#define EMPTY_ADDR 0xffff

void switchInitState(switchState *swistate, int physid);
void switchInitRcvPacketBuff(packetBuffer *packetbuff);
void switchInitSendPacketBuff(packetBuffer *packetbuff);

/* not sure if we need these two functions*/
void switchUploadPacket(switchState * swistate, char fname[], char replymsg[]);
void switchDownloadPacket(switchState * swistate, char fname[], char replymsg[]);

void switchTransmitPacket(switchState * swistate, int linkID);

/* transmit from switch to des. if linkID is in table, send via linkID
 * otherwise, send to all host. Have to fix to not send to the source host, 
 * maybe add a int variable in the function input to track which input link is.
 * don't know exactly how.
 */
void switchTransmitPacket(switchState * swistate, int linkID)
{
#if DEBUG == 1
	printf("switchTransmitPacket\n");
#endif
	int i;
	if(linkID != NONEXIST) {// entry existed in forward table, need to find correct outlink 
		/*
		for (i = 0; i < swistate->numlinks; i++) { //search for correct link
			if(swistate->linkout[i].linkID == linkID){
				linkSend(&swistate->linkout[i], &(swistate->rcvPacketBuff));
				break;
			}
		}
		*/
		linkSend(&swistate->linkout[linkID], &(swistate->rcvPacketBuff));
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
	forwardTable table;
	Packet *transingpacket;
	int i,j;	//index i for each host <----> switch, j for each entry in forwardtable
	int linkID;

/*create and init a packet queue*/
	packetqueue *packetq = init();

#if DEBUG == 1
	printf("switchMain\n");
#endif

	while(1) {
#if DEBUG == 1
#endif
		for(i = 0; i <swistate->numlinks; i++) {
			linkReceive(&(swistate->linkin[i]), &tmpbuff);
			if(tmpbuff.valid == 1 && tmpbuff.new == 1){	//a new packet comming
#if DEBUG == 1
	printf("new packet comming\n");
#endif
				for(j = 0; j < table.count; j++){
					if(table.destinationAddr[j] == tmpbuff.srcaddr){
						//update table linkID
						table.linkID[j] = swistate->linkout[i].linkID;
						break;
					}
				}
				if(j == table.count) { //update new entry to table
					table.valid[j] = 1;
					table.destinationAddr[j] = tmpbuff.srcaddr;
					table.linkID[j] = i;
					table.count ++;
				    printf("\n\n\nvalid\t\tDestination Network Address\t\tOutgoing link #\n");
				    for(i = 0; i < table.count; i++)
				    {
					printf("%d\t\t%d\t\t\t\t\t%d\n",table.valid[i],table.destinationAddr[i], table.linkID[i]);
				    }
				    printf("\n\n");
				}
				AppendQ(packetq, tmpbuff);
				#if DEBUG == 1
				printf("appending\n");
				#endif
			}
		}


		if(packetq->head != NULL){ //serve a pcket from head of Q
			transingpacket = ServeQ(packetq);
			#if DEBUG == 1
			printf("serving\n");
			#endif
		}
		else {			// empty Q, wait and check again
			usleep(TENMILLISEC);
			continue;
		}
		if(transingpacket->packet.valid == 1 && transingpacket->packet.new == 1){
			swistate->rcvPacketBuff= transingpacket->packet;
			swistate->rcvPacketBuff.new = 1;
			swistate->rcvPacketBuff.valid = 1;

			for(j = 0; j< table.count; j++) {
				if(table.destinationAddr[j] == transingpacket->packet.dstaddr){
					linkID = table.linkID[j];
					break;	// take the linkID in table to transmit
				}
			}
			if(j == table.count)
				linkID = NONEXIST;	//entry not found, means new link
			switchTransmitPacket(swistate, linkID);	//send to linkID
			#if DEBUG == 1
			printf("transmited\n");
			#endif
			swistate->rcvPacketBuff.new = 0;	// indicate packet sent, no new 
			#if DEBUG == 1
			printf("resetbuff new = 0\n");
			#endif
		}
		#if DEBUG == 1
		printf("sleep\n");
		#endif
		usleep(TENMILLISEC);
	}
}

void switchInit(switchState *swistate, int physid)
{
	switchInitState(swistate, physid);
	switchInitRcvPacketBuff(&(swistate->rcvPacketBuff));
	switchInitSendPacketBuff(&(swistate->rcvPacketBuff));
}

void switchInitState(switchState *swistate, int physid)
{
	swistate->physid = physid;
	swistate->rcvPacketBuff.valid = 0;
	swistate->rcvPacketBuff.new = 0;
	swistate->numlinks = 0;
}

void switchInitRcvPacketBuff(packetBuffer * packetbuff)
{
	packetbuff->valid = 0;
	packetbuff->new = 0;
}

void switchInitSendPacketBuff(packetBuffer *packetbuff)
{
	packetbuff->valid = 0;
	packetbuff->new = 0;
}

void AppendQ(packetqueue *q, packetBuffer newpacket)
{
	//I tried to not use malloc dynamic memory allocate, it didnt work...dont know Y.
	Packet * new = (Packet*)malloc(sizeof(Packet));

	new->packet = newpacket;
	new->next = NULL;
	if (q->head == NULL){
		q->head = q->tail = new;
	}
	else {
		q->tail->next = new;
		q->tail = new;
	}
}

Packet* ServeQ(packetqueue *q)
{
	Packet *head;
	head = q->head;
	if(q->head == q->tail){ //empty
		q->head = NULL;
	}
	else {
		q->head = q->head->next;
	}
	return head;
}

packetqueue* init(){

	//I tried to not use malloc dynamic memory allocate, it didnt work...dont know Y.
	packetqueue* pq = (packetqueue*)malloc(sizeof(packetqueue));;
	pq->head = NULL;
	pq->tail = NULL;
	return pq;
}
