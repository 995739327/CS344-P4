#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>   // errno
#include <limits.h>  // INT_MAX

int main(int argc, char *argv[]) {

  if (argc != 2) {
    fprintf(stderr, "USAGE: %s [keylength]\n", argv[0]);
    return 1;
  }

  char* str;
  int num = strtol(argv[1], &str, 10);

  if (errno != 0 || *str != '\0' || num > INT_MAX) {
    fprintf(stderr, "USAGE: %s [keylength] (keylength must be an integer)\n", argv[0]);
    return 1;
  }

  int i;
  int keyl = num;
  time_t t;
  char key[keyl + 1];

  srand((unsigned) time(&t));

  for (i = 0; i < keyl; i++) {
    int ch = rand() % 27 + 65;
    if (ch == 91) ch = 32;
    key[i] = ch;
  }
  key[keyl] = '\n';

  printf("%s", key);

  return 0;
}
