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
void send_msg(struct ClientMessage *msg); //Client Sends a message to followers
void send_to_client(struct sockaddr_in *clientAddr, struct ServerMessage *msg); //Sends the ClientMessage to the server
void recv_from_client(struct ClientMessage *msg); //Decides which request is performed
static int build_stored(void *NotUsed, int argc, char **argv, char **azColName);
static int build_active(void *message, int argc, char **argv, char **azColName);
static int store_msg(void *message, int argc, char **argv, char **azColName);

char stored_messages[5096];

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
    client_MSG.LeaderID = 2345;
    strcpy(client_MSG.message, "Hello Fancy");
    echoClntAddr.sin_addr.s_addr = inet_addr("192.123.123.123");
    echoClntAddr.sin_port   = htons(22345);
    signOut(&client_MSG);
    signIn(&client_MSG,&echoClntAddr);
    follow(&client_MSG);
    unfollow(&client_MSG);
    get_msg(&client_MSG);
    send_msg(&client_MSG);
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
   char sql[100];
   memset(sql, 0, sizeof(char)*100);
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
    strcat(sql, ",\"");
    strcat(sql, inet_ntoa((*ClntAddr).sin_addr));
    strcat(sql, "\",");
    char userport[10];
    sprintf(userport,"%hd",(*ClntAddr).sin_port);
    strcat(sql, userport);
    strcat(sql, ");");
    printf("%s\n",sql);
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
//Client Signs Out
void signOut(struct ClientMessage *msg)
{
   sqlite3 *db;
   char *zErrMsg = 0;
   int rc;
   char sql[100];
   memset(sql, 0, sizeof(char)*100);
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
    strcat(sql,"DELETE FROM SignIn WHERE USR=");
    char userstring[10];
    sprintf(userstring,"%d",(*msg).UserID);
    strcat(sql,userstring);
    strcat(sql, ";");
    printf("%s\n",sql);
   /* Execute SQL statement */
   rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);
   if( rc != SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
   }
   else{
      fprintf(stdout, "User %d has signed out\n", (*msg).UserID);
   }
   sqlite3_close(db);
}
//Client follows a Leader
void follow(struct ClientMessage *msg)
{
   sqlite3 *db;
   char *zErrMsg = 0;
   int rc;
   char sql[100];
   memset(sql, 0, sizeof(char)*100 );
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
    strcat(sql,"INSERT INTO Follow (USR,LDR)VALUES (");
    char userstring[10];
    char leaderstring[10];
    sprintf(userstring,"%d",(*msg).UserID);
    strcat(sql,userstring);
    strcat(sql, ",");
    sprintf(leaderstring,"%d",(*msg).LeaderID);
    strcat(sql,leaderstring);
    strcat(sql, ");");
    printf("%s\n",sql);
   /* Execute SQL statement */
   	rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);
   	if( rc != SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
   }
   else{
      fprintf(stdout, "User %d has followed %d\n", (*msg).UserID, (*msg).LeaderID);
   }
   
   sqlite3_close(db);
}
//Client unfollows a Leader
void unfollow(struct ClientMessage *msg)
{
   sqlite3 *db;
   char *zErrMsg = 0;
   int rc;
   char sql[100];
   memset(sql, 0, sizeof(char)*100 );
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
    strcat(sql,"DELETE FROM follow WHERE USR= ");
    char userstring[10];
    sprintf(userstring,"%d",(*msg).UserID);
    strcat(sql,userstring);
    strcat(sql, " AND LDR= ");
    char leaderstring[10];
    sprintf(leaderstring,"%d",(*msg).LeaderID);
    strcat(sql,leaderstring);
    strcat(sql, ";");
    printf("%s\n",sql);
   /* Execute SQL statement */
   	rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);
   	if( rc != SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
   }
   else{
      fprintf(stdout, "User %d has unfollowed %d\n", (*msg).UserID, (*msg).LeaderID);
   }
   sqlite3_close(db);
}
//Server Gets saved messages for client
void get_msg(struct ClientMessage *msg)
{
	sqlite3 *db;
	char *zErrMsg = 0;
	int rc;
	char sql[512];
	char rm_msg[512];
	memset(sql, 0, sizeof(char)*512 );
	memset(rm_msg, 0, sizeof(char)*512 );

	/* Open database */
	rc = sqlite3_open("message.db", &db);
	if( rc ){
	  fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
	  return;
	}else{
	  fprintf(stderr, "Opened database successfully\n");
	}

	/* Create SQL statement */
	strcat(sql,"SELECT M.LDR, M.MSG, S.IP, S.PORT FROM MSG M, SignIn S WHERE S.USR = M.USR AND S.USR = ");
    char userstring[10];
    sprintf(userstring,"%d",(*msg).UserID);
    strcat(sql,userstring);
    strcat(sql, ";");
    printf("%s\n",sql);
    strcat(rm_msg, "DELETE FROM MSG WHERE USR= ");
    strcat(rm_msg, userstring);
    strcat(rm_msg, ";");
    printf("%s\n",rm_msg);
    
	/* Execute SQL statement */
	rc = sqlite3_exec(db, sql, build_stored, 0, &zErrMsg);
	if( rc != SQLITE_OK ){
	  fprintf(stderr, "SQL error: %s\n", zErrMsg);
	  sqlite3_free(zErrMsg);
	}else{
	  fprintf(stdout, "Stored messages sent to %s\n", userstring);
	}
	rc = sqlite3_exec(db, rm_msg, 0, 0, &zErrMsg);
	if( rc != SQLITE_OK ){
	  fprintf(stderr, "SQL error: %s\n", zErrMsg);
	  sqlite3_free(zErrMsg);
	}else{
	  fprintf(stdout, "Deleted stored messages for %s\n", userstring);
	}
	sqlite3_close(db);
}
/*This builds a server message and a sockaddr_in struct to send the client their messages
  Each message is packaged and sent to the sender function until all messages have been sent*/
