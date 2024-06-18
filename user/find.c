#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find(char *dirname, char *filename) {
    // 定义文件缓冲区和指针
    char buf[512], *p;
    // 定义文件描述符
    int fd;
    struct dirent de;
    struct stat st;

    if((fd = open(dirname, 0)) < 0){
        fprintf(2, "ls: cannot open %s\n", dirname);
        return;
    }
    //通过fstat函数文件描述符获取文件信息
    if(fstat(fd, &st) < 0){
        fprintf(2, "ls: cannot stat %s\n", dirname);
        close(fd);
        return;
    }

    if(st.type != T_DIR) {
        fprintf(2, "find: %s is not a directory\n", dirname);
        return;
    }
    
    if(strlen(dirname) + 1 + DIRSIZ + 1 > sizeof buf){
      fprintf(2, "ls: path too long\n");
      return;
    }

    strcpy(buf, dirname);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
        if(de.inum == 0)
            continue;
        memmove(p, de.name, DIRSIZ);
        // 文件名后面加上\0
        p[DIRSIZ] = 0;
        //不要递归 "." 和 ".." 
        if(strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) {
            continue;
        }
        // 无法读取文件信息, 通过stat函数文件名获取文件信息
        if(stat(buf, &st) < 0) {
            fprintf(2, "ls: cannot stat %s\n", buf);
            continue;
        }
        // 如果是目录，递归查找
        if(st.type == T_DIR) {
            find(buf, filename);
        }
        // 如果是文件，判断是否是要找的文件
        if(strcmp(de.name, filename) == 0) {
            printf("%s\n", buf);
        }
    }
}



int main(int argc, char *argv[]) {
    if(argc != 3) {
        fprintf(2, "Usage: find <dirname> <filename>");
        exit(1);
    } 
    char *dirname = argv[1];
    char *filename = argv[2];
    find(dirname, filename);
    exit(0);
}