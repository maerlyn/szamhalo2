#ifndef LLIST_C
#define LLIST_C

#include <stdio.h>
#include <stdlib.h>

typedef struct _koord { int x, y, z; } koordinata;

typedef struct Node_tag {
  int fd;
  koordinata koord;
  struct Node_tag* next;
} Node;

typedef struct {
  Node* first;
  Node* last;
  int count;
} HeadNode;

HeadNode* list_new() {
  HeadNode* ret = calloc(sizeof(HeadNode), 1);
  return ret;
}

void list_add(HeadNode* head, Node* node) {
  if (!head->first) {
    head->first = node;
  }
  if (head->last) {
    head->last->next = node;
  }
  head->last = node;
  ++head->count;
}

void list_delete_socket(HeadNode *head, int socket) {
  Node *node = head->first, *prev = 0;
  while (node->fd != socket) { prev = node; node = node->next; }

  if (prev) { prev->next = node->next; }
  --head->count;

  if (head->first == node) { head->first = node->next; }
  if (head->last  == node) { head->last = prev; }

  free(node);
}

Node* list_get_node(HeadNode *head, int socket) {
  Node* n;
  for (n = head->first; n; n = n->next)
    if (n->fd == socket)
      return n;

  return 0;
}

void list_delete(HeadNode* head) {
  Node *n = head->first, *next;
  while (n) {
    next = n->next;
    free(n);
    n = next;
  }
  free(head);
}

#endif
