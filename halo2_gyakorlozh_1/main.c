#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "llist.c"

/* feladat: Caesar-kodolas */

#define MAXCLIENTS 4
#define PORT "7462"

/*  globalis valtozok a tul sok parameterpasszolgatas elkerulese vegett */
unsigned int offset;
HeadNode* llist;
fd_set master_fds;
int client_count = 0, listener;
char legutobbiak[5][512];

int getmaxfd(fd_set *fd) {
    int i, max = -1;
    for (i = 3; i < 1024; ++i) {
        if (FD_ISSET(i, fd) && max < i) { max = i; }
    }
    return max+1;
}

void caesar(char* buf) {
    int i = -1;
    while (buf[++i]) {
        if ('a' <= buf[i] && buf[i]  <= 'z') {
            buf[i] += offset;

            if (buf[i] > 'z') {
                buf[i] -= ('z'-'a'+1);
            }
        }
    }
}
void decaesar(char* buf) {
    int i = -1;
    while (buf[++i]) {
        if ('a' <= buf[i] && buf[i]  <= 'z') {
            buf[i] -= offset;
            if (buf[i] < 'a') {
                buf[i] += ('z'-'a'+1);
            }
        }
    }
}

void kliens_lekapcsolas(int socket) {
    close(socket);
    FD_CLR(socket, &master_fds);
    --client_count;
    list_delete_socket(llist, socket);
}

void broadcast(char* mit) {
  int i;

  for (i = listener+1; i <= getmaxfd(&master_fds); ++i) {
    send(i, mit, strlen(mit), 0);
  }
}

void legutobbiak_push(char* eredeti, char* muvelet) {
  int i;

  for (i = 3; i >= 0; --i) {
    strcpy(legutobbiak[i+1], legutobbiak[i]);
  }

  sprintf(legutobbiak[0], "%s: %s", muvelet, eredeti);
}

void kliensadat_feldolgozas(int socket, char* buffer) {
    char tmp[512];
    char allapot_cod[] = "Atallitottalak kodolasra\n";
    char allapot_dec[] = "Atallitottalak dekodolasra\n";
    int i;

    printf("#%i kuldott valamit: %s", socket, buffer);

    if (strncmp(buffer, "QUIT", 4) == 0) {
        send(socket, "pá\n", 4, 0);
        kliens_lekapcsolas(socket);
        return;
    }

    if (strncmp(buffer, "?", 1) == 0) {
      for (i = 0; i < 5; ++i) {
        send(socket, legutobbiak[i], strlen(legutobbiak[i]), 0);
      }
    } else if (strncmp(buffer, "COD", 3) == 0) {
        list_socket_allapot_set(llist, socket, 0);
        send(socket, allapot_cod, sizeof allapot_cod, 0);
    } else if (strncmp(buffer, "DEC", 3) == 0) {
        list_socket_allapot_set(llist, socket, 1);
        send(socket, allapot_dec, sizeof allapot_dec, 0);
    }
    else {

        if (0 == list_socket_allapot(llist, socket)) {
            legutobbiak_push(buffer, "kodolas");
            caesar(buffer);
        } else {
            legutobbiak_push(buffer, "dekodolas");
            decaesar(buffer);
        }

        strcpy(tmp, list_socket_ipport(llist, socket));
        strcat(tmp, " ");
        strcat(tmp, buffer);
        broadcast(tmp);
    }
}

void ipport(struct sockaddr clientaddr, char* hova) {
  char buf[256];
  struct sockaddr_in *sin = (struct sockaddr_in*)&clientaddr;

  inet_ntop(AF_INET, &(sin->sin_addr), buf, sizeof(buf));
  sprintf(hova, "%s:%d", buf, ntohs(sin->sin_port));
}

int main(int argc, char* argv[]) {
    struct addrinfo hints, *res;
    int newfd;
    int yes = 1;
    fd_set read_fds;
    struct timeval timeout = {10, 0};
    int i;
    struct sockaddr clientaddr; socklen_t clientaddrlen;
    ssize_t readbytes;
    char buffer[512];
    char tulsokkliens[] = "Tul sok a kliens\n";
    Node* newnode;

    if (argc != 2) {
      fprintf(stderr, "hasznalat: %s <offset>\n", argv[0]);
      exit(1);
    }

    offset = atoi(argv[1]);
    if (offset == 0 && strcmp("0", argv[1]) != 0) {
      fprintf(stderr, "offset szam legyen!\n");
      exit(1);
    }

     llist = list_new();

    memset(legutobbiak, 0, sizeof legutobbiak);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    getaddrinfo(NULL, PORT, &hints, &res);

    listener = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (listener < 0) { perror("socket"); exit(1); }
        printf("listener: %d\n", listener);

    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    if (0 > bind(listener, res->ai_addr, res->ai_addrlen)) { perror("bind"); exit(1); };
    if (0 > listen(listener, 10)) { perror("listen"); exit(1); }
    FD_ZERO(&master_fds);
    FD_SET(listener, &master_fds);

    printf("szaladok!\n");
    for (;;) {
        read_fds = master_fds;
        select(getmaxfd(&read_fds), &read_fds, NULL, NULL, &timeout);
        for (i = 3; i <= getmaxfd(&master_fds); ++i) {
            if (FD_ISSET(i, &read_fds)) {

                if (i == listener) { /* uj kapcsolat */
                    clientaddrlen = sizeof clientaddr;
                    newfd = accept(listener, &clientaddr, &clientaddrlen);

                    if (newfd < 0) { perror("accept"); }
                    else
                    if (client_count+1 > MAXCLIENTS) { /* ha tul sok a kliensunk */
                        printf("uj kliens, de tul sok lenne (most van %d)\n", client_count);
                        send(newfd, tulsokkliens, strlen(tulsokkliens), 0);
                        close(newfd);
                    } else { /* meg belefer */
                        printf("uj kliens, es elfogadjuk\n");
                        ++client_count;
                        FD_SET(newfd, &master_fds);
                        send(newfd, "szia!\n", 7, 0);

                        newnode = calloc(sizeof(Node), 1);
                        newnode->fd = newfd;
                        newnode->allapot = 0;
                        ipport(clientaddr, newnode->ipport);
                        list_add(llist, newnode);
                    }
                } /* i==listener */
                else {
                    memset(buffer, 0, sizeof buffer);
                    readbytes = read(i, buffer, sizeof buffer -1);
                    if (readbytes == 0) { /* lecsatlakozott */
                        printf("lecsatlakozott\n");
                        kliens_lekapcsolas(i);
                    } else { /* rendes uzenet */
                        kliensadat_feldolgozas(i, buffer);
                    } /* readbytes == 0 else */
                }

            } /* if fd_isset */
        }
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;
    }

    freeaddrinfo(res);

    return 0;
}
