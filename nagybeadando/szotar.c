#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SZOHOSSZ 32

char (*szotar)[SZOHOSSZ];
int szavakszama = -1;

char* trim(char* input) {
  while (input[0] && isspace(input[0])) ++input;
  while (strlen(input) && isspace(input[strlen(input)-1])) input[strlen(input)-1] = 0;
  return input;
}

void szotar_hozzaad(char* buf) {
  szotar = realloc(szotar, (szavakszama+1) * SZOHOSSZ);
  strcpy(szotar[szavakszama], trim(buf));
  ++szavakszama;
}

void szotar_betolt() {
  FILE* f = fopen("szotar", "r");
  char buf[SZOHOSSZ];

  if (szavakszama != -1) free(szotar);

  szavakszama = 0;
  while (EOF != fscanf(f, "%s", buf)) {
    szotar_hozzaad(buf);

  }

  fclose(f);
  printf("szotar betoltve %d szoval\n", szavakszama);
}

void szotar_kiir() {
  int i;
  for (i = 0; i < szavakszama; ++i) {
    printf("%s\n", szotar[i]);
  }
}

void szotar_takarit() {
  free(szotar);
  szotar = 0;
  szavakszama = -1;
}

char* szotar_random() {
  int i = rand() % szavakszama;
  return szotar[i];
}
