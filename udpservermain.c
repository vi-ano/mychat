#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>

#include "udpserver.h"
#include "udpserverdebug.h"

#define DEBUG
#define PORT 7879

pthread_mutex_t lockforlist;

int current_num_clients = -1;
struct ClientElement* rootaddress = NULL;

void *function_for_service_messages(void *ptr) // main function for thread that sends service messages
{
    char send_message;

    while(1)
    {
        if((current_num_clients+1) > 0)
        {

            struct ClientElement *current;
            current = rootaddress;

            while (current != NULL )
            {
                pthread_mutex_lock(&current->lock);
                if (current->ttl == 3)
                {
                    printf("ready for delete\n");
                    printf("client %s:%d will be removed\n",inet_ntoa(current->client.sin_addr), ntohs(current->client.sin_port));

                    pthread_mutex_lock (&lockforlist);
                    rootaddress = delete_from_queue(rootaddress,current);
                    current_num_clients--;
                    pthread_mutex_unlock (&lockforlist);

                    printf("Number of clients now %i\n",current_num_clients+1);

                }



                if (current->ttl < 3)
                {
                    current->ttl += 1;

                    printf("Send to %s:%d with ttl %i\n",inet_ntoa(current->client.sin_addr), ntohs(current->client.sin_port), current->ttl);

                    send_message = '1';
                    sendto(*(int *)ptr, &send_message, 1, 0, (struct sockaddr*)&current->client, sizeof(current->client));
                }

                pthread_mutex_unlock(&current->lock);
                current = current->next;
            }

        }
        sleep(5);
    }
}


int main (int argc, char *argv[])
{
    int serversocket;
    struct sockaddr_in serveraddress, clientaddress;
    int structlen = sizeof(clientaddress);
    char message_buf[1472];
    char updated_message_buf[51];
    int recvlen;
    int q = 0;
    struct client_msg* ptomsg;


    int i;
    int port;
    pthread_t thread;


    serversocket=socket(AF_INET, SOCK_DGRAM, 0);
    if (serversocket < 0)
    {
        printf("socket() failed: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    if (argc==2)
        port = atoi(argv[1]);
    else
        port = PORT;

#ifdef DEBUG
    printf ("port=%d\n", port);
#endif

    serveraddress.sin_family = AF_INET;
    serveraddress.sin_port = htons(port);
    serveraddress.sin_addr.s_addr = INADDR_ANY;


    if (bind(serversocket, (struct sockaddr*) &serveraddress, sizeof(serveraddress)) < 0)
    {
        printf("bind() failed: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    pthread_mutex_init(&lockforlist,NULL);

    if(pthread_create(&thread, NULL, function_for_service_messages, &serversocket))
    {
        printf("pthread_create() failed: %s\n", strerror(errno));
        return EXIT_FAILURE;

    }

    memset(message_buf,'\0', strlen(message_buf));

    while(1)
    {

        recvlen = recvfrom(serversocket, message_buf, sizeof(message_buf), 0, (struct sockaddr*)&clientaddress,&structlen); //1) Got message. Put it in "clientaddress' structure

        ptomsg = (struct client_msg*)message_buf;


        printf("recvlen %i\n", recvlen);
        printf("from %s:%d received: %s\n\n", inet_ntoa(clientaddress.sin_addr), ntohs(clientaddress.sin_port), message_buf);

        if (recvlen == -1)
        {
            printf("recvlen() failed: %s\n", strerror(errno));
            return EXIT_FAILURE;
        }


        if ( ptomsg->cmd == '0') //2) check if it initialization message
        {
            pthread_mutex_lock (&lockforlist);

            current_num_clients++;
            printf("current num clients %i\n", (current_num_clients+1));
            rootaddress = insert_in_queue(rootaddress, clientaddress);

            pthread_mutex_unlock (&lockforlist);

            printf("Add %s with %d\n\n", inet_ntoa(rootaddress->client.sin_addr), ntohs(rootaddress->client.sin_port));

            continue;
        }


        if (if_client_in(rootaddress, clientaddress) == 1)     // if client ip is in base
        {
            if ( ptomsg->cmd == '1' /*&& ((current_num_clients+1) > 0)*/)
            {
                printf("get in serv\n");
                printf("%s:%d entered\n", inet_ntoa(clientaddress.sin_addr), ntohs(clientaddress.sin_port));

                struct ClientElement *current;
                current = rootaddress;

                do
                {
                    if (clientaddress.sin_port == current->client.sin_port)
                    {
                        pthread_mutex_lock(&current->lock);
                        current->ttl -= 1;
                        pthread_mutex_unlock(&current->lock);

                        printf("now ttl %s:%d have ttl = %i\n", inet_ntoa(current->client.sin_addr), ntohs(current->client.sin_port), current->ttl);
                    }
                    current = current->next;
                } while (current != NULL);
            }

         if ( ptomsg->cmd == '2')  //4) if its a) normal message and b) from known client
         {
            updated_message_buf[0] = ptomsg->cmd; //add ip address to whole message.updated_buffer = 'IP'+ 'Message'

            *(int *)&updated_message_buf[1] = clientaddress.sin_addr.s_addr;  //ip address was written

            q=5;
            while ((q-5)<(recvlen-1))
            {
             DPRINTF("what we write %x\n", ptomsg->msg [q-5]);
             updated_message_buf [q] = ptomsg->msg [q-5];
             q++;
            }

            updated_message_buf[q+5] = '\0';


            struct ClientElement *temp;
            temp = rootaddress;


            do{
                DPRINTF("we are in\n");
                if (clientaddress.sin_port != temp->client.sin_port)
                  sendto(serversocket, updated_message_buf, 4+recvlen, 0, (struct sockaddr*) &temp->client, structlen);
                temp = temp->next;
               } while (temp != NULL);

            }
        }

    }

    close (serversocket);

    return 0;
}
