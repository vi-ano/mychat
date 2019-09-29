#ifndef _UDPSERVER_H_
#define _UDPSERVER_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

struct ClientElement
{
  struct sockaddr_in client;
  int ttl;
  struct ClientElement *next;
  pthread_mutex_t lock;
};



struct client_msg
{
    char cmd;
    char msg[];
};

struct ClientElement* insert_in_queue(struct ClientElement*, struct sockaddr_in);       //insert client in queue. in: 'root'+'data'
struct ClientElement* delete_from_queue(struct ClientElement*, struct ClientElement*);  //delete client from queue. in: 'root'+'client'
int if_client_in(struct ClientElement*, struct sockaddr_in);                            //check if client in queue. in: 'root'+'ddata

#endif
