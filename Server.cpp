//
//  Server.cpp
//  Playground
//
//  Created by David Kopala on 7/23/17.
//  Copyright © 2017 David Kopala. All rights reserved.
//

#include "Server.h"

#define BACKLOG 10

//Server::smsg *Server::messages;
List<Server::TCPMessage> *Server::messages;
List<Server::ClientInfo> *Server::clients;
//Server::socketList *Server::clients;

int Server::p;

Server::Server(int port)
{
    struct sockaddr_in myaddr;
    int sockfd;
    
    p = port;
    
    //Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Could not create socket");
    }
    
    //Bind to socket
    memset((char *)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(port);
    
    if (bind(sockfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
        perror("Could not bind to socket");
        return;
    }
    
    //Set SO_REUSEADDR option to prevent program from being relaunched quickly
    int optval = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        perror("could not set socket options");
    }
    
    clients = new List<ClientInfo>();
    messages = new List<TCPMessage>();
    
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }
    
    pthread_t listeningThread;
    join_newconn *test = new join_newconn;
    test->ptr = this;
    test->sockfd = sockfd;
    //int *new_sock = (int *)malloc(1);
    //*new_sock = sockfd;
    if (pthread_create(&listeningThread, NULL, Server::listening_handler, (void *)test) >= 0) {
        
    }
}

void *Server::listening_handler(void *context) {
    join_newconn *join = (join_newconn *)context;
    int newfd;
    sockaddr their_addr;//struct sockaddr_storage their_addr;
    socklen_t sin_size = sizeof(their_addr);
    int mysock = join->sockfd;//*(int*)sockfd;
    //socketList *lastSocket = clients;
    
    while (true) {
        newfd = accept(mysock, (struct sockaddr *)&their_addr, &sin_size);
        
        //newConnectionCallback();
        
        ClientInfo *info = new ClientInfo;
        info->fd = newfd;
        struct sockaddr_in *test = (struct sockaddr_in *) &their_addr;
        info->addr = inet_ntoa(test->sin_addr);
        info->port = p;
        
        join->ptr->newConnectionCallback(info);
        
        join_newmessage *data = new join_newmessage;
        data->data = info;
        data->ptr = join->ptr;
        clients->addCopy(*info);
        pthread_t snifferThread;
        if (pthread_create(&snifferThread, NULL, Server::connection_handler, (void *)data) < 0) {
            
        }
    }
    
    return 0;
}

void *Server::connection_handler(void *tcp_info)
{
    //Get the socket descriptor
    //int sock = *(int*)socket_desc;
    join_newmessage *data = (join_newmessage *)tcp_info;
    ClientInfo *info = data->data;//= (ClientInfo *) tcp_info;
    ssize_t read_size;
    char client_message[2000];
    
    while( (read_size = recv(info->fd , client_message , 2000 , 0)) > 0 )
    {
        //Send the message back to client
        TCPMessage *message = new TCPMessage;
        message->addr = info;
        //lastMsg->next = (smsg *)malloc(sizeof(smsg));
        char mes_cpy[500] = {'\0', };
        strcpy(mes_cpy, client_message);
        message->message = mes_cpy;
        messages->add(message);
        data->ptr->newMessageCallback(message);
        bzero(client_message, 2000);
    }
    
    if(read_size == 0)
    {
        //puts("Client disconnected");
        data->ptr->clientDisconnected(info);
        fflush(stdout);
    }
    else if(read_size == -1)
    {
        perror("recv failed");
    }
    
    //free(socket_desc);
    
    return 0;
}

void Server::writeToAll(char *message)
{
    //socketList *client = clients;
    for (int i = 0; i < clients->count(); i++) {
        //write(clients->get(i)->fd, message, strlen(message));
        writeToClient(clients->get(i), message);
    }
}

void Server::writeToClient(Server::ClientInfo *client, char *msg)
{
    write(client->fd, msg, strlen(msg));
}

void Server::writeToClients(List<Server::ClientInfo> *w_clients, char *msg)
{
    for (int i = 0; i < w_clients->count(); i++) {
        writeToClient(w_clients->get(i), msg);
    }
}

void Server::sigchld_handler(int s)
{
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    
    errno = saved_errno;
}

void Server::newMessageCallback(Server::TCPMessage *message) {
    
}

void Server::newConnectionCallback(ClientInfo *info) {
    
}

void Server::clientDisconnected(Server::ClientInfo *info) {
    puts("Default disconnect callback");
}
