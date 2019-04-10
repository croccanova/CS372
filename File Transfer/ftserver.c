/*
# Christian Roccanova
# CS372
# Fall 2018
# Project2 - server side portion of a file transfer system
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <dirent.h>
#include <arpa/inet.h>


// Creates address info
// code as seen in Beej's guide section 5.1 https://beej.us/guide/bgnet/html/single/bgnet.html
struct addrinfo * buildAddress(char* port) {
	int status;
	struct addrinfo hints;
	struct addrinfo * res;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((status = getaddrinfo(NULL, port, &hints, &res)) != 0) {
		fprintf(stderr,	"getaddrinfo error : %s\n", gai_strerror(status));
		exit(1);
	}

	return res;
}

// based on Beej's guide section 5.2 https://beej.us/guide/bgnet/html/single/bgnet.html#socket
struct addrinfo * buildAddressIP(char* ipAddress, char* port) {
	int status;
	struct addrinfo hints;
	struct addrinfo * res;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if ((status = getaddrinfo(ipAddress, port, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(1);
	}

	return res;
}

//put address information into the socket
// based on Beej's guide section 5.2 https://beej.us/guide/bgnet/html/single/bgnet.html#socket
int buildSocket(struct addrinfo * res) {
	int sockfd;
	if ((sockfd = socket((struct addrinfo *)(res)->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
		fprintf(stderr, "Error: Socket creation failed.\n");
		exit(1);
	}
	return sockfd;
}

// bind socket to a port
// based on Beej's guide section 5.3 https://beej.us/guide/bgnet/html/single/bgnet.html#bind
void bindSocket(int sockfd, struct addrinfo * res) {
	if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
		close(sockfd);
		fprintf(stderr, "Error: Binding failed.\n");
		exit(1);
	}
}

// connect to client
// based on Beej's guide section 5.4 https://beej.us/guide/bgnet/html/single/bgnet.html#connect
void connectSocket(int sockfd, struct addrinfo * res) {
	int status;
	if ((status = connect(sockfd, res->ai_addr, res->ai_addrlen)) == -1) {
		fprintf(stderr, "Error: Connection Failed.\n");
		exit(1);
	}
}

// based on Beej's guide section 5.5 https://beej.us/guide/bgnet/html/single/bgnet.html#listen
void listenSocket(int sockfd) {
	if (listen(sockfd, 5) == -1) {
		close(sockfd);
		fprintf(stderr, "Error: Listen failed.\n");
		exit(1);
	}
}

//create storage array for files
char ** buildArray(int size) {
	char ** array = malloc(size * sizeof(char *));
	int i;
	for (i = 0; i < size; i++) {
		array[i] = malloc(100 * sizeof(char));
		memset(array[i], 0, sizeof(array[i]));
	}
	return array;
}

// clears storage array
void freeArray(char ** array, int size) {
	int i;
	for (i = 0; i < size; i++) {
		free(array[i]);
	}
	free(array);
}

// reads files in the directory
// based on the following example https://stackoverflow.com/questions/11736060/how-to-read-all-files-in-a-folder-using-c
int getFiles(char** files) {
	DIR* d;
	struct dirent * dir;
	d = opendir(".");
	int i = 0;
	if (d) {
		// loop until there is nothing left to read
		while ((dir = readdir(d)) != NULL) {
			if (dir->d_type == DT_REG) {
				strcpy(files[i], dir->d_name);
				i++;
			}
		}
		closedir(d);
	}
	return i;
}

// searches through files to find a specific one
int findFile(char ** files, int fileCount, char * fileName) {
	// 0 is false, assume false until file is found
	int fileFound = 0;
	int i;

	// loop through files, checking for a matching name
	for (i = 0; i < fileCount; i++) {
		if (strcmp(files[i], fileName) == 0) {
			fileFound = 1;
		}
	}
	return fileFound;
}

// sends file to client
void sendData(char* ipAddress, char * port, char * fileName) {
	sleep(3); // sleeps added as calls in quick succession seemed to be causing connection failures
	struct addrinfo * res = buildAddressIP(ipAddress, port);
	int dataSocket = buildSocket(res);

	connectSocket(dataSocket, res);

	// file buffer
	char buffer[2000];
	memset(buffer, 0, sizeof(buffer));

	int fd = open(fileName, O_RDONLY);
	
	// read file
	// the following code is based on the example found here: ftp://gaia.cs.umass.edu/pub/kurose/ftpserver.c
	while (1) {
		int byteCount = read(fd, buffer, sizeof(buffer) - 1);
		if (byteCount == 0)
			break;

		// output error if file cannot be read
		if (byteCount < 0) {
			fprintf(stderr, "Error: Could not read file.\n");
			return;
		}

		void* package = buffer;

		// keep sending until there is nothing left
		while (byteCount > 0) {
			// send data in pieces
			int bytesWritten = send(dataSocket, package, sizeof(buffer), 0);
			if (bytesWritten < 0) {
				fprintf(stderr, "Error: Could not write file.\n");
				return;
			}
			// update remaining count of data
			byteCount -= bytesWritten;
			package += bytesWritten;
		}
		
		memset(buffer, 0, sizeof(buffer));
	}

	memset(buffer, 0, sizeof(buffer));
	strcpy(buffer, "done");

	// send done signal to client
	send(dataSocket, buffer, sizeof(buffer), 0);

	close(dataSocket);
	freeaddrinfo(res);
}


// finds requested file, preps it and then sends it to the client
void transferFile(char * ipAddress, char* port, char * fileName, int new_fd) {
	sleep(3); // sleeps added as calls in quick succession seemed to be causing connection failures
	char** files = buildArray(500);
	int fileCount = getFiles(files);
	char newName[100];
	
	//check if file exists
	int requested = findFile(files, fileCount, fileName);
	if (requested) {

		printf("File found, sending %s to client.\n", fileName);
		char * fileSearch = "found";
		send(new_fd, fileSearch, strlen(fileSearch), 0);

		// package and send file data
		sendData(ipAddress, port, fileName);		
	}
	// send error message if file was not found
	else {
		printf("File not found.\n");
		char * notFound = "File not found";
		send(new_fd, notFound, 100, 0);
	}

	//clear data
	freeArray(files, 500);
}

// obtains list of files in the server's directory and sends the list to the client
void listFiles(char * ipAddress, char* port) {
	sleep(3); // sleeps added as calls in quick succession seemed to be causing connection failures
	int i;
	printf("File list requested \n");
	printf("Sending file list to %s \n", ipAddress);

	char** files = buildArray(500);

	int fileCount = getFiles(files);
	 
	// create socket and connection for sending
	struct addrinfo * res = buildAddressIP(ipAddress, port);
	int dataSocket = buildSocket(res);
	connectSocket(dataSocket, res);

	// send each file one by one
	for (i = 0; i < fileCount; i++) {
		send(dataSocket, files[i], 100, 0);
	}

	char* completed = "done";
	send(dataSocket, completed, strlen(completed), 0);
	printf("Listing completed.\n");

	close(dataSocket);
	freeaddrinfo(res);

	// clear data
	freeArray(files, 500);
}

// based on Beej's guide section 5.6 https://beej.us/guide/bgnet/html/single/bgnet.html#accept
void acceptConnection(int new_fd) {
	char * good = "ok";
	char * bad = "bad";
	char port[100];
	char command[500];
	char ipAddress[100];

	// receive port, then send confirmation
	memset(port, 0, sizeof(port));
	recv(new_fd, port, sizeof(port) - 1, 0);
	send(new_fd, good, strlen(good), 0);

	// receive command, then send confirmation
	memset(command, 0, sizeof(command));
	recv(new_fd, command, sizeof(command) - 1, 0);
	send(new_fd, good, strlen(good), 0);

	//receive ip address
	memset(ipAddress, 0, sizeof(ipAddress));
	recv(new_fd, ipAddress, sizeof(ipAddress) - 1, 0);

	printf("Connection established with %s\n", ipAddress);

	// find and transfer a specified file if the -g command was received
	if (strcmp(command, "g") == 0) {
		send(new_fd, good, strlen(good), 0);
		char fileName[100];
		memset(fileName, 0, sizeof(fileName));
		recv(new_fd, fileName, sizeof(fileName) - 1, 0);

		printf("Client requesting file '%s'\n", fileName);

		transferFile(ipAddress, port, fileName, new_fd);		

		printf("File transfer complete.\n");
	}
	// obtain and send list of files if -l command was received
	else if (strcmp(command, "l") == 0) {
		send(new_fd, good, strlen(good), 0);

		listFiles(ipAddress, port);
	}
	// if neither of the above is correct, the command is invalid
	else {
		send(new_fd, bad, strlen(bad), 0);
		fprintf(stderr, "Received invalid command from client.\n");
	}

	printf("Waiting for further connections...\n");
}

// based on Beej's guide section 6.1 https://beej.us/guide/bgnet/html/single/bgnet.html#simpleserver
void waitForConnection(int sockfd) {
	struct sockaddr_storage their_addr;
	socklen_t addr_size;
	int new_fd;

	while (1) {
		addr_size = sizeof(their_addr);
		new_fd = accept(sockfd, (struct addrinfo *)&their_addr, &addr_size);

		if (new_fd == -1) {
			continue;
		}

		acceptConnection(new_fd);
		close(new_fd);
	}
}


int main(int argc, char *argv[]) {
	
	// print error if port argument was not included
	if (argc != 2) {
		fprintf(stderr, "Invalid number of arguments.\n");
		exit(1);
	}

	// create and bind sockets, then listen and wait for connections
	struct addrinfo* res = buildAddress(argv[1]);
	int sockfd = buildSocket(res);
	bindSocket(sockfd, res);
	listenSocket(sockfd);
	printf("Server listening on port %s\n", argv[1]);
	waitForConnection(sockfd);	
	freeaddrinfo(res);
}