#include <stdio.h>
#include <stdlib.h>

typedef struct Node_tag {
  int value;
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

void list_add(HeadNode* head, int ujelem) {
  Node* node = calloc(sizeof(Node), 1);
  node->value = ujelem;
  if (!head->first) {
    head->first = node;
  }
  if (head->last) {
    head->last->next = node;
  }
  head->last = node;
  ++head->count;
}

void list_delete_nth(HeadNode *head, int index) {
  Node *node = head->first, *prev = 0;
  while (index) { prev = node; node = node->next; --index; }

  if (prev) { prev->next = node->next; }
  --head->count;
  
  if (head->first == node) { head->first = node->next; }
  if (head->last  == node) { head->last = prev; }
  
  free(node);
}

void list_print(HeadNode* head) {
  printf("osszesen %i elem\n", head->count);
  Node* n = head->first;
  while (n) {
    printf("%i\n", n->value);
    n = n->next;
  }
  printf("===================================\n");
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

int main() {
  int menu = 0, ujelem, index;
  HeadNode* head = list_new();
  
  do {
    printf("1. uj listaelem\n2. n. elem torlese\n3. kilistazas\n4. kilepes\n\nvalasztas: ");
    scanf("%i", &menu);
    
    switch (menu) {
      case 1:
        printf("uj elem: ");
        scanf("%i", &ujelem);
        list_add(head, ujelem);
        break;
      case 2:
        if (!head->count) { printf("nincs mit torolni\n"); break; }
        printf("torlendo index [0-%i]:", head->count-1);
        scanf("%i", &index);
        if (0 <= index && index < head->count) {
          list_delete_nth(head, index);
        } else {
          printf("hibas index\n");
        }
        break;
      case 3:
        list_print(head);
        break;
    }
  } while (menu != 4);
  
  list_delete(head);
  
  return 0;
}

