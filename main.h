
#define PAYLOAD_LENGTH 2000 /* Maximum payload size */
#define CONT_LENGTH 200

typedef struct { /* Packet buffer */
   int srcaddr;  /* Source address */
   int dstaddr;  /* Destination addres */
   int length;   /* Length of packet */
   unsigned char payload[PAYLOAD_LENGTH + 1];  /* Payload section */
   int valid;   /* Indicates if the contents is valid */ 
   int new;     /* Indicates if the contents has been downloaded */
} packetBuffer;

typedef struct { /* Packet buffer */
   int srcaddr;  /* Source address */
   int dstaddr;  /* Destination addres */
   int length;   /* Length of packet */
   char payload[PAYLOAD_LENGTH + 1];  /* Payload section */
   int valid;   /* Indicates if the contents is valid */ 
   int new;     /* Indicates if the contents has been downloaded */
} tempBuffer;

typedef struct queue { /* Queue */
     unsigned char container[CONT_LENGTH];    /* Contents of packet */
     struct queue *next; /* Pointer to next item in queue */
} Queue;


