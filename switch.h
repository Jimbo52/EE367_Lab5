#ifndef SWITCH_H
#define SWITCH_H

#define NUMSWITCHLINKS 200
#define NUMSWITCHS 100

typedef struct packet{
	packetBuffer packet;
	int k;
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
	int valid;
	int destNetworkAddress;
	LinkInfo outlink;
} tableEntry;

typedef struct {
	int numentries;
	tableEntry Entry[NUMSWITCHLINKS];
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

void AppendQ(switchState *swistate, packetBuffer newpacket, int k);
Packet * ServeQ(switchState *swistate);

void displayForwardTable(switchState *swistate);

void switchTransmitPacket(switchState *swistate);

#endif
