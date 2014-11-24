#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>

int main() {
  struct addrinfo hints, *res;
  fd_set readfds, master;
  int maxfds, i, j, listener, newfds, oldmaxfds;
  struct timeval timeout = {10, 0};
  struct sockaddr clientaddr; socklen_t clientaddrlen;
  ssize_t bytes;
  char buf[512];
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
            printf("#%d kliens irta: %s\n", i, buf);

            if (buf[0] == 'q' && buf[1] == 'u' && buf[2] == 'i' && buf[3] == 't') { //csokito
              FD_CLR(i, &master);
              close(i);
              oldmaxfds = maxfds; maxfds = -1;
              for (j = 0; j < oldmaxfds; ++j) { if (FD_ISSET(j, &master) && j > maxfds) { maxfds = j; } }
              printf("lekapcsoltuk\n");

            } else {

              for (j = 0; j <= maxfds; ++j) {
                if (i != j && j != listener && FD_ISSET(j, &master)) {
                  //elkuldjuk mindenkinek, kiveve magat
                  printf("kuldjuk #%d-nek...\n", j);
                  if (send(j, buf, bytes, 0) != bytes) {
                    perror("send");
                  }
                }
              }
              printf("elkuldtuk mindenkinek\n");
            }
          }
        }
      }
    }
  }

  close(listener);

  return 0;
}
