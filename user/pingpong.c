#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    int down[2];
    pipe(down);

    int up[2];
    pipe(up);

    if (fork() == 0) {
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
        close(up[1]);
        close(down[0]);

        char ball = 0xab;

        // Bounce
        write(down[1], &ball, 1);
        read(up[0], &ball, 1);

        if (ball != 0xac) {
            fprintf(2, "pingpong: that's not right! %d\n", ball);
            exit(1);
        }
        printf("%d: received pong\n", getpid());

        close(up[0]);
        close(down[1]);

        if (wait(0) < 0) {
            fprintf(2, "pingpong: error waiting for child\n");
            exit(1);
        }
    }

    exit(0);
}