static int build_stored(void *NotUsed, int argc, char **argv, char **azColName){
	struct sockaddr_in clientAddr;
	struct ServerMessage server_MSG;
	memset(&clientAddr, 0, sizeof(clientAddr));   /* Zero out structure */
	memset(&server_MSG, 0, sizeof(server_MSG));   /* Zero out structure */
	clientAddr.sin_family = AF_INET;                /* Internet address family */
	clientAddr.sin_addr.s_addr = inet_addr(argv[2]); /* Client's IP Address */
	clientAddr.sin_port = atoi(argv[3]);         /* Client's port */
	server_MSG.LeaderID = atoi(argv[0]);                 /* Who MSG is from */
	strcpy(server_MSG.message, argv[1]);                  /* The Message */
	send_to_client(&clientAddr, &server_MSG);
	printf("%s %s %s %s\n",argv[0],argv[1],argv[2],argv[3]);
	  //printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	return 0;
}
//Client requests messages to be sent to their followers
void send_msg(struct ClientMessage *msg)
{
	sqlite3 *db;
	char *zErrMsg = 0;
	int rc;
	char sql[512];
	memset(sql, 0, sizeof(char)*512);
	char stored_sql[512];
	memset(stored_sql, 0, sizeof(char)*512);
	/* Open database */
	rc = sqlite3_open("message.db", &db);
	if( rc ){
	  fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
	  return;
	}else{
	  fprintf(stderr, "Opened database successfully\n");
	}

	/* Create SQL statement */
	strcat(sql,"SELECT S.USR, F.LDR, S.IP, S.PORT FROM SignIn S, Follow F WHERE F.LDR = ");
    char userstring[10];
    sprintf(userstring,"%d",(*msg).UserID);
    strcat(sql,userstring);
    strcat(sql, " AND S.USR = F.USR;");
    printf("%s\n",sql);
    strcat(stored_sql, "SELECT F.USR, F.LDR FROM Follow F WHERE F.LDR = ");
    strcat(stored_sql,userstring);
    strcat(stored_sql," AND F.USR NOT IN (SELECT S.USR FROM SignIn S);");
    
	/* Execute SQL statement */
	rc = sqlite3_exec(db, sql, build_active, (void*)(*msg).message, &zErrMsg);
	if( rc != SQLITE_OK ){
	  fprintf(stderr, "SQL error: %s\n", zErrMsg);
	  sqlite3_free(zErrMsg);
	}else{
	  fprintf(stdout, "Messages sent from %s\n", userstring);
	}
	memset(stored_messages, 0, sizeof(char)*5096);
	rc = sqlite3_exec(db, stored_sql, store_msg, (void*)(*msg).message, &zErrMsg);
	if( rc != SQLITE_OK ){
	  fprintf(stderr, "SQL error: %s\n", zErrMsg);
	  sqlite3_free(zErrMsg);
	}else{
	  fprintf(stdout, "Messages ready to store from %s\n", userstring);
	}
	rc = sqlite3_exec(db, stored_messages, 0, 0, &zErrMsg);
	if( rc != SQLITE_OK ){
	  fprintf(stderr, "SQL error: %s\n", zErrMsg);
	  sqlite3_free(zErrMsg);
	}else{
	  fprintf(stdout, "Messages stored from %s\n", userstring);
	}
	sqlite3_close(db);
}
static int build_active(void *message, int argc, char **argv, char **azColName){
	struct sockaddr_in clientAddr;
	struct ServerMessage server_MSG;
	memset(&clientAddr, 0, sizeof(clientAddr));   /* Zero out structure */
	memset(&server_MSG, 0, sizeof(server_MSG));   /* Zero out structure */
	clientAddr.sin_family = AF_INET;                /* Internet address family */
	clientAddr.sin_addr.s_addr = inet_addr(argv[2]); /* Client's IP Address */
	clientAddr.sin_port = atoi(argv[3]);         /* Client's port */
	server_MSG.LeaderID = atoi(argv[1]);                 /* Who MSG is from */
	strcpy(server_MSG.message, (char*)message);                  /* The Message */
	send_to_client(&clientAddr, &server_MSG);
	printf("to %s from %s\n",argv[0],argv[1]);
	  //printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	return 0;
}
static int store_msg(void *message, int argc, char **argv, char **azColName){
	sqlite3 *db;
	char *zErrMsg = 0;
	int rc;
	char sql[512];
	memset(sql, 0, sizeof(char)*512);
	/* Open database */
	rc = sqlite3_open("message.db", &db);
	if( rc ){
	  fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
	  return 0;
	}else{
	  fprintf(stderr, "Opened database successfully\n");
	}

	/* Create SQL statement */
	strcat(sql,"INSERT INTO MSG (USR,LDR,MSG) VALUES (");
    strcat(sql,argv[0]);
    strcat(sql, ",");
        strcat(sql,argv[1]);
    strcat(sql, ",\"");
    strcat(sql,(char*)message);
    strcat(sql, "\"); ");
    printf("%s\n",sql);
    strcat(stored_messages,sql);
	return 0;
}
void send_to_client(struct sockaddr_in *clientAddr, struct ServerMessage *msg)
{
	printf("Sending Message ");
}
// void recv_from_client(struct ClientMessage *msg); //Decides which request is performed
