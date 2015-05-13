#ifndef SWITCH_H
#define SWITCH_H

#define NUMSWITCHLINKS 200
#define LOCAL_PACKET_SIZE 11

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

typedef struct SwitchState{
	int root;
	int distance;
	struct SwitchState *parent;
	char child[NUMSWITCHLINKS];
	char datalink[NUMSWITCHLINKS];
	char TLVpacket[LOCAL_PACKET_SIZE];

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
void generateStatePacket(switchState *swistate, int linkout);
void processStatePacket(switchState *swistate, char packet[], LinkInfo *link, int k);

void displayForwardTable(switchState *swistate);

void switchTransmitPacket(switchState *swistate);
void switchTransmitState(switchState *swistate);
void switchReceiveState(LinkInfo *link, switchState *swistate, int k);

#endif
