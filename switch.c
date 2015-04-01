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


void switchMain(switchState *swistate)
{
	char buffer[1000];
	char word[1000];
	int value;
	char replymsg[1000];
	packetBuffer tmpbuff;

	forwardTable *table;
	table = &(swistate->table);

	Packet *transingpacket;
	int i,j;	//index i for each host <----> switch, j for each entry in forwardtable

#if DEBUG == 1
	printf("switchMain\n");
#endif

	while(1) {
		for(i = 0; i <swistate->numlinks; i++) {
			linkReceive(&(swistate->linkin[i]), &tmpbuff);
			if(tmpbuff.valid == 1 && tmpbuff.new == 1){	//a new packet comming
#if DEBUG == 1
	printf("new packet comming\n");
#endif
				for(j = 0; j < table->count; j++){
					if(table->destinationAddr[j] == tmpbuff.srcaddr){
						//update table linkID
						table->linkID[j] = swistate->linkout[i].linkID;
						break;
					}
				}
				if(j == table->count) { //update new entry to table
					table->valid[j] = 1;
					table->destinationAddr[j] = tmpbuff.srcaddr;
					table->linkID[j] = i;
					table->count ++;
					displayForwardTable(swistate);
				}
				AppendQ(swistate, tmpbuff);
				#if DEBUG == 1
				printf("appending\n");
				#endif
			}
		}


		if(swistate->packetQueue.head != NULL){ //serve a pcket from head of Q
			switchTransmitPacket(swistate);	
			#if DEBUG == 1
			printf("transmited\n");
			#endif
		}
		else {			// empty Q, wait and check again
			usleep(TENMILLISEC);
			continue;
		}
		#if DEBUG == 1
		printf("sleep\n");
		#endif
		usleep(TENMILLISEC);
	}
}

void switchInit(switchState *swistate, int physid)
{
	swistate->physid = physid;
	swistate->numlinks = 0;
	
	//forwarding table
	swistate->table.count = 0;

	//queue
	swistate->packetQueue.head = NULL;
	swistate->packetQueue.tail = NULL;
}


void AppendQ(switchState *swistate, packetBuffer newpacket)
{
	Packet * new = malloc(sizeof(Packet));
	new->packet = newpacket;
	new->next = NULL;

	packetqueue *q;
	q = &(swistate->packetQueue);

	if (q->head == NULL){
		q->head = q->tail = new;
	}
	else {
		q->tail->next = new;
		q->tail = new;
	}
}

Packet* ServeQ(switchState *swistate)
{
	packetqueue *q;
	q = &(swistate->packetQueue);
	Packet *head;
	head = q->head;

	q->head = q->head->next;
	if(q->head == NULL)
		q->tail = NULL;
	return head;
}

void displayForwardTable(switchState *swistate)
{
	int i;
	forwardTable *it;
	it = &(swistate->table);
	printf("\n\n\nvalid\t\tDestination Network Address\t\tOutgoing link #\n");
	for(i = 0; i < it->count; i++)
	{
	printf("%d\t\t%d\t\t\t\t\t%d\n",it->valid[i],it->destinationAddr[i],it->linkID[i]);
	}
	printf("\n\n");
}


/* transmit from switch to des. if linkID is in table, send via linkID
 * otherwise, send to all host. Have to fix to not send to the source host, 
 * maybe add a int variable in the function input to track which input link is.
 * don't know exactly how.
 */
void switchTransmitPacket(switchState *swistate)
{
#if DEBUG == 1
	printf("switchTransmitPacket\n");
#endif
	int i,linkID;
	Packet *transingpacket;
	transingpacket = ServeQ(swistate);
	forwardTable *table;
	table = &(swistate->table);
	
	if(transingpacket->packet.valid == 1 && transingpacket->packet.new == 1){
		for(i = 0; i< table->count; i++) {
			if(table->destinationAddr[i] == transingpacket->packet.dstaddr){
				linkID = table->linkID[i];
				break;	// take the linkID in table to transmit
			}
		}
		if(i == table->count)
			linkID = NONEXIST;	//entry not found, means new link
	}

	if(linkID != NONEXIST) {// entry existed in forward table, need to find correct outlink 
		linkSend(&swistate->linkout[linkID], &(transingpacket->packet));
	}
	else {		//can't find linkID in forward table 
		for(i = 0; i < swistate->numlinks; i++){	//send to all host
			linkSend(&(swistate->linkout[i]), &(transingpacket->packet));
		}
	}
}
