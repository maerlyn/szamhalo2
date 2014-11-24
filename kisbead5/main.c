#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT "7462"
#define BACKLOG 1

void client_address(struct sockaddr_storage their_addr, char* return_address) {
  char msg[INET6_ADDRSTRLEN];
  struct sockaddr_in *sin;
  struct sockaddr_in6 *sin6;

  switch (their_addr.ss_family) {
    case AF_INET:
      sin = (struct sockaddr_in*) &their_addr;
      inet_ntop(AF_INET, &(sin->sin_addr), msg, INET_ADDRSTRLEN);
      sprintf(msg, "%s:%d", msg, ntohs(sin->sin_port));
      break;

    case AF_INET6:
      sin6 = (struct sockaddr_in6*) &their_addr;
      inet_ntop(AF_INET6, &(sin6->sin6_addr), msg, INET6_ADDRSTRLEN);
      sprintf(msg, "%s:%d", msg, ntohs(sin6->sin6_port));
      break;

    default:
      msg[0] = 0;
      strcat(msg, "unknown family");
  }

  strcat(msg, "\n");
  strcpy(return_address, msg);
}

int main(void) {
  struct sockaddr_storage their_addr;
  socklen_t addr_size;
  struct addrinfo hints, *res;
  int listener, new_fd;
  char msg[INET_ADDRSTRLEN];
  int yes = 1;
  pid_t newpid;
  size_t bytes;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  getaddrinfo(NULL, PORT, &hints, &res);

  listener = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

  if (bind(listener, res->ai_addr, res->ai_addrlen) < 0) { perror("bind"); exit(1); }
  listen(listener, BACKLOG);

  addr_size = sizeof(their_addr);

  printf("figyelek\n");
  for(;;) {
    new_fd = accept(listener, (struct sockaddr*)&their_addr, &addr_size);

    newpid = fork();

    if (newpid == 0) { // gyerek
      close(listener);
      client_address(their_addr, msg);
      printf("%s", msg);
      send(new_fd, msg, strlen(msg), 0);
      bytes = read(new_fd, msg, sizeof msg -1);
      msg[bytes-2] = 0; //\r\n levagasa a vegerol

      if (0 == strcmp(msg, "SZULO")) {
        sprintf(msg, "szulo: %d\n", getppid());
      } else
      if (0 == strcmp(msg, "GYEREK")) {
        sprintf(msg, "gyerek: %d\n", getpid());
      } else
        strcpy(msg, "ismeretlen parancs\n");

      send(new_fd, msg, strlen(msg), 0);


      close(new_fd);
      exit(0); //enelkul forkbomba lesz, ketszeris ugy jartam, mire leesett :)
    } else { //szulo
      close(new_fd);
    }
  } //for(;;)

  close(listener);

  return 0;
}
