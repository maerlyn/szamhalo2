#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

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
      break;
        
    case AF_INET6:
      sin6 = (struct sockaddr_in6*) &their_addr;
      inet_ntop(AF_INET6, &(sin6->sin6_addr), msg, INET6_ADDRSTRLEN);
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
  char msg[INET6_ADDRSTRLEN];
  int yes = 1;
  
  fd_set master, read_fds;
  int fdmax, i;
  
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  getaddrinfo(NULL, PORT, &hints, &res);
  
  FD_ZERO(&master);
  FD_ZERO(&read_fds);
  
  for (; res != NULL; res = res->ai_next) {
    listener = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    
    if (res->ai_family == AF_INET6) {
      setsockopt(listener, IPPROTO_IPV6, IPV6_V6ONLY, &yes, sizeof(int));
    }
    
    if (bind(listener, res->ai_addr, res->ai_addrlen) < 0) { perror("bind"); exit(1); }
    listen(listener, BACKLOG);
    FD_SET(listener, &master);
    fdmax = listener;
    printf("listening on #%d\n", listener);
  }
  
  addr_size = sizeof(their_addr);
  
  for(;;) {
    read_fds = master;
    if (select(fdmax+1, &read_fds, NULL, NULL, NULL) < 0) { perror("select()"); exit(1); }
    
    for (i = 0; i <= fdmax; ++i) {
      if (FD_ISSET(i, &read_fds)) { //figyeldoda, kliens jott
        new_fd = accept(i, (struct sockaddr*) &their_addr, &addr_size);
        client_address(their_addr, msg);
        printf("%s", msg);
        send(new_fd, msg, strlen(msg), 0);
        close(new_fd);
      }
    }
  } //for(;;)
  
  close(listener);

  return 0;
}
