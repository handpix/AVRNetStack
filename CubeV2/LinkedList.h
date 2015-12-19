/*
* LinkedList.h
*
* Created: 12/16/2015 2:48:28 AM
*  Author: mhendricks
*/


#ifndef LINKEDLIST_H_
#define LINKEDLIST_H_

typedef int (*findCallback)(void *data, void *key);

typedef struct Node
{
	void *data;
	struct Node *next;
} node;

node *insert(node *pointer, void *data);
node *find(node *list, void *key, findCallback findFunc);
node *delete(node *pointer, void *key, findCallback findFunc);
node *deleteNode(node *list, node *node);


#endif /* LINKEDLIST_H_ */