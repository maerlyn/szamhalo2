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
  struct sockaddr fromaddr; socklen_t fromlen = sizeof fromaddr;
  char buf[512];
  char host[INET_ADDRSTRLEN];
  int bytecount;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;

  getaddrinfo(NULL, "7462", &hints, &res);
  sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  bind(sock, res->ai_addr, res->ai_addrlen);

  bytecount = recvfrom(sock, buf, sizeof buf, 0, &fromaddr, &fromlen);
  struct sockaddr_in *sin = (struct sockaddr_in*) &fromaddr;
  inet_ntop(sin->sin_family, &(sin->sin_addr), host, INET_ADDRSTRLEN);
  printf("received %d bytes from %s\n", bytecount, host);
  buf[bytecount] = 0;
  printf("%s", buf);



  return 0;
}

