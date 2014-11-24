#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "szotar.c"
#include "llist.c"
#include "tippek.c"

#define MAXCLIENTS 1
#define PORT "7462"

HeadNode *kliensek;
int listener;
fd_set master_fds;

int maxfds() {
  int i, ret = 0;
  for (i = 0; i < 1024; ++i) if (FD_ISSET(i, &master_fds) && i > ret) ret = i;
  return ret;
}

void szabalykuldes(int socket) {
  char buf[256];
  FILE *f = fopen("szabalyok", "r");

  if (!f) {
    perror("fopen()");
    return;
  }

  while (!feof(f)) {
    memset(buf, 0, sizeof buf);
    fread(buf, sizeof(char), sizeof buf, f);
    send(socket, buf, strlen(buf), 0);
  }

  fclose(f);
}

void szerver_inditas() {
  struct addrinfo hints, *res;
  int yes = 1;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  getaddrinfo(NULL, PORT, &hints, &res);

  listener = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (listener < 0) { perror("socket()"); exit(1); }
  setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
  if (bind(listener, res->ai_addr, res->ai_addrlen) < 0) { perror("bind()"); exit(1); }
  if (listen(listener, 10) < 0) { perror("listen()"); exit(1); }

  FD_ZERO(&master_fds);
  FD_SET(listener, &master_fds);

  printf("figyelek...\n");

  freeaddrinfo(res);
}

void ujkliens(int fd, struct sockaddr addr) {
  struct sockaddr_in *sin = (struct sockaddr_in*)&addr;
  char buf[INET_ADDRSTRLEN];
  Node *n;

  inet_ntop(AF_INET, &(sin->sin_addr), buf, INET_ADDRSTRLEN);
  sprintf(buf, "%s:%d", buf, ntohs(sin->sin_port));

  n = calloc(sizeof(Node), 1);
  n->fd = fd;
  strcpy(n->ipport, buf);

  list_add(kliensek, n);
  szabalykuldes(fd);
  FD_SET(fd, &master_fds);
}

void klienstorles(int fd) {
  FD_CLR(fd, &master_fds);
  close(fd);
  list_delete_socket(kliensek, fd);
}

void broadcast(char* msg) {
  Node *n = kliensek->first;
  while (n) {
    send(n->fd, msg, strlen(msg), 0);
    n = n->next;
  }
}

