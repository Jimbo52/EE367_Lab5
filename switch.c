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
#define MAXBUFFER 100
#define TENMILLISEC 10000
#define EMPTY_ADDR 0xffff

#define DEBUG TABLE

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
	int k,j;	//index i for each host <----> switch, j for each entry in forwardtable

	while(1) {
		for(k = 0; k <swistate->numlinks; k++) {
			linkReceive(&(swistate->linkin[k]), &tmpbuff);
			if(tmpbuff.valid == 1 && tmpbuff.new == 1){	//a new packet comming
				for(j = 0; j < table->numentries; j++){
					if(table->Entry[j].destNetworkAddress == tmpbuff.srcaddr){
						//update table linkID
						table->Entry[j].outlink = swistate->linkout[k];
						break;
					}
				}
				if(j == table->numentries) { //update new entry to table
					table->Entry[j].valid = 1;
					table->Entry[j+1].valid = 0;
					table->Entry[j].destNetworkAddress = tmpbuff.srcaddr;
					table->Entry[j].outlink = swistate->linkout[k];
					table->numentries++;
					#if DEBUG == TABLE
					displayForwardTable(swistate);
					#endif
				}
				AppendQ(swistate, tmpbuff, k);
			}
		}


		if(swistate->packetQueue.head != NULL){ //serve a pcket from head of Q
			switchTransmitPacket(swistate);	
		}
		else {			// empty Q, wait and check again
			usleep(TENMILLISEC);
			continue;
		}
		usleep(TENMILLISEC);
	}
}

void switchInit(switchState *swistate, int physid)
{
	swistate->physid = physid;
	swistate->numlinks = 0;
	
	//forwarding table
	swistate->table.numentries = 0;

	//queue
	swistate->packetQueue.head = NULL;
	swistate->packetQueue.tail = NULL;
}


void AppendQ(switchState *swistate, packetBuffer newpacket, int k)
{
	Packet * new = malloc(sizeof(Packet));
	new->k = k;
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
	for(i = 0; i < it->numentries; i++)
	{
	printf("%d\t\t%d\t\t\t\t\t%d\n",it->Entry[i].valid,it->Entry[i].destNetworkAddress,it->Entry[i].outlink.linkID);
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
	int i,transmitlink;
	Packet *transingpacket;
	transingpacket = ServeQ(swistate);
	forwardTable *table;
	table = &(swistate->table);
	
	if(transingpacket->packet.valid == 1 && transingpacket->packet.new == 1){
		for(i = 0; i< table->numentries; i++) {
			if(table->Entry[i].destNetworkAddress == transingpacket->packet.dstaddr){
				transmitlink = table->Entry[i].outlink.linkID;
				break;	
			}
		}
		if(i == table->numentries)
			transmitlink = -1;	
	}

	if(transmitlink > -1) {
		for(i = 0; i < swistate->numlinks; i++) 
		{
			if(swistate->linkout[i].linkID == transmitlink)
			{
				linkSend(&swistate->linkout[i], &(transingpacket->packet));
				break;
			}
		}
	}
	else {		
		for(i = 0; i < swistate->numlinks; i++){
			if(i != transingpacket->k)
			linkSend(&(swistate->linkout[i]), &(transingpacket->packet));
		}
	}
}
