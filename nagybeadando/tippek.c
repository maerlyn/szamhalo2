#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CIMHOSSZ (INET_ADDRSTRLEN+6)

char (*tippek)[SZOHOSSZ];
int *valaszok;
char (*kuldok)[CIMHOSSZ];

int tippekszama = -1;

void tipp_hozzaad(char* tipp, int valasz, char* kuldo) {
  if (tippekszama == -1) {
    tippek = calloc(sizeof(char), SZOHOSSZ);
    valaszok = calloc(sizeof(int), SZOHOSSZ);
    kuldok = calloc(sizeof(char), CIMHOSSZ);
    tippekszama = 0;
  } else {
    tippek = realloc(tippek, (tippekszama+1) * SZOHOSSZ);
    valaszok = realloc(valaszok, (tippekszama+1) * sizeof(int));
    kuldok = realloc(kuldok, (tippekszama+1) * CIMHOSSZ);
  }

  strcpy(tippek[tippekszama], tipp);
  valaszok[tippekszama] = valasz;
  strcpy(kuldok[tippekszama], kuldo);
  ++tippekszama;
}

void tippek_kliensnek(int fd) {
  int i;
  char buf[512];
  for (i = 0; i < tippekszama; ++i) {
    sprintf(buf, "Tipp: %s\nKuldo: %s\nValasz: %d\n", tippek[i], kuldok[i], valaszok[i]);
    send(fd, buf, strlen(buf), 0);
  }
}

void tippek_takarit() {
  if (tippekszama != -1) {
    free(tippek);
    free(valaszok);
    free(kuldok);
    tippekszama = -1;
  }
}
