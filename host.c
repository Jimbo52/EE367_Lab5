/*  620 670
 * hostMain is the main function for the host.  It is an infinite
 * loop that repeatedy polls the connection from the manager and
 * its input link.  
 *
 * If there is command message from the manager,
 * it parses the message and executes the command.  Then it
 * will send a reply back to the manager.  
 *
 * If there is a packet on its incoming link, it checks if
 * the packet is destined for it.  Then it stores the packet
 * in its receive packet buffer.
 *
 * There is also a 10 millisecond delay in the loop caused by
 * the system call "usleep".  This puts the host to sleep for a
 * fixed amount of time.  This should keep the all the nodes
 * in the network working at the same rate; all nodes will do
 * some tasks then sleep for a fixed amount of time, then do
 * additional tasks then sleep for a fixed amount of time.  
 * This help ensure that no node does too much work compared
 * to the others.  Thus, the nodes all work at the same rate.
 * This prevents some nodes working too fast for the other
 * nodes to keep up.
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
#include "host.h"

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
void hostInitState(hostState * hstate, int physid); 
void hostInitRcvPacketBuff(packetBuffer * packetbuff);
void hostInitSendPacketBuff(packetBuffer * packetbuff);

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
int  hostCommandReceive(managerLink * manLink, char command[]);

void hostSetNetAddr(hostState * hstate, int netaddr, char replymsg[]);
void hostSetMainDir(hostState * hstate, char filename[], char replymsg[]);
void hostClearRcvFlg(hostState * hstate, char replymsg[]);
void hostUploadPacket(hostState * hstate, char fname[], char replymsg[]); 
void hostDownloadPacket(hostState * hstate, char fname[], char replymsg[]); 
void hostTransmitPacket(hostState * hstate, char word[], char replymsg[]);
void hostGetHostState(hostState * hstate, managerLink * manLink, char replymsg[]);

/* 
 * After host executes a command from the manager, it sends a reply
 * message to the manager in the format " Replytype <reply[]>".
 * - Replytype can be
 *    o "DISPLAY" which means the rest of the message should be 
 *      displayed on the user's console
 *    o "GetHostStateACK" which means the rest of the message is the
 *      host's state
 */ 
void hostReplySend(managerLink * manLink, char replytype[], char reply[]);

/* This is used to send a message to the manager */
void hostToManSend(managerLink * manLink, char reply[]);

/*
 * Functions
 */

/*
 * hostTransmitPacket will transmit a packet in the send packet buffer
 */
