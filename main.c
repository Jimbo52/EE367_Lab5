#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <unistd.h>
#include <fcntl.h>

#include "main.h"
#include "utilities.h"
#include "link.h"
#include "man.h"
#include "host.h"
#include "switch.h"
#include "net.h"

#define EMPTY_ADDR  0xffff  /* Indicates that the empty address */
                             /* It also indicates that the broadcast address */
#define MAXBUFFER 1000
#define PIPEWRITE 1 
#define PIPEREAD  0
#define MAXLINKS 200

void main()
{
hostState hstate;             /* The host's state */
switchState sstate;
linkArrayType linkArray;
manLinkArrayType manLinkArray;
switchLinkArrayType switchLinkArray;

pid_t pid;  /* Process id */
int physid; /* Physical ID of host */
int i;
int k;
int otherend;

// FOR FILE READ
int numhosts, numswitchs, numlinks;
FILE *fid;
char filenm[20];
int j;
int linkbeg[MAXLINKS];
int linkend[MAXLINKS];

printf("Enter name of file for data descriptions: ");
scanf("%s",&filenm);

fid = fopen(filenm,"r");
//fscanf(fid,"%d %d %d",&numhosts,&numswitchs,&numlinks);
fscanf(fid,"Number of hosts: %d",&numhosts);
fscanf(fid," Number of switchs: %d",&numswitchs);
fscanf(fid," Number of links: %d",&numlinks);
fscanf(fid," Link endpoints (i,j):");

for(j = 0; j < numlinks; j++) {
	fscanf(fid," (%d,%d)",&linkbeg[j],&linkend[j]);
}

fclose(fid);

printf("There are \t%d hosts,\n",numhosts);
printf("\t\t%d switchs,\n",numswitchs);
printf("and\t\t%d links in the network.\n\n",numlinks);
printf("The links are: \n");
for(j = 0; j < numlinks; j++) {
	printf("(%d,%d)\n",linkbeg[j],linkend[j]);
}
printf("\n");

linkArray.numlinks = numlinks;

/* 
 * Create nonblocking (pipes) between manager and hosts 
 * assuming that hosts have physical IDs 0, 1, ... 
 */
/*
manLinkArray.numlinks = NUMHOSTS;
netCreateConnections(& manLinkArray);
*/
manLinkArray.numlinks = numhosts;
netCreateConnections(& manLinkArray);

switchLinkArray.numlinks = numlinks - numhosts;
netCreateSwitchConnections(& switchLinkArray);

/* Create links between nodes but not setting their end nodes */
/*
linkArray.numlinks = NUMLINKS;
netCreateLinks(& linkArray);
*/
linkArray.numlinks = numlinks;
netCreateLinks(& linkArray);


/* Set the end nodes of the links */

//netSetNetworkTopology(& linkArray);
netSetNetworkTopology(& linkArray, numlinks, linkbeg, linkend);

/* Create nodes and spawn their own processes, one process per node */ 

//for (physid = 0; physid < NUMHOSTS; physid++) {
for (physid = 0; physid < numhosts; physid++) {
   pid = fork();

   if (pid == -1) {
      printf("Error:  the fork() failed\n");
      return;
   }
   else if (pid == 0) { /* The child process -- a host node */

      hostInit(&hstate, physid);              /* Initialize host's state */

      /* Initialize the connection to the manager */ 
      hstate.manLink = manLinkArray.link[physid];

      /* 
       * Close all connections not connect to the host
       * Also close the manager's side of connections to host
       */
      netCloseConnections(& manLinkArray, physid);

      /* Initialize the host's incident communication links */

      k = netHostOutLink(&linkArray, physid); /* Host's outgoing link */
      hstate.linkout = linkArray.link[k];

      k = netHostInLink(&linkArray, physid); /* Host's incoming link */
      hstate.linkin = linkArray.link[k];

      /* Close all other links -- not connected to the host */
      netCloseHostOtherLinks(& linkArray, physid);

      /* Go to the main loop of the host node */
      hostMain(&hstate);
   }  
}

//SWITCH

for (physid; physid < numhosts+numswitchs; physid++) {
   pid = fork();

   if (pid == -1) {
      printf("Error:  the fork() failed\n");
      return;
   }
   else if (pid == 0) { /* The child process -- a host node */

      switchInit(&sstate, physid);              /* Initialize host's state */

      /* Initialize the connection to the manager */ 
      sstate.sLink = switchLinkArray.link[physid];

      /* 
       * Close all connections not connect to the host
       * Also close the manager's side of connections to host
       */
      netCloseSwitchConnections(& switchLinkArray, physid);

      /* Initialize the host's incident communication links */
      for(k = -1;k < numlinks;) {
      k = netSwitchOutLink(&linkArray, physid,k+1); /* Host's outgoing link */ 
      if(k < numlinks)
      {
          sstate.linkout[sstate.numlinks] = linkArray.link[k];
       //   printf("Outgoing link: %d\n",k);
          sstate.numlinks++;
      }
      }
      otherend = 0;
      for(k = -1; k < numlinks;) { 
      k = netSwitchInLink(&linkArray, physid,k+1); /* Host's incoming link */
//      otherend = linkArray.link[k].uniPipeInfo.physIdSrc;
      if(k < numlinks)
      {
         sstate.linkin[otherend] = linkArray.link[k];
      //   printf("Incoming link: %d\n",k);
         otherend++; 
      }
      }

      /* Close all other links -- not connected to the host */
      netCloseHostOtherLinks(& linkArray, physid);

      /* Go to the main loop of the host node */
      switchMain(&sstate);
   }  
}

/* Manager */

/* 
 * The manager is connected to the hosts and doesn't
 * need the links between nodes
 */

/* Close all links between nodes */
netCloseLinks(&linkArray);

/* Close the host's side of connections between a host and manager */
netCloseManConnections(&manLinkArray);

/* Go to main loop for the manager */
manMain(& manLinkArray);

/* 
 * We reach here if the user types the "q" (quit) command.
 * Now if we don't do anything, the child processes will continue even
 * after we terminate the parent process.  That's because these
 * child proceses are running an infinite loop and do not exit 
 * properly.  Since they have no parent, and no way of controlling
 * them, they are called "zombie" processes.  Actually, to get rid
 * of them you would list your processes using the LINUX command
 * "ps -x".  Then kill them one by one using the "kill" command.  
 * To use the kill the command just type "kill" and the process ID (PID).
 *
 * The following system call will kill all the children processes, so
 * that saves us some manual labor
 */
kill(0, SIGKILL); /* Kill all processes */
}




