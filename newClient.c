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

int userID;
//controls child in the fork
int *exit_flag;


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
void recv_from_server(); //Receive messages from the server


int main(int argc, char *argv[])
{
    exit_flag = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0); 
    //signIn();
    //recv_from_server();
    //return(0);
    int sock;                        /* Socket descriptor */
    struct sockaddr_in echoServAddr; /* Echo server address */
    struct sockaddr_in fromAddr;     /* Source address of echo */
    unsigned short echoServPort;     /* Echo server port */
    unsigned int fromSize;           /* In-out of address size for recvfrom() */
    char *servIP;                    /* IP address of server */
    struct ClientMessage client_MSG = {0,1234,5555,"Hello World\0"};                /* String to send to echo server */
    struct ServerMessage server_MSG;      /* Buffer for receiving echoed string */
    int echoStringLen;               /* Length of string to echo */
    int respStringLen;               /* Length of received response */
    char *echoString; 
    
    
    servIP = argv[1];           /* First arg: server IP address (dotted quad) */
    echoString = argv[2];       /* Second arg: string to echo */
    
    if ((echoStringLen = strlen(client_MSG.message)) > ECHOMAX)  /* Check input length */
        DieWithError("Echo word too long");
    
    if (argc == 4)
        echoServPort = atoi(argv[3]);  /* Use given port, if any */
    else
        echoServPort = 7;  /* 7 is the well-known port for the echo service */
    
    /* Create a datagram/UDP socket */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");
    
    /* Construct the server address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));    /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                 /* Internet addr family */
    echoServAddr.sin_addr.s_addr = inet_addr(servIP);  /* Server IP address */
    echoServAddr.sin_port   = htons(echoServPort);     /* Server port */
    
    /* Send the string to the server */
    if (sendto(sock, &client_MSG, sizeof(struct ClientMessage), 0, (struct sockaddr *)
               &echoServAddr, sizeof(echoServAddr)) != sizeof(struct ClientMessage))
        DieWithError("sendto() sent a different number of bytes than expected");
    
    /* Recv a response */
    fromSize = sizeof(fromAddr);
    if ((respStringLen = recvfrom(sock, &server_MSG, sizeof(struct ServerMessage), 0,
         (struct sockaddr *) &fromAddr, &fromSize)) != sizeof(struct ServerMessage))
        DieWithError("recvfrom() failed");
    
    if (echoServAddr.sin_addr.s_addr != fromAddr.sin_addr.s_addr)
    {
        fprintf(stderr,"Error: received a packet from unknown source.\n");
        exit(1);
    }
    
    /* null-terminate the received data */
    printf("Received: %s\n", server_MSG.message);    /* Print the echoed arg */
    
    close(sock);
    exit(0);
}

//Sets the userID. Sign in is actually handled by child in fork()
void signIn()
{
	printf("Please Enter Your User ID: ");
	scanf("%d",&userID);
	printf("Your ID is: %d\n",userID);
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
}
//Send a message to followers
void send_msg()
{
	struct ClientMessage c;
	printf("Enter the message you want to send:\n");
	scanf("%s",c.message);
	if(strlen(c.message) >= 100)
	{
		printf("Your message must be under 100 characters\n");
		return;
	}

	 c.UserID = userID;
	 c.request_Type = 1;
	 send_to_server(&c);
} 
void send_to_server(struct ClientMessage *msg)
{
	printf("Sending message type %d from %d\n",(*msg).request_Type,(*msg).UserID);
} //Sends the ClientMessage to the server
void recv_from_server()
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
