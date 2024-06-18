#include "kernel/param.h"
#include "user/user.h"

#define MAX 512

int main(int argc, char *argv[]) {
    if(argc < 2) {
        fprintf(2, "usage: xargs command\n");
        exit(1);
    }
    char *cmd[MAXARG];
    int index = 0;
    for(int i = 1; i < argc; i++) {
        cmd[index++] = argv[i];
    }
    char buf[MAX];
    int n = read(0, buf, sizeof(buf));
    char temp[MAX];
    
    int k = 0;
    for(int i = 0; i < n; i++) {
        if(buf[i] != '\n') {
            temp[k++] = buf[i];
        }else {
            temp[k] = 0;
			cmd[index] = temp;
            if(fork() == 0) {
                exec(argv[1], cmd);
                exit(0);
            }else {
                wait(0);
                k = 0;
                memset(temp, 0, sizeof(temp));
            }
        }        
    }
    exit(0);
}