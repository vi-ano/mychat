#include"udpserver.h"
#include <stdio.h>
#include <stdlib.h>



struct ClientElement*  insert_in_queue(struct ClientElement *root, struct sockaddr_in data) //add element to the back of the queue
{
  struct ClientElement *temp, *new_el;
  temp = root;

  new_el = (struct ClientElement*)malloc(sizeof(struct ClientElement));
  new_el->client = data;
  new_el->ttl = 0;
  pthread_mutex_init(&new_el->lock, NULL);

  if (root == NULL)   // created the root
    new_el->next = NULL; //initialize pointer to next element with NULL

  else
    new_el->next = root; // created element become last

  return(new_el);


}

struct ClientElement*  delete_from_queue (struct ClientElement* root, struct ClientElement* fordel)
{
  struct ClientElement *temp;
  temp = root;


  if(root == fordel)          // if we delete root (the first element)
  {
      temp = root->next;
      free(fordel);
      return temp;
  }

  while (temp->next != fordel) // find element that stands before element for deletion
    temp = temp->next;

  temp->next = fordel->next;  // move pointer to the element that stands after element for deletion
  free(fordel);

  return root;

}


int if_client_in(struct ClientElement* root, struct sockaddr_in data) //check if client with this ip in list
{
  if (root == NULL)
    return 0;

  struct ClientElement *p;
  p = root;

  while(p)
  {
   if ((p->client.sin_port == data.sin_port) && (p->client.sin_addr.s_addr == data.sin_addr.s_addr))
      return 1;

   p = p->next;
  }

  return 0;

}
