/*  
 *  Functions for the link
 *
 *  The functions linkSend and linkReceive will send and receive
 *  packets on links (e.g., pipes).  This is currently implemented
 *  by converting the packet information into a string of bytes,
 *  where each field in the packet (i.e., source address, destination
 *  addres, length, payload) is separated by spaces.  In other words,
 *  spaces are used to delimit the fields.  The addresses
 *  and length are integers and they are converted into ascii text
 *  before putting them on the pipe.  They payload can be arbitrary
 *  bytes including spaces and '\0'.  So we need to remove these
 *  before transmission. Otherwise the receiving side of the link
 *  will unable to tell if a byte is a delimiter or part of the
 *  payload.
 *
 *  We use the following simple rule to convert
 *  these bytes into two bytes:
 *
 *  ' '  -> *A
 *  '\0' -> *B
 *  '*'  -> *C
 *
 *  For example, suppose the payload is "*Hello world*\n".  Then
 *  the conversion is "*CHello*Aworld*C".   We've eliminated all
 *  the spaces from the payload.
 *  
 *  Note that in addition to ' ', we are also converting '\0' and '*'.
 *  In the case of '*', we are using it as a control character. 
 *  So there is ambiguity about whether it is for control or data.
 *  In the case of '\0', it is a control character for string.h
 *  operations.
 *
 *  Now to recover the original payload, the receiver just does
 *  the opposite of the conversion.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

#include "utilities.h"
#include "main.h"
#include "link.h"

#define PIPEREAD  0
#define PIPEWRITE 1
//#define DEBUG

/* Closes a link */
int linkClear(LinkInfo * link)
{
int flag;

if (link->linkType == UNIPIPE) {
   close(link->uniPipeInfo.fd[0]);
   close(link->uniPipeInfo.fd[1]);
}
}

/* Initializes a link */
int linkCreate(LinkInfo * link)
{
int flag;

if (link->linkType == UNIPIPE) {
   if (pipe(link->uniPipeInfo.fd) < 0) {
      printf("Creating a pipe failed\n");
      return -1;
   }
   if (link->uniPipeInfo.pipeType == NONBLOCKING) {
      flag = fcntl(link->uniPipeInfo.fd[0], F_GETFL);
      fcntl(link->uniPipeInfo.fd[0], F_SETFL, flag|O_NONBLOCK);
      flag = fcntl(link->uniPipeInfo.fd[1], F_GETFL);
      fcntl(link->uniPipeInfo.fd[1], F_SETFL, flag|O_NONBLOCK);
   }
   else printf("LinkCreate:  unknown unipipe type\n");
   return 0;
}
else {
   printf("Link not created:  invalid type\n");
   return -1;
}
}

/*
 * Checks incoming link and if stores it in a packet buffer.
 * Returns the length of the contents on the incoming link.
 */
int linkReceive(LinkInfo * link, packetBuffer * pbuff)
{
int n;
char buffer[1000];
char word[1000];
int count;
int k;
int wordptr;

n = 0;

if (link->linkType==UNIPIPE) {
   n = read(link->uniPipeInfo.fd[PIPEREAD], buffer, 1000);
   if (n > 0) {
      /* 
       * Something is received on link. 
       * Store it in the packet buffer
       */
      buffer[n] = '\0';
//    printf("DEBUG: Link received=%s\n",buffer);
   
      findWord(word, buffer, 1); /* Destination address */
      pbuff->dstaddr = ascii2Int(word);
  
      findWord(word, buffer, 2); /* Source address */
      pbuff->srcaddr = ascii2Int(word);

      findWord(word, buffer, 3); /* Length */
      pbuff->length = ascii2Int(word);

      findWord(word, buffer, 4); /* Payload */

      /* 
       * We will transform the payload so that 
       *
       * *A -> ' '
       * *B -> '\0' 
       * *C -> '*'
       */

      k = 0;
      wordptr=0;
      for (count=pbuff->length; count>0; count--) {
         if (word[wordptr]=='*') {
            wordptr++;
            if (word[wordptr]=='A') pbuff->payload[k] = ' ';
            else if (word[wordptr]=='B') pbuff->payload[k] = '\0';
            else if (word[wordptr]=='C') pbuff->payload[k] = '*';
         }
         else pbuff->payload[k] = word[wordptr];
         wordptr++;
         k++;
      } /* end of for */
      pbuff->payload[k] = '\0';
      pbuff->valid=1;
      pbuff->new=1;
   } /* end of if */

   else { /* Not a packet */
      pbuff->valid=0;
      pbuff->new=0;
   }
}

return n; /* Return length what was received on the link */ 
}

/*
 * Sends the packet in pbuff on the outgoing link.
 */
int linkSend(LinkInfo * link, packetBuffer * pbuff)
{
char sendbuff[1000];  /* buffer to build the message */
char word[1000];
char newpayload[1000];
int  count;
int  sendbuffptr;
int  newptr;
int  k;

#ifdef DEBUG
printf("Entering linksend\n");
#endif
/* Check if this send should be aborted */
if (pbuff->valid == 0) {
   printf("packet invalid\n");
   return -1;
}

if (pbuff->length > PAYLOAD_LENGTH) {
   printf("packet too big\n");
   return -1;
} 

if (pbuff->length <= 0) {
   printf("packet too small\n");
   return -1;
}

#ifdef DEBUG
printf("After If statements\n");
#endif

sendbuff[0] = ' ';  /* Start message with a space */
sendbuff[1] = '\0';

#ifdef DEBUG
printf("After sendbuff statements\n");
#endif

int2Ascii(word, pbuff->dstaddr);  /* Append destination address */
appendWithSpace(sendbuff, word);

int2Ascii(word, pbuff->srcaddr);  /* Append source address */
appendWithSpace(sendbuff, word);

int2Ascii(word, pbuff->length);  /* Append payload length */
appendWithSpace(sendbuff, word);

/* We will transform the payload so that 
 *
 * ' '   -> *A
 * '\0'  -> *B
 * '*'   -> *C
 */

k = 0;
newptr = 0;

for (count=pbuff->payload[1]+2; count>0; count--) {
   if (pbuff->payload[k] == ' ') {
      newpayload[newptr++] = '*';
      newpayload[newptr++] = 'A';
   }
   else if (pbuff->payload[k] == '\0') {
      newpayload[newptr++] = '*';
      newpayload[newptr++] = 'B';
   } 
   else if (pbuff->payload[k] == '*') {
      newpayload[newptr++] = '*';
      newpayload[newptr++] = 'C';
   } 
   else {
      newpayload[newptr++] = pbuff->payload[k];
   } 
   k++;
}

newpayload[newptr] = '\0';

appendWithSpace(sendbuff, newpayload);

if (link->linkType==UNIPIPE) {
   write(link->uniPipeInfo.fd[PIPEWRITE],sendbuff,strlen(sendbuff)); 
}

/* Used for DEBUG -- trace packets being sent */
//printf("Link %d transmitted\n",link->linkID);
}

