#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <sys/select.h>

#include "llist.c"

#define PORT "7462"
#define X_MAX 3
#define Y_MAX 3
#define Z_MAX 3

int kocka[X_MAX][Y_MAX][Z_MAX];
HeadNode* head;
char utolso_uzenetek[5][512];

int maxfds(fd_set fds) {
  int i, max = 0;
  for (i = 0; i < 1024; ++i) { if (FD_ISSET(i, &fds) && i > max) { max = i; } }
  return max+1;
}

int van_szabad_hely() {
  int ret = 0, x, y, z;
  for (x = 0; x < X_MAX; x += X_MAX-1)
    for (y = 0; y < Y_MAX; y += Y_MAX-1)
      for (z = 0; z < Z_MAX; z += Z_MAX-1)
        ret |= (kocka[x][y][z] == 0 ? 1 : 0);
  return ret;
}

koordinata elso_szabad_hely() {
  koordinata k = {0, 0, 0};
  int x, y, z;

  for (x = 0; x < X_MAX; x += X_MAX-1)
    for (y = 0; y < Y_MAX; y += Y_MAX-1)
      for (z = 0; z < Z_MAX; z += Z_MAX-1)
        if (kocka[x][y][z] == 0) {
          k.x = x;
          k.y = y;
          k.z = z;
          return k;
        }

  fprintf(stderr, "nincs is szabad hely");
  return k;
}

void atmozgat(int kit, koordinata honnan, koordinata hova) {
  char radleptek[] = "RADLEPTEK\n";
  Node *n;
  if (kocka[hova.x][hova.y][hova.z] != 0) {
    send(kocka[hova.x][hova.y][hova.z], radleptek, strlen(radleptek), 0);
  }

  kocka[hova.x][hova.y][hova.z] = kit;
  kocka[honnan.x][honnan.y][honnan.z] = 0;

  for (n = head->first; n; n = n->next) {
    if (n->koord.x == honnan.x && n->koord.y == honnan.y && n->koord.z == honnan.z) {
      kocka[honnan.x][honnan.y][honnan.z] = n->fd;
    }
  }

}

void kliens_uzenet_feldolgozas(int socket, char* buf) {
  Node* n = list_get_node(head, socket);
  koordinata honnan, hova;
  int x;

  if (n == 0) { fprintf(stderr, "list_get hiba"); exit(1); }
  honnan = n->koord;

  for (x = 3; x >= 0; --x) {
    strcpy(utolso_uzenetek[x+1], utolso_uzenetek[x]);
  }
  sprintf(utolso_uzenetek[0], "%d %s", socket, buf);

  if (strncmp(buf, "HOLVAGYOK", strlen("HOLVAGYOK")) == 0) {
    sprintf(buf, "%d, %d, %d\n", n->koord.x, n->koord.y, n->koord.z);
    send(socket, buf, strlen(buf), 0);
    return;
  }

  if (strncmp(buf, "X+", strlen("X+")) == 0) {
    if (n->koord.x < X_MAX-1) { n->koord.x += 1; hova = n->koord; atmozgat(socket, honnan, hova); }
    else { sprintf(buf, "NEM LEHET\n"); send(socket, buf, strlen(buf), 0); }
    return;
  }
  if (strncmp(buf, "Y+", strlen("Y+")) == 0) {
    if (n->koord.y < Y_MAX-1) { n->koord.y += 1; hova = n->koord; atmozgat(socket, honnan, hova); }
    else { sprintf(buf, "NEM LEHET\n"); send(socket, buf, strlen(buf), 0); }
    return;
  }
  if (strncmp(buf, "Z+", strlen("Z+")) == 0) {
    if (n->koord.z < Z_MAX-1) { n->koord.z += 1; hova = n->koord; atmozgat(socket, honnan, hova); }
    else { sprintf(buf, "NEM LEHET\n"); send(socket, buf, strlen(buf), 0); }
    return;
  }

  if (strncmp(buf, "X-", strlen("X-")) == 0) {
    if (n->koord.x > 0) { n->koord.x -= 1; hova = n->koord; atmozgat(socket, honnan, hova); }
    else { sprintf(buf, "NEM LEHET\n"); send(socket, buf, strlen(buf), 0); }
    return;
  }
  if (strncmp(buf, "Y-", strlen("Y-")) == 0) {
    if (n->koord.y > 0) { n->koord.y -= 1; hova = n->koord; atmozgat(socket, honnan, hova); }
    else { sprintf(buf, "NEM LEHET\n"); send(socket, buf, strlen(buf), 0); }
    return;
  }
  if (strncmp(buf, "Z-", strlen("Z-")) == 0) {
    if (n->koord.z > 0) { n->koord.z -= 1; hova = n->koord; atmozgat(socket, honnan, hova); }
    else { sprintf(buf, "NEM LEHET\n"); send(socket, buf, strlen(buf), 0); }
    return;
  }

  if (strncmp(buf, "?", strlen("?")) == 0) {
    for (x = 4; x >= 0; --x) {
      send(socket, utolso_uzenetek[x], strlen(utolso_uzenetek[x]), 0);
      printf("%s", utolso_uzenetek[x]);
    }
    return;
  }

  sprintf(buf, "NEM ERTEM\n");
  send(socket, buf, strlen(buf), 0);
}

