#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <sys/select.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>

#define PORT "7462"
#define MAX_GAMES 10
#define FIELD_SIZE 15

#define STDIN 0

typedef struct {
  int fd;
  struct sockaddr_in address;
  char printable_address[INET_ADDRSTRLEN];
} clientinfo;

/* fuu,  globalis valtozok, de eszmeletlen mertekben kenyelmesebb igy,
mint valtozokat passzolgatni n+1 fuggvenynek */
int listener, gamecount = 0;
fd_set master_fds;
clientinfo clients[2];
char gamer_signs[] = {'X', 'O'};
pid_t games[MAX_GAMES];

int victor(char field[FIELD_SIZE][FIELD_SIZE]) {
  int i, j, player, egybe = 0;

  for (player = 0; player <= 1; ++player) {
    for (i = 0; i < FIELD_SIZE; ++i)
      for (j = 0; j < FIELD_SIZE; ++j)
        if (field[i][j] == gamer_signs[player]) {
          if (++egybe == 5) return player;
        }
        else egybe = 0;

    egybe = 0;
    for (i = 0; i < FIELD_SIZE; ++i)
      for (j = 0; j < FIELD_SIZE; ++j)
        if (field[j][i] == gamer_signs[player]) {
          if (++egybe == 5) return player;
        }
          else egybe = 0;

  for (i = 0; i <= FIELD_SIZE-5; ++i)
    for (j = 0; j < FIELD_SIZE-5; ++j)
      if (
          field[i][j] == gamer_signs[player] &&
          field[i+1][j+1] == gamer_signs[player] &&
          field[i+2][j+2] == gamer_signs[player] &&
          field[i+3][j+3] == gamer_signs[player] &&
          field[i+4][j+4] == gamer_signs[player]
      )
        return player;

  for (i = 0; i < FIELD_SIZE-5; ++i)
    for (j = FIELD_SIZE-5; j >= 0; --j)
      if (
          field[i][j] == gamer_signs[player] &&
          field[i-1][j+1] == gamer_signs[player] &&
          field[i-2][j+2] == gamer_signs[player] &&
          field[i-3][j+3] == gamer_signs[player] &&
          field[i-4][j+4] == gamer_signs[player]
      )
        return player;
  }

  return -1;
}

void render_field(char field[FIELD_SIZE][FIELD_SIZE], char* destination) {
  char buf[3*FIELD_SIZE];
  int i, j;

  memset(destination, 0, sizeof destination);
  memset(buf, 0, sizeof(buf));

  /* header */
  sprintf(buf, "    ");
  for (i = 0; i < FIELD_SIZE; ++i) {
    sprintf(buf, "%s%c ", buf, i+'A');
  }
  strcat(buf, "\n");
  strcpy(destination, buf);

  /* field */
  for (i = 0; i < FIELD_SIZE; ++i) {
    sprintf(buf, "% 3d|", i+1);

    for (j = 0; j < FIELD_SIZE; ++j) {
      sprintf(buf, "%s%c|", buf, field[j][i]);
    }

    strcat(buf, "\n");
    strcat(destination, buf);
  }
}

void broadcast(char* msg) {
  send(clients[0].fd, msg, strlen(msg), 0);
  send(clients[1].fd, msg, strlen(msg), 0);
}

int create_game_server() {
  pid_t pid;
  char field[FIELD_SIZE][FIELD_SIZE];
  char field_output[4 * FIELD_SIZE * FIELD_SIZE];
  int current_player, max_fd, i, vict, x, y;
  fd_set read_fds;
  struct timeval timeout = {10, 0};
  char buf[512];

  pid = fork();

  if (pid != 0) { /* we're in the parent */
    return pid;
  }

  close(listener); listener = -1;
  FD_ZERO(&master_fds);
  FD_SET(clients[0].fd, &master_fds);
  FD_SET(clients[1].fd, &master_fds);

  for (i = 0; i <= 1; ++i) {
    sprintf(buf, "Your character is '%c'\n", gamer_signs[i]);
    send(clients[i].fd, buf, strlen(buf), 0);
  }

  memset(field, 32, sizeof field); /* fill with spaces */
  render_field(field, field_output);
  broadcast(field_output);

  max_fd = clients[0].fd > clients[1].fd ? clients[0].fd : clients[1].fd;


  current_player = 0;
  send(clients[0].fd, "It's your turn, what's your step? ", strlen("It's your turn, what's your step? "), 0);
  send(clients[1].fd, "It's the other player's turn\n", strlen("It's the other player's turn\n"), 0);

  /*  handle game */
  while (current_player >= 0) {
    read_fds = master_fds;
    select(max_fd+1, &read_fds, NULL, NULL, &timeout);

    for (i = 0; i <= 1; ++i) {

      if (FD_ISSET(clients[i].fd, &read_fds) && i == current_player) {
        read(clients[i].fd, buf, sizeof buf);

        x = buf[0] - 'A';
        y = atoi(buf+1) - 1;

        if (0 <= x && x < FIELD_SIZE && 0 <= y && y < FIELD_SIZE) {
          if (field[x][y] == ' ') {
            field[x][y] = gamer_signs[current_player];

            render_field(field, field_output);
            broadcast(field_output);

            vict = victor(field);
            if (vict != -1) {
              sprintf(buf, "Player %c won.\n", gamer_signs[vict]);
              broadcast(buf);
              printf("%s", buf);
              close(clients[0].fd);
              close(clients[1].fd);
              exit(0);
            }

            send(clients[current_player].fd, "Waiting for the other player...\n", strlen("Waiting for the other player...\n"), 0);
            current_player ^= 1;
            send(clients[current_player].fd, "It's your turn, what's your step? ", strlen("It's your turn, what's your step? "), 0);
        } else {
            send(clients[i].fd, "That field's already occupied\n", strlen("That field's already occupied\n"), 0);
            send(clients[i].fd, "It's your turn, what's your step? ", strlen("It's your turn, what's your step? "), 0);
        }

        }
      }
      else if (FD_ISSET(clients[i].fd, &read_fds) && i != current_player) {
        read(clients[i].fd, buf, sizeof buf);
        send(clients[i].fd, "This ain't your turn!\n", strlen("This ain't your turn!\n"), 0);
      }
    }

    timeout.tv_sec = 10;
  }

  exit(0);
}

