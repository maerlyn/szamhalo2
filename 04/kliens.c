#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/unistd.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int main(void) {
  struct addrinfo hints, *res;
  int sock;
  char buf[512];

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;

  getaddrinfo(NULL, "7462", &hints, &res);
  sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

  strcpy(buf, "684353249745684684\n");
  sendto(sock, buf, strlen(buf), 0, res->ai_addr, res->ai_addrlen);

  return 0;
}

