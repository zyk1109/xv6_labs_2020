#include "user/user.h"


int main() {
	int p1[2];
    int p2[2];
	pipe(p1);
    pipe(p2);
	if(fork() == 0) {
		char buf_read;
        char buf_write = 'a';
        close(p1[1]);
        close(p2[0]);
		read(p1[0], &buf_read, 1);
		close(p1[0]);
		fprintf(1, "%d: received ping\n", getpid());
		write(p2[1], &buf_write, 1);
		close(p2[1]);
		exit(0);
		
    }else {
		char buf_read;
        char buf_write = 'a';   
        close(p1[0]);
        close(p2[1]);     
        write(p1[1], &buf_write, 1);
        close(p1[1]);
        wait(0);
        read(p2[0], &buf_read, 1);
        fprintf(1, "%d: received pong\n", getpid());
        close(p2[0]);
        exit(0);

    }
    return 0;
}