#ifndef LLIST_C
#define LLIST_C

#include <stdio.h>
#include <stdlib.h>

typedef struct Node_tag {
  int fd;
  int allapot;
  char ipport[256];
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

int list_socket_allapot(HeadNode *head, int socket) {
    Node *node;
    for (node = head->first; node; node = node->next) {
        if (node->fd == socket) return node->allapot;
    }
    return 0;
}

char* list_socket_ipport(HeadNode *head, int socket) {
    Node *node;
    for (node = head->first; node; node = node->next) {
        if (node->fd == socket) return node->ipport;
    }
    return 0;
}


void list_socket_allapot_set(HeadNode *head, int socket, int allapot) {
    Node *node;
    for (node = head->first; node; node = node->next) {
        if (node->fd == socket) node->allapot = allapot;
    }
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