void signal_handler(int signo, siginfo_t *info, void *context) {
  int i, j;

  info = info; context = context; /* unused warnings */

  switch (signo) {
    case SIGCHLD:
      printf("A child has just quit\n");

      /* shift all pids after the quitting one left */
      for (i = 0; i < gamecount; ++i) {
        if (games[i] == info->si_pid) {
          for (j = i; j < gamecount-1; ++j) {
            games[j] = games[j+1];
          }
        }
      }
      --gamecount;

      printf("Games running: %d\n", gamecount);

      break;

    case SIGUSR1:
      if (listener < 0) {
        broadcast("Server is quitting\n");
        exit(0);
      }
      break;
   }
}

void register_signal_handler() {
  struct sigaction sa;

  sa.sa_sigaction = signal_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO;
  if (sigaction(SIGCLD, &sa, NULL) < 0)  { perror("sigaction"); exit(1); }
  if (sigaction(SIGUSR1, &sa, NULL) < 0)  { perror("sigaction"); exit(1); }
}

int main(void) {
  struct addrinfo hints, *res;
  int yes = 1, max_fd, i, client_count = 0, new_fd, aborted = 0, j;
  struct timeval timeout = {10, 0};
  fd_set read_fds;
  struct sockaddr clientaddr; socklen_t clientaddrlen;
  char buf[16];

  register_signal_handler();

  /* create main server */
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  getaddrinfo(NULL, PORT, &hints, &res);

  listener = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (listener < 0) { perror("socket"); exit(1); }
  setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
  if (bind(listener, res->ai_addr, res->ai_addrlen) < 0) { perror("bind"); exit(1); }
  freeaddrinfo(res);
  if (listen(listener, 1) < 0) { perror("listen"); exit(1); }

  FD_ZERO(&master_fds);
  FD_SET(STDIN, &master_fds);
  FD_SET(listener, &master_fds);
  max_fd = listener;

  /* main server loop */
  while (!aborted) {
    read_fds = master_fds;
    select(max_fd + 1, &read_fds, NULL, NULL, &timeout);

    for (i = 0; i <= max_fd; ++i) {
      if (FD_ISSET(i, &read_fds)) {
        if (i == STDIN) {
          read(STDIN, buf, sizeof buf);
          if (strncmp(buf, "quit", 4) == 0) {
            printf("Quitting...");
            aborted = 1;
            for (j = 0; j < gamecount; ++j) {
              kill(games[j], SIGUSR1);
            }
          }
        }
        else
        if (i == listener) { /* new client */
          clientaddrlen = sizeof clientaddr;
          new_fd = accept(listener, &clientaddr, &clientaddrlen);

          if (gamecount == MAX_GAMES) {
            send(new_fd, "Too many games already\n", strlen("Too many games already\n"), 0);
            close(new_fd);
            continue;
          }

          clients[client_count].fd = new_fd;
          clients[client_count].address = *((struct sockaddr_in*)&clientaddr);
          inet_ntop(AF_INET, &(clients[client_count].address.sin_addr), clients[client_count].printable_address, INET_ADDRSTRLEN);

          printf("client connected from %s\n", clients[client_count].printable_address);

          if (++client_count == 2) {
            printf("creating game server...\n");
            client_count = 0;

            games[gamecount++] = create_game_server();
            printf("Games running: %d\n", gamecount);

            close(clients[0].fd);
            close(clients[1].fd);
            FD_CLR(clients[0].fd, &master_fds);
            FD_CLR(clients[1].fd, &master_fds);
          } else {
            send(new_fd, "Waiting for another player to connect...\n", strlen("Waiting for another player to connect...\n"), 0);
          }  /* ++client_count == 2 */
        } /* i == listener */
      } /* fd_isset */
    } /* for */

    timeout.tv_sec = 10;
  }

  for (i = gamecount; i > 0; --i) {
    printf("Waiting for %d games to quit...\n", i);
    waitpid(games[i-1], NULL, 0);
  }

  close(listener);

  return 0;
}

