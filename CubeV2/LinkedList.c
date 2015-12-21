#include <avr/io.h>
#include <stdlib.h>
#include "LinkedList.h"

uint8_t depth(node *list)
{
	uint8_t ret = 0;
	while(list->next!=NULL)
	{
		ret ++;
		list = list -> next;
	}
	return ret;
}

node *insert(node *list, void *data)
{
	/* Iterate through the list until we encounter the last node.*/
	while(list->next!=NULL)
	{
		list = list -> next;
	}
	/* Allocate memory for the new node and put data in it.*/
	list->next = (node *)malloc(sizeof(node));
	list = list->next;
	list->data = data;
	list->next = NULL;
	return list->next;
}

node *find(node *list, void *key, findCallback findFunc)
{
	list=list->next; //First node is dummy node.
	/* Iterate through the entire linked list and search for the key. */
	while(list!=NULL)
	{
		if(findFunc(list->data, key)) //key is found.
		{
			return list;
		}
		list = list -> next;//Search in the next node.
	}
	/*Key is not found */
	return NULL;
}

node* deleteNode(node *list, node *delNode)
{
	node *pointer = list;
	/* Go to the node for which the node next to it has to be deleted */
	while(pointer->next != NULL && pointer->next!=delNode)
	{
		pointer = pointer -> next;
	}
	// We did not find the requested node, next points at NULL, so return (this is the end of list)
	if(pointer->next != delNode)
	{
		return NULL;
	}

	// pointer points to the node before delNode
	
	// Take this node out of the middle, If this node is the last node, next->next will be NULL making this our last node;
	free(pointer->next);	// free memory used by this node
	pointer->next = pointer->next->next;
	return pointer->next;
}