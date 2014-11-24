#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>

int main(int argc, char* argv[]) {
  struct addrinfo hints, *res, *p;
  int rv;
  char msg[INET6_ADDRSTRLEN];
  struct sockaddr_in  *sin;
  struct sockaddr_in6 *sin6;
  
  if (argc < 2) {
    fprintf(stderr, "nem eleg argumentum\n");
    exit(1);
  }
  
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  
  if (0 == (rv = getaddrinfo(argv[1], "http", &hints, &res))) {
    printf("Host: %s\n", argv[1]);
    for (p = res; p != NULL; p = p->ai_next) {
      switch(p->ai_family) {
        case AF_INET:
          sin = (struct sockaddr_in*) p->ai_addr;
          inet_ntop(AF_INET, &(sin->sin_addr), msg, INET_ADDRSTRLEN);
          break;
        
        case AF_INET6:
          sin6 = (struct sockaddr_in6*) p->ai_addr;
          inet_ntop(AF_INET6, &(sin6->sin6_addr), msg, INET6_ADDRSTRLEN);
          break;
      
        default:
          strcpy(msg, "unknown family");
      }
      printf("\t%s\n", msg);
    }
    freeaddrinfo(res);

  } else {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    exit(1);
  }

  return 0;  
}
