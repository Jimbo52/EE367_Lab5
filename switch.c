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

#define DEBUG TLV

void switchMain(switchState *swistate)
{
	char buffer[1000];
	char word[1000];
	int value;
	char replymsg[1000];
	packetBuffer tmpbuff;

	forwardTable *table;
	table = &(swistate->table);

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

	//tree
	swistate->root = physid;
	swistate->distance = 10000;
	swistate->parent = NULL;
	int i;
	for(i=0;i<NUMSWITCHLINKS;i++)
	{
		swistate->child[i] = 0;
		swistate->datalink[i] = 0;
	}
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

void generateStatePacket(switchState *swistate, int link)
{
	//Type
	unsigned char temp[1];
	temp[0] = 1;
	appendWithSpace(swistate->TLVpacket,temp);

	//Length	
	temp[0] = 4;
	appendWithSpace(swistate->TLVpacket,temp);

	//Root
	char temp2[2];
	memcpy(temp2, (char*)&(swistate->root),2); 	
	appendWithSpace(swistate->TLVpacket,temp);

	//Distance
	int2Ascii(temp,swistate->distance);
	appendWithSpace(swistate->TLVpacket,temp);
	
	//Child
	temp[0] = swistate->child[link];
	appendWithSpace(swistate->TLVpacket,temp);	
	
}

void processStatePacket(switchState *swistate, char packet[], LinkInfo *link, int k)
{
	char fromPacket[1];
	findWord(fromPacket, packet, 1);	

	if(ascii2Int(fromPacket) != 1)
	{
		#if DEBUG == TLV
		printf("Invalid TLV packet.\n");
		#endif

		return;
	}
	
	//find the minimum root and synchronize it
	findWord(fromPacket, packet, 3);
	int temp = ascii2Int(fromPacket);
	if(swistate->root > temp)
	{
		swistate->root = temp;
		//if current state is root, then its distance is 0
		if(swistate->root == swistate->physid)
			swistate->distance = 0;
	}

	//distance update and set parent if necessary
	findWord(fromPacket, packet, 4);
	temp = ascii2Int(fromPacket);
	if(swistate->distance > (temp + 1))
	{
		swistate->distance = temp + 1;
		swistate->parent->linkin[k] = *link;
		swistate->parent->linkout[k] = *link;
		
		//?????????????????????????????????????????????????
	}

	//Identify children
	findWord(fromPacket, packet, 5);
	if(fromPacket == 1)
	
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

void switchTransmitState(switchState *swistate)
{
	int i;
	for(i=0;i<swistate->numlinks;i++)
	{
		generateStatePacket(swistate,i);
		#if DEBUG == TLV
		printf("Sending TLV: %s\n",swistate->TLVpacket);
		#endif

		if(swistate->linkout[i].linkType == UNIPIPE)
		write(swistate->linkout[i].uniPipeInfo.fd[PIPEWRITE],
			  swistate->TLVpacket,strlen(swistate->TLVpacket));
	}		
}

void switchReceiveState(LinkInfo *link, switchState *swistate, int k)
{
	char packet[LOCAL_PACKET_SIZE];
	int n;
	if(link->linkType == UNIPIPE)
	{
		n = read(link->uniPipeInfo.fd[PIPEREAD], packet, LOCAL_PACKET_SIZE);
		packet[n] = '\0';

		#if DEBUG == TLV
		printf("Receving TLV: %s\n",packet);
		#endif
		processStatePacket(swistate,packet,link,k);
	}	
}
