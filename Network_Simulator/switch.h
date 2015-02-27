//This file creates struct between switch and host.
//Basically, this is a combination of host.h and man.h for switch.h


//create queue to store incoming packets.
typedef struct {
	packetBuffer packet;
	int head;
	int tail;
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
	int valid;
	int destinationAdd;
	int linkID;
} forwardTable; 

typedef struct {
	int physid;
	int rcvflag;
	packetBuffer packetBuff;
	switchLink swiLink;
	LinkInfo linkin[NUMLINKS];	
	LinkInfo linkout[NUMLINKS];
} swtichState;

void switchMain(switchState * swistate);

void switchInit(switchState * swistate, int physid);

//operation for pakcetqueue, a new packet arrivals at switch or a packet leave for a host
void AppendQ(packetqueue * q, packetBuffer newpacket);

packetBuffer ServeQ(packetqueue *q);