void kocka_kirajzol() {
  int x, y, z;

  printf("------------------------------------------------------\n");
  for (x = 0; x < X_MAX; ++x) {
    for (y = 0; y < Y_MAX; ++y) {
      for (z = 0; z < Z_MAX; ++z) {
        if (kocka[x][y][z]) printf("%2d", kocka[x][y][z]);
        else printf(" .");
      }
      printf("\n");
    }
    printf("----------\n");
  }
  printf("------------------------------------------------------\n");
}

int main(void) {
  struct addrinfo hints, *res;
  int listener, yes=1, i, newfd;
  fd_set master, read_fds;
  struct timeval timeout = {10, 0};
  struct sockaddr clientaddr; socklen_t clientaddrlen;
  ssize_t bytes;
  char buf[512];
  Node* node;
  int quit = 0;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  getaddrinfo(NULL, PORT, &hints, &res);

  listener = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
  if (bind(listener, res->ai_addr, res->ai_addrlen) < 0) { perror("bind"); exit(1); }
  if (listen(listener, 0) < 0) { perror("listen"); exit(1); }

  FD_ZERO(&master);
  FD_SET(0, &master);
  FD_SET(listener, &master);

  memset(kocka, 0, sizeof kocka);
  head = list_new();
  memset(utolso_uzenetek, 0, sizeof utolso_uzenetek);

  printf("futok\n");
  while (quit == 0) {
    read_fds = master;
    select(maxfds(read_fds), &read_fds, NULL, NULL, &timeout);

    for (i = 0; i < maxfds(master); ++i) {
      if (FD_ISSET(i, &read_fds)) {
        if (i == 0) {
          read(0, buf, sizeof buf -1);
          if (strncmp(buf, "quit", strlen("quit")) == 0) {
            quit = 1;
          }
        }
        else
        if (i == listener) { /*kliens jott*/
          clientaddrlen = sizeof clientaddr;
          newfd = accept(listener, &clientaddr, &clientaddrlen);
          printf("kliens jott: #%d\n", newfd);

          if (van_szabad_hely()) {
            printf("\tes van szabad hely\n");
            FD_SET(newfd, &master);
            node = calloc(1, sizeof(Node));
            node->fd = newfd;
            node->koord = elso_szabad_hely();
            kocka[node->koord.x][node->koord.y][node->koord.z] = newfd;
            list_add(head, node);
            kocka_kirajzol();
          } else {
            printf("\tde nincs neki szabad hely\n");
            close(newfd);
          }
        } /*i == listener*/
        else {
          bytes = read(i, buf, sizeof buf -1);
          if (bytes == 0) { /*lecsatlakozott*/
            close(i);
            printf("#%d lecsatlakozott\n", i);
            FD_CLR(i, &master);
            list_delete_socket(head, i);
          } else { /*rendes uzenet kuldott*/
            buf[bytes] = 0;
            printf("#%d kuldte: %s", i, buf);

            kliens_uzenet_feldolgozas(i, buf);
            kocka_kirajzol();
          }
        } /*i == listener else*/

      } /*if fd_isset*/
    }

    timeout.tv_sec = 10;
  }

  list_delete(head);
  close(listener);
  freeaddrinfo(res);

  return 0;
}