void hostTransmitPacket(hostState * hstate, char word[], char replymsg[])
{
char dest[1000];
int  dstaddr;
    unsigned int i,j;
    char tempbuff[PAYLOAD_LENGTH + 1];
    Queue *newq;
    Queue *old;
    Queue *tail;
    Queue *head;
    /* Flag, true if last item in queue */
    int last = 0;
    /* Flag, true is only one container sent */
    int only_one = 1;
    /* Variable to hold size on container */
    unsigned int cont_size;


/* Get the destination address from the manager's command message (word[]) */ 
findWord(dest, word, 2);
dstaddr = ascii2Int(dest);

/* 
 * Set up the send packet buffer's source and destination addresses
 */
hstate->sendPacketBuff.dstaddr = dstaddr;
hstate->sendPacketBuff.srcaddr = hstate->netaddr;

/* Transmit the packet on the link */
//linkSend(&(hstate->linkout), &(hstate->sendPacketBuff));
    
    
    /* If the data to be sent is less then 200 bytes */
    if(hstate->sendPacketBuff.length <= 200)
    {
        /* Then just send it */
        linkSend(&(hstate->linkout), &(hstate->sendPacketBuff));
    }
    
    else
    {
        
        /* Create queue to send */
        
        /* Store payload to temp buffer */
        for(i = 0; i < hstate->sendPacketBuff.length; i += 1)
        {
            /* Store ith element of payload to temp buff */
            tempbuff[i] = hstate->sendPacketBuff.payload[i];
            /* Clear out element from source payload */
            hstate->sendPacketBuff.payload[i] = '\0';
        }
        /* mark the end of the payload */
        tempbuff[i] = '\0';
        
        /* Create Queue */
        head = tail = NULL;
        /* While there are bytes to send */
        for(i = 0; i < hstate->sendPacketBuff.length;)
        {
            /* Create a new element in the queue */
            newq = (Queue*) malloc(sizeof(Queue));
            /* Next item in queue does not exist */
            newq->next = NULL;
            /* While there is room in the container */
            for(j = 0; j < 198; j += 1)
            {
                /* if there are still bytes to store */
                if(tempbuff[i+j] != '\0')
                {
                    /* Store next byte into container */
                    newq->container[j+2] = tempbuff[i+j];
                }
                else
                {
                    /* Last byte flag */
                    last = 1;
                    break;
                }
            }
#ifdef DEBUG
            printf("j = %d\n",j);
#endif
            /* Store size of container in second byte*/
            newq->container[1] = j;
            /* If last container */
            if (last)
            {
                /* Set type to 1 for last item in queue */
                newq->container[0] = 1;
            }
            else
            {
                /* Set type to 0 for more items to store */
                newq->container[0] = 0;
            }
            
            /* Set pointer to next to null */
            newq->next = NULL;
            /* If first item in queue */
            if(head == NULL)
            {
#ifdef DEBUG
                printf("Entering head == NULL if statement\n");
#endif
                /* Point head and tail of queue to newq */
                head = newq;
                tail = newq;
                /* Set only one container flag set to true */
                only_one = 1;
            }
            else
            {
#ifdef DEBUG
                printf("Entering else for head == NULL if statement\n");
#endif
                /* Else, next value after tail to newq  */
                tail->next = newq;
                /* Replace tail with newq */
                tail = newq;
                /* Only one flag set to false */
                only_one = 0;
            }
            /* Update count of bytes processed */
            i += j;
            cont_size =  tail->container[1];
#ifdef DEBUG
            /* printf("Debug Transmit: Container: %s",tail->container); */
            printf("Print length: %d \n", hstate->sendPacketBuff.length);
            printf("Debug transmit: Container.\nSize: %d\n",cont_size);
            for(j = 0; j < cont_size; j += 1)
                printf("%c",tail->container[j+2]);
            printf("\n\n");
            printf("Debug transmit: Head.\n");
            for(j = 0; j < cont_size; j += 1)
                printf("%c",head->container[j+2]);
#endif
        }
        
        /* Transmit the packet on the link */
        
        /* While there are still containers in the queue */
        while(head != NULL)
        {
#ifdef DEBUG
            printf("Entering while loop\n");
#endif
            /* While there are still bytes to process in the container */
            cont_size = head->container[1];
            for(i = 0; i < cont_size+2; i += 1)
            {
                /* Store ith byte of container to payload */
                hstate->sendPacketBuff.payload[i] = head->container[i];
            }
            /* Point old at head */
#ifdef DEBUG
            printf("Container served\n");
            printf("Size of serve: %d\n",head->container[1]);
#endif
            old = head;
#ifdef DEBUG
            printf("old = head\n");
#endif
            /* Point head of queue to next container */
            head = head->next;
#ifdef DEBUG
            printf("head = head->next\n");
#endif
            /* Free old */
            free(old);
            old = NULL;
#ifdef DEBUG
            printf("old freed\n");
#endif
            /* Send last container */
            linkSend(&(hstate->linkout), &(hstate->sendPacketBuff));
#ifdef DEBUG
            printf("Container sent\n");
#endif
            /* Sleep 10ms */
#ifdef DEBUG
            printf("Begin delay\n");
#endif
            usleep(TENMILLISEC);
#ifdef DEBUG
            printf("End delay\n");
#endif
            if (only_one)
                break;
        }
        /* Clear out the packet buffer */
        head = NULL;
        hstate->sendPacketBuff.payload[0] = '\0';
        hstate->sendPacketBuff.length = 0;
    }


/* Message to be sent back to the manager */
strcpy(replymsg,"Packet sent");
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
void hostMain(hostState * hstate)
{
char buffer[1000]; /* The message from the manager */
char word[1000];
int  value;
char replymsg[1000];   /* Reply message to be displayed at the manager */
packetBuffer tmpbuff;
    int flag; /* Flag to check status of received packet */
    unsigned char payloadbuff[2001];  /* Array to hold received payloads */
    int i, j;
    unsigned int cont_size;
    int onepack;


while(1) {

   /* Check if there is a command message from the manager */
   int length; /* Size of string in pipe */
   length = hostCommandReceive(&(hstate->manLink),buffer);

   if (length > 1) { /* Execute the manager's command */
      findWord(word, buffer, 1);
      if (strcmp(word, "SetNetAddr")==0) {
         findWord(word, buffer, 2); /* Find net address */
         value = ascii2Int(word);   /* Convert it to integer */
         hostSetNetAddr(hstate, value, replymsg);
         hostReplySend(&(hstate->manLink),"DISPLAY",replymsg);
      }
      else if (strcmp(word, "SetMainDir")==0) {
         findWord(word, buffer, 2); /* Find directory name */
         hostSetMainDir(hstate, word, replymsg);
         hostReplySend(&(hstate->manLink),"DISPLAY",replymsg);
      }
      else if (strcmp(word, "ClearRcvFlg")==0) {
         hostClearRcvFlg(hstate, replymsg);
         hostReplySend(&(hstate->manLink),"DISPLAY",replymsg);
      }
      else if (strcmp(word, "UploadPacket")==0) {
         findWord(word, buffer, 2); /* Find file name */
         hostUploadPacket(hstate, word, replymsg); 
         hostReplySend(&(hstate->manLink), "DISPLAY",replymsg);
      }
      else if (strcmp(word, "DownloadPacket")==0) {
         findWord(word, buffer, 2); /* Find file name */
         hostDownloadPacket(hstate, word, replymsg);
         hostReplySend(&(hstate->manLink), "DISPLAY",replymsg);
      }
      else if (strcmp(word, "GetHostState")==0) {
         hostGetHostState(hstate, &(hstate->manLink), replymsg);
         hostReplySend(&(hstate->manLink), "GetHostStateAck",replymsg);
      }
      else if (strcmp(word, "TransmitPacket")==0) {
         hostTransmitPacket(hstate, buffer, replymsg);
         hostReplySend(&(hstate->manLink), "DISPLAY", replymsg);
      }
   } /* end of if */

   /* Check if there is an incoming packet */
   linkReceive(&(hstate->linkin), &tmpbuff);

   /* 
    * If there is a packet and if the packet's destination address 
    * is the host's network address then store the packet in the
    * receive packet buffer
    */
//   if (tmpbuff.dstaddr == hstate->netaddr && tmpbuff.valid == 1 && tmpbuff.new == 1) {
//      hstate->rcvPacketBuff = tmpbuff;
//      hstate->rcvPacketBuff.new = 1;
//      hstate->rcvPacketBuff.valid = 1;
//   }
    
    
    if (tmpbuff.dstaddr == hstate->netaddr && tmpbuff.valid == 1 && tmpbuff.new == 1 && tmpbuff.length > 200) {
#ifdef DEBUG2
        printf("tempbuff output: %s\n",tmpbuff.payload);
#endif
#ifdef DEBUG
        printf("Display contents of tempbuff:\n%s\n",tmpbuff.payload);
#endif
#ifdef DEBUG
        printf("Enter receive loop\n");
#endif
#ifdef DEBUG
        printf("Contents of payloadbuff: %s\n",payloadbuff);
#endif
        cont_size = tmpbuff.payload[1];
        /* If this is the first packet */
        if(payloadbuff[0] == '\0')
        {
            /* Copy payload to payload buffer */
            for(i = 0; i < cont_size; i += 1)
            {
                payloadbuff[i] = tmpbuff.payload[i+2];
            }
            j = i;
            /* Only packet sent */
            onepack = 1;
#ifdef DEBUG
            printf("First container\n");
            printf("i = %d\nj = %d\n",i,j);
#endif
        }
        else
        {
            /* Else, add to end of payload buffer */
            for(i = 0; i < cont_size; i += 1)
            {
                /* Ignore TVL values */
                payloadbuff[i+j] = tmpbuff.payload[i+2];
            }
            /* Update count of bytes processed */
            j += i;
            /* More then one packet sent */
            onepack = 0;
#ifdef DEBUG
            printf("Adding container\n");
            printf("i = %d\nj = %d\n",i,j);
#endif
        }
        /* If this is the last packet */
        if(tmpbuff.payload[0] == 1)
        {
#ifdef DEBUG
            printf("Last container\n");
            printf("i = %d\nj = %d\n",i,j);
#endif
            /* Copy payload buffer back to buffer in tmpbuff */
            for(i = 0; i < (unsigned int) tmpbuff.length; i += 1)
            {
                /* Copy ith element of payload buffer to tmpbuff */
                tmpbuff.payload[i] = payloadbuff[i];
                /* Clear out ith element of payload buffer */
                payloadbuff[i] = '\0';
#ifdef DEBUG
                printf("%c", tmpbuff.payload[i]);
#endif
            }
            /* Mark end of tempbuff.payload and clear payloadbuff */
            payloadbuff[0] = '\0';
            tmpbuff.payload[i] = '\0';
            tmpbuff.length = j;
            /* Copy over tmpbuff to rcvPacketBuff */
            hstate->rcvPacketBuff = tmpbuff;
            hstate->rcvPacketBuff.new = 1;
            hstate->rcvPacketBuff.valid = 1;
#ifdef DEBUG
            printf("Display contents of tempbuff:\n%s\n",tmpbuff.payload);
#endif
        }
    }
    else if (tmpbuff.dstaddr == hstate->netaddr && tmpbuff.valid == 1 && tmpbuff.new == 1 && tmpbuff.length <= 200) {
        hstate->rcvPacketBuff = tmpbuff;
        hstate->rcvPacketBuff.new = 1;
        hstate->rcvPacketBuff.valid = 1;
    }
    
   /* The host goes to sleep for 10 ms */
   usleep(TENMILLISEC);

} /* End of while loop */

}

/*
 * Sets the host's network address.  Also creates a reply message
 * to the manager.
 */
void hostSetNetAddr(hostState * hstate, int netaddr, char replymsg[])
{
hstate->netaddr = netaddr;
strcpy(replymsg, "Network address is set");
}

/* 
 * The host checks the connection from the manager.  If the manager sent
 * a command, the host stores it in "command[]".  It returns the length
 * of the received message + 1.
 */ 

int hostCommandReceive(managerLink * manLink, char command[])
{
int n;
n = read(manLink->toHost[PIPEREAD],command,250);
command[n] = '\0';
return n+1;
}

/* 
 * The host sends a message to the manager.
 */
void hostToManSend(managerLink * manLink, char reply[])
{
write(manLink->fromHost[PIPEWRITE],reply,strlen(reply));
}

/* 
 * hostReplySend is called after the host executes a command 
 * from the manager. It sends a reply
 * message to the manager in the format " Replytype <reply[]>".
 * - Replytype can be
 *    o "DISPLAY" which means the rest of the message should be 
 *      displayed on the user's console
 *    o "GetHostStateACK" which means the rest of the message is the
 *      host's state
 */ 
void hostReplySend(managerLink * manLink, char replytype[], char replymsg[])
{
char reply[1000];

reply[0] = '\0';
appendWithSpace(reply, replytype);
appendWithSpace(reply, replymsg);
hostToManSend(manLink, reply);
}

/* 
 * Initializes the host.   
 */
void hostInit(hostState * hstate, int physid)
{

hostInitState(hstate, physid);     /* Initialize host's state */

/* Initialize the receive and send packet buffers */
hostInitRcvPacketBuff(&(hstate->rcvPacketBuff));  
hostInitSendPacketBuff(&(hstate->sendPacketBuff)); 
}

/* 
 * Initialize send packet buffer 
 */
void hostInitSendPacketBuff(packetBuffer * packetbuff)
{
packetbuff->valid = 0;
packetbuff->new = 0;
}


/* 
 * Upload a file in the main directory into the send packet buffer
 */
void hostUploadPacket(hostState * hstate, char fname[], char replymsg[]) 
{
char c;
FILE * fp;
char path[MAXBUFFER];  /* Path to the file */
char tempbuff[MAXBUFFER]; /* A temporary buffer */
int length, temp_length;
int i,j;

/* 
 * Upload the file into tempbuff 
 */

if (hstate->maindirvalid == 0) {
   strcpy(replymsg, "Upload aborted:  the host's main directory is not yet chosen");
   return;
}

/* Create a path to the file and then open it */
strcpy(path,"");
strcat(path, hstate->maindir);
strcat(path, "/");
strcat(path, fname);

fp = fopen(path,"rb"); 
if (fp == NULL) { /* file didn't open */
   strcpy(replymsg, "Upload aborted: the file didn't open");
   return;
}

length = fread(tempbuff, 1, PAYLOAD_LENGTH+1, fp);

if (length==0) {
   strcpy(replymsg, "Upload aborted: error in reading the file");
   return;
}
else if (length > PAYLOAD_LENGTH) {
   strcpy(replymsg, "Upload aborted: file is too big");
   return;
}

tempbuff[length] = '\0';

/* Fill in send packet buffer */

hstate->sendPacketBuff.valid=1;
hstate->sendPacketBuff.length=length;

for (i=0; i<length; i++) { /* Store tempbuff in payload of packet buffer */
   hstate->sendPacketBuff.payload[i] = tempbuff[i];
}

    hstate->sendPacketBuff.payload[i] = '\0';
/* Message to the manager */
strcpy(replymsg, "Upload successful");

fclose(fp);
}

/* 
 * Initialize receive packet buffer 
 */ 

void hostInitRcvPacketBuff(packetBuffer * packetbuff)
{
packetbuff->valid = 0;
packetbuff->new = 0;
}

/*
 * Download the payload of the packet buffer into a 
 * file in the main directory
 */

void hostDownloadPacket(hostState * hstate, char fname[], char replymsg[]) 
{
char c;
FILE * fp;
char path[MAXBUFFER];  /* Path to the file */

/* Create a path to the file and then open it */

if (hstate->rcvPacketBuff.valid == 0) {
   strcpy(replymsg, "Download aborted: the receive packet buffer is empty");
   return;
}

if (hstate->maindirvalid == 0) {
   strcpy(replymsg, "Download aborted: the host's main directory is not yet chosen");
   return;
}

strcpy(path,"");
strcat(path, hstate->maindir);
strcat(path, "/");
strcat(path, fname);
printf("host:  path to the file: %s\n",path);
fp = fopen(path,"wb"); 

/* Download the packet buffer payload into the file */
if (hstate->rcvPacketBuff.new == 1) {
   fwrite(hstate->rcvPacketBuff.payload,1,hstate->rcvPacketBuff.length,fp);
   hstate->rcvPacketBuff.new=0;
}

/* Message sent to the manager */
strcpy(replymsg, "Download successful");
fclose(fp);
} 

/* 
 * Clear the receive packet buffer
 */
void hostClearRcvFlg(hostState * hstate, char replymsg[])
{
hstate->rcvflag = 0;
hstate->rcvPacketBuff.valid = 0;
hstate->rcvPacketBuff.new = 0;

/* Message to the manager */
strcpy(replymsg, "Host's packet received flag is cleared");
}

/* 
 * Change main directory of the host 
 */

void hostSetMainDir(hostState * hstate, char dirname[], char replymsg[])
{
strcpy(hstate->maindir, dirname);
hstate->maindirvalid = 1;

/* Message to the manager */
strcpy(replymsg, "Host's main directory name is changed");
}

/*
 * Get the host's state  
 * - host's physical ID
 * - host's main directory
 * - host's network address
 * - host's neighbor address
 * - host's receive flag
 * and create a message that has all this information to be sent
 * to the manager
 */
void hostGetHostState(hostState * hstate, managerLink * manLink, char replymsg[])
{
char word[1000];
char empty[7] = "Empty";

/* Create reply message */

replymsg[0] = '\0';  /* Clear reply message */

int2Ascii(word, hstate->physid);
appendWithSpace(replymsg, word);

if (hstate->maindirvalid==0) appendWithSpace(replymsg, empty);
else appendWithSpace(replymsg, hstate->maindir);

if (hstate->netaddr == EMPTY_ADDR) appendWithSpace(replymsg, empty);
else {
   int2Ascii(word, hstate->netaddr);
   appendWithSpace(replymsg, word);
}

if (hstate->nbraddr == EMPTY_ADDR) appendWithSpace(replymsg, empty);
else {
   int2Ascii(word, hstate->nbraddr);
   appendWithSpace(replymsg, word);
}

int2Ascii(word, hstate->rcvPacketBuff.new);
appendWithSpace(replymsg, word);

}

/* 
 * Initialize the state of the host 
 */
void hostInitState(hostState * hstate, int physid)
{
hstate->physid = physid;
hstate->maindirvalid = 0; /* main directory name is empty*/
hstate->netaddr = physid; /* default address */  
hstate->nbraddr = EMPTY_ADDR;  
hstate->rcvPacketBuff.valid = 0;
hstate->rcvPacketBuff.new = 0;
}

