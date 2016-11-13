#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <sys/mman.h>   /* for mmap() */
#include <string.h>     /* for string strcpy() and strlen() */ 

#define ECHOMAX 100     /* Longest string to echo */
/*Global Variables*/

int userID;         /* The user's ID */
int *exit_flag;     /* controls child in the fork */
int sock;           /* Socket descriptor */ 
struct sockaddr_in echoServAddr; /* Echo server address */
unsigned short echoServPort;     /* Echo server port */

struct ClientMessage{
	enum {Login, Post, Activate, Subscribe, Unsusbscribe, Logout }
	request_Type;                                    /* same size as an unsigned int */
	unsigned int UserID;                             /* unique client identifier */
	unsigned int LeaderID;                           /* unique client identifier */
	char message[100];                               /* text message */
};                                     /* an unsigned int is 32 bits = 4 bytes */

struct ServerMessage{
	unsigned int LeaderID;                           /* unique client identifier */
	char message[100];                               /* text message */
};                                     /* an unsigned int is 32 bits = 4 bytes */

void DieWithError(char *errorMessage);  /* External error handling function */
void signIn(); //Client Signs in
void signOut(); //Client Signs Out
void follow(); //Client follows a Leader
void unfollow(); //Client follows a Leader
void get_msg(); //Gets saved messages from server
void send_msg(); //Send a message to followers
void send_to_server(struct ClientMessage *msg); //Sends the ClientMessage to the server
void run_client(); //Receive messages from the server


int main(int argc, char *argv[])
{
    exit_flag = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);  
    *exit_flag = 0;
    struct sockaddr_in fromAddr;     /* Source address of echo */
    unsigned int fromSize;           /* In-out of address size for recvfrom() */
    char servIP[16];                    /* IP address of server */
    struct ServerMessage server_MSG;      /* Buffer for receiving echoed string */
    int echoStringLen;               /* Length of string to echo */
    int respStringLen;               /* Length of received response */
    char serverport[10]; 
    
    printf("Please Enter Your User ID: ");
	scanf("%d",&userID);
	printf("Your ID is: %d\n",userID);  
    printf("Please enter the server IP Address: ");
    scanf("%s",servIP);
    printf("Please enter the server's port number: ");
    scanf("%s",serverport);
    echoServPort = atoi(serverport);
    
    
    /* Create a datagram/UDP socket */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");
    
    /* Construct the server address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));    /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                 /* Internet addr family */
    echoServAddr.sin_addr.s_addr = inet_addr(servIP);  /* Server IP address */
    echoServAddr.sin_port   = htons(echoServPort);     /* Server port */
    
    
    fromSize = sizeof(fromAddr);
    /* Recv a response */

	if (fork() == 0){
    	int x;
  	 	for (;;){
			recvfrom(sock, &server_MSG, sizeof(struct ServerMessage), 0,(struct sockaddr *) &fromAddr, &fromSize);
			if (echoServAddr.sin_addr.s_addr != fromAddr.sin_addr.s_addr)
			{
				fprintf(stderr,"Error: received a packet from unknown source.\n");
				exit(1);
			}
			printf("%d said: %s\n",server_MSG.LeaderID,server_MSG.message);


    	}
	   printf("Hello Child\n");
	   exit(0);

	}    
    
    signIn();
    run_client();
    close(sock);
    exit(0);
}

//Sets the userID. Sign in is actually handled by child in fork()
void signIn()
{
	struct ClientMessage c;
	c.request_Type = 0;
	c.UserID = userID;
	printf("Signing in\n");
	send_to_server(&c);
}

//Client Signs Out. Sets exit_flag to stop receiving, sends a sign out request
void signOut()
{
	struct ClientMessage c;
	c.request_Type = 5;
	c.UserID = userID;
	*exit_flag = -1;
	printf("Signing out\n");
	send_to_server(&c);
} 

void follow() //Client follows a Leader. 
{
	printf("Who would you like to follow:\n");
	struct ClientMessage c;
	c.request_Type = 3;
	c.UserID = userID;
	scanf("%d", &c.LeaderID);
	printf("You have followed %d\n", c.LeaderID);
	send_to_server(&c);
}
//Client unfollows a Leader
void unfollow()
{
	printf("Who would you like to unfollow:\n");
	struct ClientMessage c;
	c.request_Type = 4;
	c.UserID = userID;
	scanf("%d", &c.LeaderID);
	printf("You have unfollowed %d\n", c.LeaderID);
	send_to_server(&c);
} 
//This sends a request to send stored messages to the client
void get_msg()
{
	struct ClientMessage c;
	c.request_Type = 2;
	c.UserID = userID;
	send_to_server(&c);
	return;
}
//Send a message to followers
void send_msg()
{
	struct ClientMessage c;
	fseek(stdin,0,SEEK_END);
	printf("Enter the message you want to send:\n");
	fgets(c.message, 100, stdin);
	//scanf("%s",c.message);
	fseek(stdin,0,SEEK_END);
	if(strlen(c.message) >= 100)
	{
		printf("Your message must be under 100 characters\n");
		return;
	}

	 c.UserID = userID;
	 c.request_Type = 1;
	 send_to_server(&c);
	 return;
} 
void send_to_server(struct ClientMessage *msg)
{
	printf("Sending message type %d from %d message: %s\n",(*msg).request_Type,(*msg).UserID,(*msg).message);
	    /* Send the string to the server */
		sendto(sock, msg, sizeof(*msg), 0, (struct sockaddr *) &echoServAddr, 
			sizeof(echoServAddr));
	
} //Sends the ClientMessage to the server
void run_client()
{
	int selection;
	//Loop with switch statement drives the user interface
	//User keeps entering options until they quit
	do
	{
		printf("Please Select an Option\n");
		printf("1-Follow a User\n");
		printf("2-Unfollow a User\n");
		printf("3-Get Messages\n");
		printf("4-Send Message\n");
		printf("5-Sign Out\n");
		scanf("%d",&selection);
	
		switch(selection)
		{
			case 1: //follow a user
			{
				follow();
				break;
			}
			case 2: //unfollow a user
			{
				unfollow();
				break;
			}
			case 3: //Get messages
			{
				get_msg();
				break;
			}
			case 4: //Send message
			{
				send_msg();
				break;
			}
			case 5: //log out
			{
				signOut();
				break;
			}		
			default: //Error for incorrect entries
				printf("Your entry is invalid. Please try again\n");
		}
	}while(selection != 5); //do until user logs out
} //Receive messages from the server
