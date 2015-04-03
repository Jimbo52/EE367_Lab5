/* 
 * This is the source code for the SWITCH.
 * hostMain is the main function for the host.  It is an infinite
 * loop that repeatedy polls the connection from the manager and
 * its input link.  
 *
 * If there is command message from the manager,
 * it parses the message and executes the command.  This will
 * result in it sending a reply back to the manager.  
 *
 * If there is a packet on its incoming lik, it checks if
 * the packet is destined for it.  Then it stores the packet
 * in its receive packet buffer.
 *
 * There is also a 10 millisecond delay in the loop caused by
 * the system call "usleep".  This puts the host to sleep.  This
 * should reduce wasted CPU cycles.  It should also help keep
 * all nodes in the network to be working at the same rate, which
 * helps ensure no node gets too much work to do compared to others.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <unistd.h>
#include <fcntl.h>

#include "main.h"
#include "utilities.h"
#include "link.h"
#include "man.h"
#include "switch.h"

#define EMPTY_ADDR  0xffff  /* Indicates that the empty address */
                             /* It also indicates that the broadcast address */
#define MAXBUFFER 1000
#define PIPEWRITE 1 
#define PIPEREAD  0
#define TENMILLISEC 10000   /* 10 millisecond sleep */

/* 
 * hostInit initializes the host.  It calls
 * - hostInitState which initializes the host's state.
 * - hostInitRcvPacketBuff, which initializes the receive packet buffer
 * - hostInitSendPacketBuff, which initializes the send packet buffer
 */
void switchInitState(switchState * sstate, int physid); 
void switchInitRcvPacketBuff(packetBuffer * packetbuff);
void switchInitSendPacketBuff(packetBuffer * packetbuff);

/*
 * hostMain is the main loop for the host. It has an infinite loop.
 * In the loop it first calls
 * hostCommandReceive to check if a command message came from the
 * manager.
 * If a command arrived, then it checks the first word of the
 * message to determine the type of command.  Depending on the
 * command it will call
 * - hostSetNetAddr to set the host's network address
 *      The command message should be "SetNetAddr <network address>"
 * - hostSetMainDir to set the host's main directory
 *      The command message should be "SetMainDir <directory name>"
 * - hostClearRcvFlg to clear the host's receive flag
 * - hostUploadPacket to upload a file to the host's send packet
 *      buffer. The command message should be "UploadPacket <file name>"
 * - hostDownloadPacket to download the payload of the host's
 *      receive packet buffer to a file.  The command message
 *      should be "DownloadPacket <file name>"
 * - hostTransmitPacket to transmit the packet in the send packet buffer.
 *      The command message should be "TransmitPacket <destination address>"
 * - hostGetHostState to get the host's state.  The command message
 *      should be "GetHostState".  
 */

void switchUploadPacket(switchState * sstate, char fname[], char replymsg[]); 
void switchDownloadPacket(switchState * sstate, char fname[], char replymsg[]); 
void switchTransmitPacket(switchState * sstate, char word[],int linkin,int transmitlink);

/*
 * Functions
 */

/*
 * switchTransmitPacket will transmit a packet in the send packet buffer
 */
void switchTransmitPacket(switchState * sstate, char word[],int linkin,int transmitlink)
{
char dest[1000];
int  dstaddr;
int k;

//printf("Transmitting Packet on Switch\n");

/* Get the destination address from the manager's command message (word[]) */ 
//findWord(dest, word, 2);
//dstaddr = ascii2Int(dest);

/* 
 * Set up the send packet buffer's source and destination addresses
 */
//sstate->sendPacketBuff.dstaddr = dstaddr;
//sstate->sendPacketBuff.srcaddr = sstate->netaddr;

if(transmitlink > -1) {
	for(k = 0; k < sstate->numlinks; k++) {
	    if(sstate->linkout[k].linkID == transmitlink) {
//		printf("Sending on link %d\n",transmitlink);
		linkSend(&sstate->linkout[k],&(sstate->rcvPacketBuff));
		break;
	    }
}
}
else {
/* Transmit the packet on the link */
for(k = 0; k < sstate->numlinks; k++)  {
	if(k != linkin) {
		linkSend(&(sstate->linkout[k]), &(sstate->rcvPacketBuff));
//		printf("Transmitted on link %d\n",sstate->linkout[k].linkID);
	}
	}
}

}


/* 
 * Main loop of the host node
 *
 * It polls the manager connection for any requests from
 * the manager, and repliies
 *
 * Then it polls any incoming links and downloads any
 * incoming packets to its receive packet buffer
 *
 * Then it sleeps for 10 milliseconds
 *
 * Then back to the top of the loop
 *
 */
