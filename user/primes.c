#include "kernel/types.h"
#include "user/user.h"

int filter(int *pin) {
  close(pin[1]);

  int buf;
  int prime;
  int pout[2];

  pipe(pout);

  int n = read(pin[0], &prime, sizeof(int));
  if (n == 0) {
    close(pin[0]);
    close(pout[0]);
    close(pout[1]);
    exit(0);
  }

  int pid = fork();

  if (pid == 0) {
    filter(pout);
  } else {
    fprintf(1, "prime %d\n", prime);

    while (read(pin[0], &buf, sizeof(int))) {
      if (buf % prime == 0) {
        continue;
      }
      write(pout[1], &buf, sizeof(int));
    }

    close(pin[0]);
    close(pout[1]);
    wait(&pid);
  }

  return 0;
}

int main(int argc, char *argv[]) {
  int p[2];
  pipe(p);

  int pid = fork();

  if (pid == 0) {
    filter(p);
  } else {
    for (int i = 2; i < 36; i++) {
      write(p[1], &i, sizeof i);
    }
    close(p[1]);
    wait(&pid);
  }
  exit(0);
}
