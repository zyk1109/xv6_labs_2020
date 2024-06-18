#include "user/user.h"

void mapping(int n, int fd[]) {
    close(n);
    dup(fd[n]);
    close(fd[0]);
    close(fd[1]);
}

void primes() {
    int fd[2];
    int previous, current;
    if(read(0, &previous, sizeof(previous))) {
        fprintf(1, "prime %d\n", previous);
        pipe(fd);
        if(fork() == 0) {
            mapping(1, fd);
            while(read(0, &current, sizeof(current))) {
                if(current % previous != 0) {
                    write(1, &current, sizeof(current));
                }
            }
        }else {
            wait(0);
            mapping(0, fd);
            primes();
        }
    }
    exit(0);
}

int main(int argc, char *argv[]) {
    int fd[2];

    pipe(fd);
    if(fork() == 0) {
        mapping(1, fd);
        for(int i = 2; i <= 35; i++) {
            write(1, &i, sizeof(i));
        }
        close(1);
    }else {
        wait(0);
        mapping(0, fd);
        primes();
    }
    exit(0);
}

