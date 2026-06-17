#include "kernel/types.h"
#include "user/user.h"

void filter(int *pin) {
    close(pin[1]);

    int prime;
    if (read(pin[0], &prime, sizeof(int)) == 0) {
        close(pin[0]);
        exit(0);
    }
    printf("prime %d\n", prime);

    int pout[2];
    pipe(pout);

    if (fork() == 0) {
        filter(pout);
    } else {
        close(pout[0]);
        int buf;
        while (read(pin[0], &buf, sizeof(int))) {
            if (buf % prime != 0) {
                write(pout[1], &buf, sizeof(int));
            }
        }
        close(pin[0]);
        close(pout[1]);
        wait(0);
    }
}

int main(int argc, char *argv[]) {
    int p[2];
    pipe(p);

    if (fork() == 0) {
        filter(p);
    } else {
        close(p[0]);
        for (int i = 2; i <= 35; i++) {
            write(p[1], &i, sizeof(i));
        }
        close(p[1]);
        wait(0);
    }
    exit(0);
}
