//This file creates struct between switch and host.
//Basically, this is a combination of host.h and man.h for switch.h

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

//create queue to store incoming packets.

#define NUMLINKS 200
#define MAXPACKETS 100;

typedef struct {
	packetBuffer packet[MAXPACKETS];
	int head;	//index of head
	int tail;	//index of tail
} packetqueue;

//below to create a forwardtable
typedef struct{
	int toHost[2];
	int fromHost[2];
} switchLink;

typedef struct {
	int numlinks;
	switchLink link[NUMLINKS];
} switchLinkArrayType;

typedef struct {
	int valid[NUMLINKS];
	int destinationAddr[NUMLINKS];
	int linkID[NUMLINKS];
	int count;
} forwardTable; 

typedef struct {
	int physid;	//physic id for switch, might use later but not for task1 I think
	int rcvflag;
	int numlinks;
	packetBuffer sendpacketBuff;
	packetBuffer rcvPacketBuff;
	switchLink swiLink;
	LinkInfo linkin[NUMLINKS];	
	LinkInfo linkout[NUMLINKS];
} switchState;

void switchMain(switchState * swistate);

void switchInit(switchState * swistate, int physid);

//operation for pakcetqueue, a new packet arrivals at switch or a packet leave for a host
void AppendQ(packetqueue *q, packetBuffer newpacket);

packetBuffer ServeQ(packetqueue *q);
