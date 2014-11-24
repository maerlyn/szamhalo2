#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/unistd.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int main(void) {
  struct addrinfo hints, *res, lookup_hints, *lookup_res, *p;
  int sock;
  struct sockaddr fromaddr; socklen_t fromlen = sizeof fromaddr;
  char buf[512], service[20];
  char host[INET6_ADDRSTRLEN];
  int bytecount;
  struct sockaddr_in *sin;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;

  getaddrinfo(NULL, "7462", &hints, &res);
  sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  bind(sock, res->ai_addr, res->ai_addrlen);

  printf("futok\n");

  bytecount = recvfrom(sock, buf, sizeof buf, 0, &fromaddr, &fromlen);
  sin = (struct sockaddr_in*)&fromaddr;
  inet_ntop(sin->sin_family, &(sin->sin_addr), host, INET_ADDRSTRLEN);
  printf("received %d bytes from %s\n", bytecount, host);
  buf[bytecount] = 0;
  printf("%s\n", buf);

  //megjott az adat, utana kell nezni
  memset(&lookup_hints, 0, sizeof lookup_hints);
  lookup_hints.ai_family = AF_INET;
  lookup_hints.ai_socktype = SOCK_STREAM;

  getaddrinfo(buf, "http", &lookup_hints, &lookup_res);
  for (p = lookup_res; p; p = p->ai_next) {
    sin = (struct sockaddr_in*) p->ai_addr;
    inet_ntop(AF_INET, &(sin->sin_addr), buf, INET_ADDRSTRLEN);

    getnameinfo(p->ai_addr, p->ai_addrlen, host, sizeof host, service, sizeof service, 0);
    sprintf(buf, "%s\t%s\n", buf, service);
    printf("%s", buf);

    sendto(sock, buf, strlen(buf), 0, &fromaddr, fromlen);
  }
  strcpy(buf, "FIN");
  sendto(sock, buf, strlen(buf), 0, &fromaddr, fromlen);

  freeaddrinfo(res);
  freeaddrinfo(lookup_res);

  return 0;
}
