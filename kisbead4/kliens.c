#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/unistd.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int main(int argc, char* argv[]) {
  struct addrinfo hints, *res;
  int sock;
  char buf[512];
  size_t bytes;
  struct sockaddr fromaddr; socklen_t fromaddrlen;
  int fin = 0;

  if (argc < 2) {
    fprintf(stderr, "Nem eleg argumentum\n");
    exit(1);
  }

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;

  getaddrinfo(NULL, "7462", &hints, &res);
  sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

  strcpy(buf, argv[1]);
  sendto(sock, buf, strlen(buf), 0, res->ai_addr, res->ai_addrlen);

  do {
    bytes = recvfrom(sock, buf, sizeof(buf)-1, 0, &fromaddr, &fromaddrlen);
    buf[bytes] = 0;
    fin = (strcmp(buf, "FIN") == 0 ? 1 : 0);
    if (!fin)
      printf("%s", buf);
  } while (!fin);

  return 0;
}
