#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int down[2];
  pipe(down);

  int up[2];
  pipe(up);

  if (fork() == 0) {
    // child
    close(up[0]);
    close(down[1]);

    char buf = 0;
    read(down[0], &buf, 1);
    printf("%d: received ping\n", getpid());
    buf++;
    write(up[1], &buf, 1);

    close(up[1]);
    close(down[0]);
  } else {
    // parent
    close(down[0]);
    close(up[1]);

    char ball = 0xab;
    write(down[1], &ball, 1);
    read(up[0], &ball, 1);
    if (ball != 0xac) {
        fprintf(2, "oh shit, it's wrong. %d\n", ball);
    }
    printf("%d: received pong\n", getpid());

    close(down[1]);
    close(up[0]);
  }

  exit(0);
}
