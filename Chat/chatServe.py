#!/bin/python

# Christian Roccanova
# CS 372
# Project 1
# chatserve - implementation of the server portion of a chat  program

import sys
from socket import *

#function to handle chatting
def chat(con, userName, clientName):
    message = ""

    #loop until a user chooses to quit
    while 1:

        #receive message from client
        receivedMsg = con.recv(501)[0:-1]
        
        #client elects to end connection
        if receivedMsg == "\\quit":
            print "Client terminated connection."
            print "Waiting for a client..."
            break
        
        #print message/prompt
        print clientName + ">", receivedMsg
        
        sentMsg = ""

        #cap message at 500 characters
        while len(sentMsg) > 500 or len(sentMsg) == 0:
            sentMsg = raw_input(userName + "> ")
        
        #quits when user inputs "\quit"
        if sentMsg == "\quit":
            print "Connection closed."
            print "Waiting for a client..."
            break
        con.send(sentMsg)

#main
if __name__ == "__main__":

    #check if the user input the correct number of arguments. If not, print error message
    if len(sys.argv) != 2:
        print "Invalid input, please use the following format: python chatServer.py [port]"
        exit(1)
        
    #the following socket code is based on the example at the following link: https://docs.python.org/2/howto/sockets.html
    portNum = int(sys.argv[1])
    mySocket = socket(AF_INET, SOCK_STREAM)    
    mySocket.bind(('', portNum))   
    mySocket.listen(1)
    
    userName = ""
    #Get username
    userName = raw_input("Enter a username that is 10 characters or less: ")
    while len(userName) > 10:
        userName = raw_input("Error: Username too long. Please enter a username that is 10 characters or less: ")
    print "Waiting for a client..."
        
    while 1:
        (connection, address) = mySocket.accept()                         
        print "Connected to client at address: ", address

        #get client username
        clientName = connection.recv(10)
        
        #send server username
        connection.send(userName)

        #begin chatting
        chat(connection, userName, clientName)
        
        connection.close()