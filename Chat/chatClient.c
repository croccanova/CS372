/***********************************************************************
Christian Roccanova
CS 372
Project 1
chatClient - implementation of the client portion of a chat program
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>


// create address info
// based on Beej's guide section 5.1 https://beej.us/guide/bgnet/html/single/bgnet.html#getaddrinfo
struct addrinfo* buildAddress(char* address, char* port) {
	struct addrinfo hints;
	struct addrinfo *res;
	int status;

	// empty struct
	memset(&hints, 0, sizeof hints);

	//specify version
	hints.ai_family = AF_INET;  
	
	//TCP stream sockets
	hints.ai_socktype = SOCK_STREAM;    

	//print error if status is not 0
	if ((status = getaddrinfo(address, port, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(1);
	}

	return res;
}

// based on Beej's guide section 5.2 https://beej.us/guide/bgnet/html/single/bgnet.html#socket
int buildSocket(struct addrinfo* res) {
	int sockfd;

	if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
		fprintf(stderr, "Error: Socket creation failed.\n");
		exit(1);
	}
	return sockfd;
}

// based on Beej's guide section 5.4 https://beej.us/guide/bgnet/html/single/bgnet.html#connect
void connectSocket(int sockfd, struct addrinfo * res) {
	int status;
	if ((status = connect(sockfd, res->ai_addr, res->ai_addrlen)) == -1) {
		fprintf(stderr, "Error: Connection failed.\n");
		exit(1);
	}
}

//function to handle the act of chatting
void chat(int sockfd, char * username, char * servername) {
	int bytes_sent;
	int status;

	//character buffers for messages
	char inBuff[500];
	memset(inBuff, 0, sizeof(inBuff));

	char outBuff[500];
	memset(outBuff, 0, sizeof(outBuff));

	//clear stdin
	fgets(inBuff, 500, stdin);


	//loops until either client or server ends the connection by inputting "\quit"
	while (1) {
		
		//get user input for messages
		printf("%s> ", username);
		fgets(inBuff, 500, stdin);

		//error checking based on Beej's guide section 5.7 https://beej.us/guide/bgnet/html/single/bgnet.html#sendrecv
		//send returns number of bytes sent
		bytes_sent = send(sockfd, inBuff, strlen(inBuff), 0);

		//if bytes_sent = -1, there was an error sending data
		if (bytes_sent == -1) {
			fprintf(stderr, "Error: Data not sent correctly.\n");
			exit(1);
		}

		//break if user inputs "\quit"		
		if (strcmp(inBuff, "\\quit\n") == 0) {
			printf("Terminating connection.\n");
			break;
		}

		//same as above, but for bytes received
		status = recv(sockfd, outBuff, 500, 0);

		if (status == -1) {
			fprintf(stderr, "Error: Data not received correctly.\n");
			exit(1);
		}

		//break if server input "\quit"
		else if (status == 0) {
			printf("Server terminating connection.\n");
			break;
		}
		//else print message
		else {
			printf("%s> %s\n", servername, outBuff);
		}

		//clear buffers for next round of messages
		memset(inBuff, 0, sizeof(inBuff));
		memset(outBuff, 0, sizeof(outBuff));
	}

	// close connection
	close(sockfd);
	printf("Connection closed.\n");
}

int main(int argc, char *argv[]) {
	char userName[10];
	char serverName[10];
	int sockfd;

	//check if the user input the correct number of arguments. If not, print error message
	if (argc != 3) {
		fprintf(stderr, "Error: Invalid input, please use the following format: ./chatClient [server] [port]\n");
		exit(1);
	}

	printf("Enter a username that is 10 characters or less: ");
	scanf("%s", userName);

	while (strlen(userName) > 10) {
		printf("Error: Username too long. Please enter a username that is 10 characters or less: ");
		scanf("%s", userName);
	}


	struct addrinfo* res = buildAddress(argv[1], argv[2]);

	//create socket, then connect it
	sockfd = buildSocket(res);
	connectSocket(sockfd, res);
	
	//send client username
	send(sockfd, userName, strlen(userName), 0);

	//get server username
	recv(sockfd, serverName, 10, 0);

	//call chat function to begin messaging
	chat(sockfd, userName, serverName);

	freeaddrinfo(res);
}