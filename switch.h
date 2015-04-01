//This file creates struct between switch and host.
//Basically, this is a combination of host.h and man.h for switch.h
/*
				       +------------------+
				       |	 packet  Q|
				       | switch 	  |
				       |	 forward T|
				       +------------------+
					^       	^	
				/	|       	|	\
			/		|       	|		\
		/			V       	V			\
	+------+		+------+        	+------+		+------+
	| host |		| host |        	| host |		| host |
	+------+		+------+        	+------+		+------+
		\			\             /			       /	
	              \			  \         /		  	/ 
			      \	            \     /   	        /
				      \	      \ /	/
					   +--------+
					   | sysman |
					   +--------+
*/
//create queue to store incoming packets.
#ifndef SWITCH_H
#define SWITCH_H

#define NUMSWITCHLINKS 100
#define MAXPACKETS 100
#define NUMSWITCHS 1

typedef struct packet{
	packetBuffer packet;
	struct packet *next;
} Packet;

typedef struct {
	Packet *head;
	Packet *tail;	
} packetqueue;

//below to create a forwardtable
typedef struct{
	int toHost[2];
	int fromHost[2];
} switchLink;

typedef struct {
	int numlinks;
	switchLink link[NUMSWITCHLINKS];
} switchLinkArrayType;

typedef struct {
	int valid[NUMSWITCHLINKS];
	int destinationAddr[NUMSWITCHLINKS];
	int linkID[NUMSWITCHLINKS];
	int count;
} forwardTable; 

typedef struct {
	int physid;	//physic id for switch, might use later but not for task1 I think
	int numlinks;
	forwardTable table;
	packetqueue packetQueue;
	switchLink swiLink;
	LinkInfo linkin[NUMSWITCHLINKS];	
	LinkInfo linkout[NUMSWITCHLINKS];
} switchState;

void switchMain(switchState * swistate);
void switchInit(switchState * swistate, int physid);

void AppendQ(switchState *swistate, packetBuffer newpacket);
Packet * ServeQ(switchState *swistate);

void displayForwardTable(switchState *swistate);

void switchTransmitPacket(switchState *swistate);

#endif
