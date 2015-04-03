
void netCreateSwitchConnections(switchLinkArrayType * switchLinkArray);

/* Create nonblocking connections from the manager to all hosts */
void netCreateConnections(manLinkArrayType * manLinkArray); 

/* Create all nonblocking links */
void netCreateLinks(linkArrayType * linkArray);

/* 
 * Close all connections except the outgoing connection
 * from the host to manager and the incoming connection from
 * the manager to host
 */ 
void netCloseConnections(manLinkArrayType *  manLinkArray, int hostid);

void netCloseSwitchConnections(switchLinkArrayType * switchLinkArray, int switchid);

/* 
 * Set up the end nodes of the links -- essentially creating 
 * network topology
 */
//void netSetNetworkTopology(linkArrayType * linkArray);
void netSetNetworkTopology(linkArrayType * linkArray, int numlinks, int linkbeg[], int linkend[]);

int netSwitchOutLink(linkArrayType * linkArray, int switchid, int startindex);

/* Find host's outgoing link and return its index from the link array */
int netHostOutLink(linkArrayType * linkArray, int hostid); 

int netSwitchInLInk(linkArrayType * linkArray, int switchid, int startindex);

/* Find host's incoming link and return its index from the link array */
int netHostInLink(linkArrayType * linkArray, int hostid); 

/* Close links not used by the host */
void netClearHostOtherLinks(linkArrayType * linkArray, int hostid);

/* Close all links */
void netCloseLinks(linkArrayType * linkArray); 

/* Close the host's side of a connection between a host and manager */
void netCloseManConnections(manLinkArrayType * manLinkArray);

