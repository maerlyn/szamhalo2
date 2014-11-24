#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>

struct addrinfo *res;

void termelo() {
    int sock;
    int yes = 1;
    char buf[] = "Hello World!";

    printf("T: az en pidem: %d\n", getpid());

    sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    connect(sock, res->ai_addr, res->ai_addrlen);

    send(sock, buf, strlen(buf), 0);

    printf("T: elkuldott adat: %s\n", buf);

    close(sock);
}

void fogyaszto() {
    int sock, newsock;
    int yes = 1;
    struct sockaddr client;
    char buf[256];
    socklen_t size;
    int bytes;

    printf("F: az en pidem: %d\n", getpid());

    sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    if (bind(sock, res->ai_addr, res->ai_addrlen) < 0) { perror("bind"); exit(1); }
    if (listen(sock, 10) < 0) { perror("listen"); exit(1); }

    newsock = accept(sock, &client, &size);
    if (newsock < 0) { perror("accept"); exit(1); }

    bytes = recv(newsock, buf, sizeof(buf), 0);
    if (bytes > 0) {
        buf[bytes] = buf[255] = 0;
        printf("F: kapott adat (%d byte): %s\n", bytes, buf);
    }

    close(newsock);
    close(sock);
}

void sigchld_handler(int sig) {
    if (sig == SIGCHLD) {
        printf("SIGNAL: elhalt egy gyerek\n");
    }
}

int main() {
    struct addrinfo hints;
    struct sigaction sa;
    int pid;

    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) < 0) { perror("sigaction"); exit(1); }

    memset(&hints, 0, sizeof hints);
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo(NULL, "7462", &hints, &res);

    pid = fork(); /* pid az uj process pid-e */

    if (pid == 0) { /* Luke */
        termelo();
    } else if (pid > 0) { /* en vagyok az apad */
        fogyaszto();
        if (waitpid(pid, NULL, 0) < 0) { perror("waitpid"); exit(1); }
    } else { /* negativ pid */
        perror("fork");
        exit(1);
    }

    freeaddrinfo(res);

    return 0;
}