void switchMain(switchState * sstate)
{
char buffer[1000]; /* The message from the manager */
char word[1000];
int  value;
int k,j,transmitlink;
char replymsg[1000];   /* Reply message to be displayed at the manager */
packetBuffer tmpbuff;
forwardingTable ftable;
pqueue * pq = init();
pqnode * pqptr;


while(1) {

   /* Check if there is an incoming packet */
   for(k = 0;k < sstate->numlinks; k++)  {
        linkReceive(&(sstate->linkin[k]), &tmpbuff);	
        if(tmpbuff.valid == 1 && tmpbuff.new == 1) {
	     for(j = 0; j < ftable.numentries; j++) {
//		   printf("\nIn table\n");
//		   printf("%d\t%d\t%d\n",ftable.Entry[j].valid,ftable.Entry[j].destNetworkAddress,ftable.Entry[j].outlink.linkID);
                   if(ftable.Entry[j].destNetworkAddress == tmpbuff.srcaddr) {
//			printf("Changing link number\n");
			ftable.Entry[j].outlink = sstate->linkout[k];
			break;
		   }
             }
	     if(j == ftable.numentries) {
                   ftable.Entry[j].valid = 1;
		   ftable.Entry[j+1].valid = 0;
		   ftable.Entry[j].destNetworkAddress = tmpbuff.srcaddr;
		   ftable.Entry[j].outlink = sstate->linkout[k]; 
		   ftable.numentries++;
//		   printf("Adding to table\n");
//		   printf("%d\t%d\t%d\n",ftable.Entry[j].valid,ftable.Entry[j].destNetworkAddress,ftable.Entry[j].outlink.linkID);
             }
             
         //    printf("We're done\n");
   //          printf("Temp buff: %s\n",tmpbuff.payload);
	     push(pq,tmpbuff,k);            	
       //      break;
        } 

//   if(strlen(tmpbuff.payload)) 
 //      printf("tmpbuff.valid == %d, tmpbuff.new == %d\n",tmpbuff.valid,tmpbuff.new);
   }
   
   /* 
    * If there is a packet and if the packet's destination address 
    * is the host's network address then store the packet in the
    * receive packet buffer
    */
   if(pq->head != NULL) {
      pqptr = pop(pq);
   }
   else {
      usleep(TENMILLISEC);
      continue;
   }
   if (pqptr->packet.valid == 1 && pqptr->packet.new == 1) {
      sstate->rcvPacketBuff = pqptr->packet;
      sstate->rcvPacketBuff.new = 1;
      sstate->rcvPacketBuff.valid = 1;

      for(j = 0; j < ftable.numentries; j++) {
	  if(ftable.Entry[j].destNetworkAddress == pqptr->packet.dstaddr) {
	      transmitlink = ftable.Entry[j].outlink.linkID;
	      break;
	  }
      }
      if(j == ftable.numentries)
      	  transmitlink = -1;
    //  printf("Sending: %s\n",sstate->rcvPacketBuff.payload); 
      switchTransmitPacket(sstate,buffer,pqptr->k,transmitlink);
      sstate->rcvPacketBuff.new = 0;
    }

   /* The host goes to sleep for 10 ms */
   usleep(TENMILLISEC);

} /* End of while loop */

}

/* 
 * Initializes the switch.   
 */
void switchInit(switchState * sstate, int physid)
{

switchInitState(sstate, physid);     /* Initialize switch's state */

sstate->numlinks = 0;

/* Initialize the receive and send packet buffers */
switchInitRcvPacketBuff(&(sstate->rcvPacketBuff));  
switchInitSendPacketBuff(&(sstate->rcvPacketBuff)); 
}

/* 
 * Initialize send packet buffer 
 */
void switchInitSendPacketBuff(packetBuffer * packetbuff)
{
packetbuff->valid = 0;
packetbuff->new = 0;
}


/* 
 * Initialize receive packet buffer 
 */ 

void switchInitRcvPacketBuff(packetBuffer * packetbuff)
{
packetbuff->valid = 0;
packetbuff->new = 0;
}

/* 
 * Initialize the state of the switch 
 */
void switchInitState(switchState * sstate, int physid)
{
sstate->physid = physid;
sstate->rcvPacketBuff.valid = 0;
sstate->rcvPacketBuff.new = 0;
}

pqueue * init(void)
{
   pqueue * q;
   q = (pqueue *)malloc(sizeof(pqueue));

   if(q == NULL)
   {
    fprintf(stderr,"Insufficient mem for new queue.\n");
    exit(1);
   }

   q->head = q->tail = NULL;
   return q;
}

pqnode * pop(pqueue * q)
{

   pqnode * head;
   head = q->head;
   if(q->head != q->tail)
     q->head = q->head->next;
   else q->head = q->tail = NULL;
   return head;
}

void push(pqueue * q,packetBuffer p,int k)
{
   pqnode * newnode = (pqnode *)malloc(sizeof(pqnode));

   newnode->k = k;
   newnode->packet = p;
   newnode->next = NULL;
   if(q->head == NULL) q->head = q->tail = newnode;
   else
   {
      q->tail->next = newnode;
      q->tail = newnode;
   }

}
