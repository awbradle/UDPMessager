#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket() and bind() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <sqlite3.h> 
#include <string.h> 

#define ECHOMAX 100     /* Longest string to echo */

//Client Message Structure
struct ClientMessage{
	enum {Login, Post, Activate, Subscribe, Unsusbscribe, Logout }
	request_Type;                                    /* same size as an unsigned int */
	unsigned int UserID;                             /* unique client identifier */
	unsigned int LeaderID;                           /* unique client identifier */
	char message[100];                               /* text message */
};                                     /* an unsigned int is 32 bits = 4 bytes */

//Server Message Structure
struct ServerMessage{
	unsigned int LeaderID;                           /* unique client identifier */
	char message[100];                               /* text message */
} ;   

void DieWithError(char *errorMessage);  /* External error handling function */
void signIn(struct ClientMessage *msg, struct sockaddr_in *ClntAddr); //Client Signs in
void signOut(struct ClientMessage *msg); //Client Signs Out
void follow(struct ClientMessage *msg); //Client follows a Leader
void unfollow(struct ClientMessage *msg); //Client follows a Leader
void get_msg(struct ClientMessage *msg); //Client Gets saved messages from server
void send_msg(struct ClientMessage *msg); //Client Send a message to followers
void send_to_client(struct ServerMessage *msg); //Sends the ClientMessage to the server
void recv_from_client(struct ClientMessage *msg); //Decides which request is performed

int main(int argc, char *argv[])
{
    int sock;                        /* Socket */
    struct sockaddr_in echoServAddr; /* Local address */
    struct sockaddr_in echoClntAddr; /* Client address */
    unsigned int cliAddrLen;         /* Length of incoming message */
    struct ClientMessage client_MSG;        /* Buffer for echo string */
    unsigned short echoServPort;     /* Server port */
    int recvMsgSize;                 /* Size of received message */
    char *echoBuffer;
    struct ServerMessage server_MSG;
    
    client_MSG.UserID = 1234;
    echoClntAddr.sin_addr.s_addr = inet_addr("192.123.123.123");
    echoClntAddr.sin_port   = htons(22345);
    signIn(&client_MSG,&echoClntAddr);
    return 0;
    if (argc != 2)         /* Test for correct number of parameters */
    {
        fprintf(stderr,"Usage:  %s <UDP SERVER PORT>\n", argv[0]);
        exit(1);
    }
    
    echoServPort = atoi(argv[1]);  /* First arg:  local port */
    
    /* Create socket for sending/receiving datagrams */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");
    
    /* Construct local address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                /* Internet address family */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    echoServAddr.sin_port = htons(echoServPort);      /* Local port */
    
    /* Bind to the local address */
    if (bind(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        DieWithError("bind() failed");
    for (;;) /* Run forever */
    {
        /* Set the size of the in-out parameter */
        cliAddrLen = sizeof(echoClntAddr);
        
        /* Block until receive message from a client */
        if ((recvMsgSize = recvfrom(sock, &client_MSG, sizeof(struct ClientMessage), 0,
                                    (struct sockaddr *) &echoClntAddr, &cliAddrLen)) < 0)
            DieWithError("recvfrom() failed");
        
        printf("Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));
        server_MSG.LeaderID = client_MSG.LeaderID;
        strcpy(server_MSG.message,"bacon");
        /* Send received datagram back to the client */
		sendto(sock, &server_MSG, recvMsgSize, 0, 
			(struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr));
    }
    /* NOT REACHED */
}
//Client Signs in
void signIn(struct ClientMessage *msg, struct sockaddr_in *ClntAddr)
{
   sqlite3 *db;
   char *zErrMsg = 0;
   int rc;
   char sql[1024];

   /* Open database */
   rc = sqlite3_open("message.db", &db);
   if( rc ){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      return;
   }
   else{
      fprintf(stderr, "Opened database successfully\n");
   }

   /* Create SQL statement */
    strcat(sql,"INSERT INTO SignIn (USR,IP,Port)VALUES (");
    char userstring[10];
    sprintf(userstring,"%d",(*msg).UserID);
    strcat(sql,userstring);
    printf("user\n");
    strcat(sql, ",\"");
    strcat(sql, inet_ntoa((*ClntAddr).sin_addr));
    strcat(sql, "\",");
    printf("address\n");
    char userport[10];
    sprintf(userport,"%hd",(*ClntAddr).sin_port);
    strcat(sql, userport);
    strcat(sql, ");");
    printf("port\n");
    printf("%s",sql);
   /* Execute SQL statement */
   rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);
   if( rc != SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
   }
   else{
      fprintf(stdout, "User %d has signed in\n", (*msg).UserID);
   }
   sqlite3_close(db);
}
// void signOut(struct ClientMessage *msg); //Client Signs Out
// void follow(struct ClientMessage *msg); //Client follows a Leader
// void unfollow(struct ClientMessage *msg); //Client follows a Leader
// void get_msg(struct ClientMessage *msg); //Client Gets saved messages from server
// void send_msg(struct ClientMessage *msg); //Client Send a message to followers
// void send_to_client(struct ServerMessage *msg); //Sends the ClientMessage to the server
// void recv_from_client(struct ClientMessage *msg); //Decides which request is performed