int main(void) {
  int i, j, egyeznek, terminated = 0, newfd;
  fd_set read_fds;
  struct timeval timeout = {10, 0};
  struct sockaddr clientaddr; socklen_t clientaddrlen;
  char buf[256], buf2[1024];
  Node *n;

  int jatek_fut = 0, gyerek_vagyok = 0;
  size_t bytes;
  char* kitalalando_szo;
  int aktualis_jatekos_fd;

  srand(time(NULL));

  szotar_betolt();
  kliensek = list_new();
  szerver_inditas();
  FD_SET(STDIN_FILENO, &master_fds);

  while (!terminated) {
    read_fds = master_fds;
    i = select(maxfds()+1, &read_fds, NULL, NULL, &timeout);
    for (i = 0; i <= maxfds(); ++i) {
      if (!FD_ISSET(i, &read_fds)) continue; /* megsporolunk egy indent szintet */

      if (i == STDIN_FILENO) {
        read(STDIN_FILENO, buf, sizeof buf);
        if (strncmp(buf, "quit", 4) == 0) {
          printf("Quitting...\n");
          terminated = 1;
          break;
        }
      }

      if (i == listener) { /* uj kapcsolat */
        clientaddrlen = sizeof clientaddr;
        newfd = accept(listener, &clientaddr, &clientaddrlen);
        if (kliensek->count >= MAXCLIENTS) {
          printf("tul sok kliens, forkolunk\n");
          if (fork()) { /* szulok vagyunk */
            jatek_fut = 0;

            list_delete(kliensek);
            kliensek = list_new();

            FD_ZERO(&master_fds);
            FD_SET(listener, &master_fds);

            tippek_takarit();
          } else {
            gyerek_vagyok = 1;
            continue;
          }
        }
        broadcast("Uj kliens lepett be\n");
        ujkliens(newfd, clientaddr);
        if (jatek_fut) {
          sprintf(buf, "A jatek mar megy, a kitalalando szo hossza: %lu\n", (unsigned long)strlen(kitalalando_szo));
          send(newfd, buf, strlen(buf), 0);
        }
        continue;
      }

      bytes = read(i, buf, sizeof buf);
      buf[bytes] = 0;
      strcpy(buf2, trim(buf)); strcpy(buf, buf2);
      printf("\tezt kaptam: >%s<\n", buf);
      if (bytes == 0 || strncmp(buf, "KILEP", strlen("KILEP")) == 0) { /* lekapcsolodott */
        klienstorles(i);
        if (kliensek->count == 0) jatek_fut = 0;
        else broadcast("Egy kliens lekapcsolodott\n");

        if (gyerek_vagyok) {
          tippek_takarit();
          list_delete(kliensek);
          szotar_takarit();
          exit(0);
        }
        continue;
      }

      if (!jatek_fut && strncmp(buf, "INDUL", strlen("INDUL")) == 0) {
        kitalalando_szo = szotar_random();
        sprintf(buf, "Jatek indul! A valasztott szo hossza: %lu\n", (unsigned long)strlen(kitalalando_szo));
        broadcast(buf);
        jatek_fut = 1;
        aktualis_jatekos_fd = kliensek->first->fd;
        printf("Jatek indul! A szo: %s\n", kitalalando_szo);
        send(aktualis_jatekos_fd, "A te tipped varjuk: ", strlen("A te tipped varjuk: "), 0);
        continue;
      }

      if (jatek_fut && strncmp(buf, "SZOTAR", strlen("SZOTAR")) == 0) {
        szotar_hozzaad(buf+strlen("SZOTAR "));
        send(i, "A szo bekerult a szotarba\n", strlen("A szo bekerult a szotarba\n"), 0);
        continue;
      }

      if (jatek_fut && buf[0] == '?') {
        tippek_kliensnek(i);
        continue;
      }

      /* innentol tipp mod, parancsok mind fentebb */

      if (jatek_fut && aktualis_jatekos_fd != i) {
        send(i, "Nem te jossz\n", strlen("Nem te jossz\n"), 0);
        continue;
      }

      if (jatek_fut && aktualis_jatekos_fd == i) {
        if (strlen(buf) != strlen(kitalalando_szo)) {
          sprintf(buf, "Teves szohossz, %lu karaktert varok\n", (unsigned long)strlen(kitalalando_szo));
          send(i, buf, strlen(buf), 0);
          continue;
        }

        j = -1; egyeznek = 0;
        /* |32 hogy ne szamitson a kisbetu-nagybetu */
        while (kitalalando_szo[++j]) egyeznek += (kitalalando_szo[j]|32)==(buf[j]|32) ? 1 : 0;
        tipp_hozzaad(buf, egyeznek, list_socket_ipport(kliensek, i));
        sprintf(buf, "Tipp: %s, egyezik: %d\n", tippek[tippekszama-1], egyeznek);
        broadcast(buf);

        n = list_get_node(kliensek, aktualis_jatekos_fd);
        if (egyeznek == strlen(kitalalando_szo)) {
          ++n->pontszam;
          send(aktualis_jatekos_fd, "Kaptal egy pontot\n", strlen("Kaptal egy pontot\n"), 0);
          /* pontok mindenkinek */
          buf2[0] = 0;
          for (n = kliensek->first; n; n = n->next) {
            sprintf(buf, "%s - %d pont\n", n->ipport, n->pontszam);
            strcat(buf2, buf);
          }
          broadcast(buf2);
          /* uj jatek */
          kitalalando_szo = szotar_random();
          sprintf(buf, "Jatek indul! A valasztott szo hossza: %lu\n", (unsigned long)strlen(kitalalando_szo));
          broadcast(buf);
          jatek_fut = 1;
          aktualis_jatekos_fd = kliensek->first->fd;
          printf("Jatek indul! A szo: %s\n", kitalalando_szo);
          send(aktualis_jatekos_fd, "A te tipped varjuk: ", strlen("A te tipped varjuk: "), 0);
        } else {
          if (n->next)
            aktualis_jatekos_fd = n->next->fd;
          else
            aktualis_jatekos_fd = kliensek->first->fd;

          send(aktualis_jatekos_fd, "A te tipped varjuk: ", strlen("A te tipped varjuk: "), 0);
        }
        continue;
      }

      if (!jatek_fut) {
        buf[strlen(buf)] = 0;
        szotar_hozzaad(buf);
        send(i, "A szo bekerult a szotarba\n", strlen("A szo bekerult a szotarba\n"), 0);
      }
    }
    timeout.tv_sec = 10;
  }

  szotar_takarit();
  list_delete(kliensek);
  tippek_takarit();
  close(listener);
  return 0;
}
