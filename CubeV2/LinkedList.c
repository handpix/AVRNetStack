#include <stdlib.h>
#include "LinkedList.h"

void insert(node *list, void *data)
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
	return 0;
}

void delete(node *pointer, void *key, findCallback findFunc)
{
	/* Go to the node for which the node next to it has to be deleted */
	while(pointer->next!=NULL && findFunc(pointer->data, key))
	{
		pointer = pointer -> next;
	}
	if(pointer->next==NULL)
	{
		//Element %d is not present in the list\n"
		return;
	}
	/* Now pointer points to a node and the node next to it has to be removed */
	node *temp;
	temp = pointer -> next;
	/*temp points to the node which has to be removed*/
	pointer->next = temp->next;
	/*We removed the node which is next to the pointer (which is also temp) */
	/* Because we deleted the node, we no longer require the memory used for it deallocate it.	*/
	free(temp);
	return;
}

//int main()
//{
///* start always points to the first node of the linked list.
//temp is used to point to the last node of the linked list.*/
//node *start,*temp;
//start = (node *)malloc(sizeof(node));
//temp = start;
//temp -> next = NULL;
///* Here in this code, we take the first node as a dummy node.
//The first node does not contain data, but it used because to avoid handling special cases
//in insert and delete functions.
//*/
//printf("1. Insert\n");
//printf("2. Delete\n");
//printf("3. Print\n");
//printf("4. Find\n");
//while(1)
//{
//int query;
//scanf("%d",&query);
//if(query==1)
//{
//int data;
//scanf("%d",&data);
//insert(start,data);
//}
//else if(query==2)
//{
//int data;
//scanf("%d",&data);
//delete(start,data);
//}
//else if(query==3)
//{
//printf("The list is ");
//print(start->next);
//printf("\n");
//}
//else if(query==4)
//{
//int data;
//scanf("%d",&data);
//int status = find(start,data);
//if(status)
//{
//printf("Element Found\n");
//}
//else
//{
//printf("Element Not Found\n");
//
//}
//}
//}
//
//
//}
//
//
