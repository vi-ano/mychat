/*For chat*/
#define DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <pthread.h>
#include <ncurses.h>
#include "udpserverdebug.h"

#define MAX_MESSAGE_SIZE 1472
#define PORT 7879

WINDOW *w_chat, *w_input;

#define PRINTF_IN_CHAT(message, args...) do {wprintw(w_chat, " " message, ##args); refresh_window(w_chat);} while(0);

/* refresh window with border */
void refresh_window(WINDOW *win) {
    if (!win)
        return;

    box(win, 0, 0);
    wrefresh(win);
}

/* create window with border */
WINDOW * create_window(int height, int width, int starty, int startx) {
    WINDOW *win;

    win = newwin(height, width, starty, startx);

    refresh_window(win);

    return win;
}



struct server_msg
{
    char cmd;
    int ip;
    char msg[MAX_MESSAGE_SIZE];
}__attribute__((packed));


struct sockaddr_in serveraddress;

void usage() {
  printf("Check the call: ./udpclient [IP] [PORT]\n");
  exit(1);

}

int strtoint (char* a)
{
  int i = 0;
  int ip[4];
  int q;
  int test;

  while (a[i] != '\0')
  {
    if (a[i] == '.')
      a[i] = '\0';

      i++;
  }

  for(i = 0; i<4; i++)
  {
    ip[i] = atoi(a);

    q=0;
    while (a[q] != '\0')
      q++;

    a = a + q + 1;

  }

  for(i = 0; i<4; i++)
    printf("%i\n", ip[i]);

   //test = ip[3] + ip[2]*256 + ip[1]*256*256 + ip[0]*256*256*256;
   test = (ip[3]) + (ip[2]<<8) + (ip[1]<<16) + (ip[0]<<24);


   return test;
}


void *function_for_sending(void *ptr) // function that works in thread that get and send messages from client to other 
{

  char get_message[MAX_MESSAGE_SIZE];
  char send_message[MAX_MESSAGE_SIZE];
  int q;

  while(1)
  {
    q=1;


    mvwprintw(w_input, 1, 1, " > ");

    memset(get_message,'\0', MAX_MESSAGE_SIZE);

    mvwscanw(w_input, 1, 4, "%1472[^\n]s", get_message);

    if(strlen(get_message) == 0)
         continue;

    send_message[0] = '2';           // first_symbol = '2' means general message


    strncpy(send_message+1,get_message, MAX_MESSAGE_SIZE-1);


    sendto(*(int *)ptr,send_message, strlen(send_message)+1, 0, (struct sockaddr*)&serveraddress, sizeof(serveraddress));


    if (strlen(get_message) != 0)
      PRINTF_IN_CHAT("message from me: %s\n", get_message);

    /* clear input window */
    wclear(w_input);

    /* refresh with border */
    refresh_window(w_input);
  }
}


int main (int argc, char* argv[])
{
  int clientsocket;

  int structlen = sizeof(serveraddress);
  char buf[1472] = "0";
  pthread_t thread;
  int recvlen;
  struct in_addr ip_addr;
  struct server_msg* ptomsg;
  int row,col;


  if (argc != 3)
      usage();


  clientsocket=socket(AF_INET, SOCK_DGRAM, 0);
  if (clientsocket < 0)
  {
    printf("socket() failed: %s\n", strerror(errno));
    return EXIT_FAILURE;
  }


  serveraddress.sin_family = AF_INET;
  serveraddress.sin_port = htons(atoi(argv[2]));
  serveraddress.sin_addr.s_addr = htonl(strtoint(argv[1]));


  sendto(clientsocket, buf, strlen(buf), 0, (struct sockaddr*)&serveraddress, structlen);

  initscr();

  /* disable cursor */
  curs_set(FALSE);

  /* get current row, col */
  getmaxyx(stdscr, row, col);

  /* create window for chat history */
  w_chat = create_window(/* height */ row-4, /* width */ col, /* starty */ 0, /*startx */ 0);

  PRINTF_IN_CHAT("\n"); // allowing to see the first symbol because the border hide it

  /* enable autoscroll in chat window */
  scrollok(w_chat,TRUE);

  /* create window for input */
  w_input = create_window(/* height */ 4, /* width */ col, /* starty */ row-4, /*startx */ 0);

  if(pthread_create(&thread, NULL, function_for_sending, &clientsocket))
  {
      printf("pthread_create() failed: %s\n", strerror(errno));
      return EXIT_FAILURE;

  }

  memset(buf,'\0', MAX_MESSAGE_SIZE);




  while(1)
  {

    recvlen = recvfrom(clientsocket, buf, sizeof(buf), 0, (struct sockaddr*)&serveraddress,&structlen);

    printf("recvlen %i", recvlen);

    ptomsg = (struct server_msg*)buf;

    if(ptomsg->cmd == '1') //  first_symbol = '1' means procedure message
    {
      //DPRINTF("got serv message\n");
      sendto(clientsocket, buf, 1, 0, (struct sockaddr*)&serveraddress, structlen);
    }

    else if(ptomsg->cmd == '2') // if get general messages from other users
    {
      ip_addr.s_addr = ptomsg->ip;

      PRINTF_IN_CHAT("from %s received: %s\n",inet_ntoa(ip_addr), ptomsg->msg);

      /* clear input window */
      //wclear(w_chat);

      /* refresh with border */
      refresh_window(w_chat);

      //printf("from %s received: %s\n",inet_ntoa(ip_addr), ptomsg->msg);

      memset(buf, '\0',recvlen);

     }
  }

  close(clientsocket);

  return 0;
}
