#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>

void histogram(char *msg, char *return_address) {
  char buf[1024];
  int counts[26], i;

  memset(counts, 0, sizeof counts);

  for (i = 0; msg[i]; ++i)
    if ('a' <= msg[i] && msg[i] <= 'z')
      ++counts[msg[i] - 'a'];

  buf[0] = 0;
  for (i = 0; i < 26; ++i)
    if (counts[i]) {
      sprintf(buf, "%s %d %c |", buf, counts[i], (i+'a'));
    }

  buf[strlen(buf)-1] = 0; // utolso | levagasa
  strcat(buf, "\n");
  strcpy(return_address, buf);
}

int main() {
  struct addrinfo hints, *res;
  fd_set readfds, master;
  int maxfds, i, j, listener, newfds, oldmaxfds;
  struct timeval timeout = {10, 0};
  struct sockaddr clientaddr; socklen_t clientaddrlen;
  ssize_t bytes;
  char buf[512], msg[1024];
  int yes=1;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  getaddrinfo(NULL, "7462", &hints, &res);

  listener = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  if (listener < 0) { perror("socket"); exit(1); }
  if (bind(listener, res->ai_addr, res->ai_addrlen) < 0) { perror("bind"); exit(1); }
  if (listen(listener, 10) < 0) { perror("listen"); exit(1); }

  freeaddrinfo(res);

  FD_ZERO(&master);
  FD_SET(listener, &master);
  maxfds = listener;

  printf("figyelek...\n");
  while (1) {
    readfds = master;
    select(maxfds+1, &readfds, NULL, NULL, &timeout);

    for (i = 0; i <= maxfds; ++i) {
      if (FD_ISSET(i, &readfds)) {
        if (listener == i) {
          //uj kliens csatlakozott
          newfds = accept(listener, &clientaddr, &clientaddrlen);
          FD_SET(newfds, &master);
          maxfds = (newfds > maxfds ? newfds : maxfds);
          printf("kliens csatlakozott: #%d\n", newfds);
        } else {
          //kliens irt valamit - olvassuk
          bytes = read(i, buf, sizeof(buf)-1);

          if (bytes == 0) {
            //disconnect
            FD_CLR(i, &master);
            close(i);
            oldmaxfds = maxfds; maxfds = -1;
            for (j = 0; j < oldmaxfds; ++j) { if (FD_ISSET(j, &master) && j > maxfds) { maxfds = j; } }
            printf("kliens lecsatlakozott, #%d\n", i);
          } else {
            buf[bytes] = buf[511] = 0;
            printf("#%d kliens irta: %s", i, buf);

            if (buf[0] == 'q' && buf[1] == 'u' && buf[2] == 'i' && buf[3] == 't') { //csokito, ronda es finom
              FD_CLR(i, &master);
              close(i);
              oldmaxfds = maxfds; maxfds = -1;
              for (j = 0; j < oldmaxfds; ++j) { if (FD_ISSET(j, &master) && j > maxfds) { maxfds = j; } }
              printf("lekapcsoltuk\n");

            } else {

              for (j = 0; j <= maxfds; ++j) {
                if (i != j && j != listener && FD_ISSET(j, &master)) {
                  histogram(buf, buf);
                  sprintf(msg, "#%d: %s\n", i, buf);

                  bytes = strlen(msg);
                  //elkuldjuk mindenkinek, kiveve magat
                  send(j, msg, bytes, 0);
                }
              }
            }
          }
        }
      }
    }
  } //annyira szep ez a sok zarojel, mintha lisp-eznenk

  close(listener);

  return 0;
}
