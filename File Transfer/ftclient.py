#!/bin/python
# Christian Roccanova
# CS372
# Fall 2018
# Project2 - client side portion of a file transfer system

import sys
from socket import *

# based on example seen here: https://docs.python.org/2/howto/sockets.html under section "creating a socket"
def createSocket():  
    
    # -l command will take 4 arguments
    if sys.argv[3] == "-l":
        argCount = 4

    # -g command will take five arguments (one extra for the file name)
    elif sys.argv[3] == "-g":
        argCount = 5

    serverport = int(sys.argv[argCount])
    serversocket = socket(AF_INET, SOCK_STREAM)
    serversocket.bind(('', serverport))
    serversocket.listen(1)
    dataSocket, address = serversocket.accept()
    return dataSocket

#connects to server
# based on example seen here: https://docs.python.org/2/howto/sockets.html under section "using a socket"
def connectServer():
    #will run only on the engineering servers
    hostName = sys.argv[1]+".engr.oregonstate.edu"
    port = int(sys.argv[2])

    clientSocket = socket(AF_INET,SOCK_STREAM)    
    clientSocket.connect((hostName, port))
    
    return clientSocket
 
#returns socket IP
def getIP():
    s = socket(AF_INET, SOCK_DGRAM)
    s.connect(("8.8.8.8", 80))
    return s.getsockname()[0]

#checks if the requested file already exists
#based on examples found here: https://www.pythoncentral.io/check-file-exists-in-directory-python/
def fileExists(fileName):
    #attempts to open file, returns false if it cannot
    try:
        f = open(fileName, "r")
        f.close()
    except IOError as e:
        return "false"
    return "true"

# retrieves a file
def getFile(dataSocket):

    port = sys.argv[5]
    fileName = sys.argv[4]

    #first check if file already exists
    exists = fileExists(fileName)

    #if file exists check if user wants to overwrite, if not, exit
    if exists == "true":
        print "File '", fileName, "'already exists, would you like to overwrite it? (Y/N)"
        answer = raw_input()
        if answer != "Y" and answer != "y":
            dataSocket.close()
            exit()

    print "Retrieving file '", fileName, "'."
    
    # send port
    clientSocket.send(port)    
    clientSocket.recv(1024)
    
    # send command
    clientSocket.send("g")    
    clientSocket.recv(1024)
    
    # send IP
    clientSocket.send(getIP())
    response = clientSocket.recv(1024)
    
    # if command was invalid, prints an error and exits
    if response == "bad":
        print "An invalid command was sent to the server."
        exit(1)
    

    clientSocket.send(fileName)
    response = clientSocket.recv(1024)

    # if file is found on server, server will relay the message "found" to the client
    if response != "found":
        print "File not found."
        return
    
    dataSocket = createSocket()

    f = open(fileName, "w")    
    buffer = dataSocket.recv(1000)
    
    # server sends the message "done" once transfer is complete
    while "done" not in buffer:
        f.write(buffer)
        buffer = dataSocket.recv(1000)

    print "File transfer complete."

    #close socket
    dataSocket.close()
        
# retrieves a list of files on the server
def getList(dataSocket):

    port = sys.argv[4]
    
    print "Retrieving list."

    # send port
    clientSocket.send(port)    
    clientSocket.recv(1024)

    #send command
    clientSocket.send("l")    
    clientSocket.recv(1024)
    
    #send IP
    clientSocket.send(getIP())
    response = clientSocket.recv(1024)
    
    # if command was invalid, prints an error and exits
    if response == "bad":
        print "An invalid command was sent to the server."
        exit(1) 
    
    dataSocket = createSocket()
    filename = dataSocket.recv(100)     #Similar here. Loop until we hit done
    
    while filename != "done":        
        print filename
        filename = dataSocket.recv(100)

    #close socket
    dataSocket.close()


# function to validate user input
def validateArgs():

    # validate number of arguments
    if len(sys.argv) < 5 or len(sys.argv) > 6:
        print "Invalid number of arguments."
        exit(1)

    # validate server name    
    elif (sys.argv[1] != "flip1" and sys.argv[1] != "flip2" and sys.argv[1] != "flip3"):   
        print "Invalid server name."
        exit(1)

    # validate server port     
    elif (int(sys.argv[2]) < 1024 or int(sys.argv[2]) > 65535):                
        print "Invalid port number, please use the range 1024-65535"
        exit(1)
    
    #validate command
    elif (sys.argv[3] != "-l" and sys.argv[3] != "-g"):
        print "Invalid command, please enter either -l or -g"
        exit(1)
    
    #validate port for -l (4th argument)
    elif (sys.argv[3] == "-l" and (int(sys.argv[4]) < 1024 or int(sys.argv[4]) > 65535)):   
        print "Invalid port number, please use the range 1024-65535"
        exit(1)

    # validate port for -g (5th argument)    
    elif (sys.argv[3] == "-g" and (int(sys.argv[5]) < 1024 or int(sys.argv[5]) > 65535)):        
        print "Invalid port number, please use the range 1024-65535"
        exit(1)

if __name__ == "__main__":    
     
    # test whether the user input is valid
    validateArgs() 

    # create a new socket
    clientSocket = connectServer()
    
    # attempt to retrieve the file or list per user input
    if (sys.argv[3] == "-l"):
        getList(clientSocket)
    elif (sys.argv[3] == "-g"):
        getFile(clientSocket)
