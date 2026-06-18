#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "kernel/param.h"
#include "user/user.h"

#define BUFSIZE 512

void lines(void (*f)(char *, void *), void *x) {
    static char buf[BUFSIZE];

    int buf_len = 0;
    int i = 0;
    while (1) {
        int n = read(0, buf + buf_len, BUFSIZE - buf_len);
        if (n < 0) {
            fprintf(2, "xargs: error reading stdin\n");
            exit(1);
        }
        buf_len += n;

        if (n == 0) {
            if (buf_len == 0) break;
            buf[buf_len] = '\n';
        }

        // Look for a newline.
        for (; i < buf_len; i++) {
            if (buf[i] == '\n') break;
        }
        if (i == BUFSIZE) {
            fprintf(2, "split: line too long\n");
            exit(1);
        }

        // Found one.
        if (i < buf_len) {
            buf[i++] = '\0';
            f(buf, x);
            memmove(buf, buf + i, buf_len - i);
            buf_len -= i;
            i = 0;
        }

        if (n == 0) break;
    }
}

int is_whitespace_or_null(char c) {
    return strchr(" \t\n", c) || c == '\0';
}

// Modifies s in-place.
// `out` must have space for MAXARG+1 pointers.
int split_words(char *s, char *out[]) {
    int n = 0;

    int start = 0;
    for (int i = 0; ; i++) {
        if (is_whitespace_or_null(s[i])) {
            if (start < i) {
                out[n++] = s + start;
            }
            if (n > MAXARG) {
                fprintf(1, "split: too many words\n");
                exit(1);
            }
            if (s[i] == '\0') break;
            s[i] = '\0';

            start = i + 1;
        }
    }

    out[n] = 0;
    return n;
}

// `out` must have space for MAXARG+1 pointers.
int concat_words(int n1, char *w1[], int n2, char *w2[], char *out[]) {
    int n = n1 + n2;
    if (n > MAXARG) {
        return -1;
    }

    for (int i = 0; i < n1; i++) {
        out[i] = w1[i];
    }
    for (int i = 0; i < n2; i++) {
        out[n1 + i] = w2[i];
    }
    out[n] = 0;

    return n;
}

void run(char *program, char **args) {
    if (fork() == 0) {
        close(0);
        open("/console", O_RDWR);
        if (exec(program, args) < 0) {
            exit(1);
        }
    } else {
        wait(0);
    }
}

struct base_args {
    char *prog_name;
    int n1;
    char **w1;
};

void xargs(char *line, void *x) {
    struct base_args *a = x;

    static char *w2[MAXARG + 1];
    int n2 = split_words(line, w2);

    static char *args[MAXARG + 1];
    if (concat_words(a->n1, a->w1, n2, w2, args) < 0) {
        fprintf(2, "xargs: too many args\n");
        return;
    }

    run(a->prog_name, args);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(1, "xargs: expected at least 1 argument\n");
        exit(1);
    }

    struct base_args a = {
        .prog_name = argv[1],
        .n1 = argc - 1,
        .w1 = argv + 1,
    };
    lines(xargs, &a);

    exit(0);
}
