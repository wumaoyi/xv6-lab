#include "kernel/types.h"
#include "user/user.h"

#define RD 0 // pipe 文件read端
#define WR 1 // pipe 文件write端

/*
编写一个使用UNIX系统调用的程序来在两个进程之间“ping-pong”一个字节，
*  请使用两个管道，每个方向一个。
1 父进程应该向子进程发送一个字节;
2 子进程应该打印“<pid>: received ping”，其中<pid>是进程ID，并在管道中写入字节发送给父进程，然后退出;
3 父级应该从读取从子进程而来的字节，打印“<pid>: received pong”，然后退出。。
*/
int main(int argc , char const *argv[]){
    char buf = 'p' ; //1 父进程应该向子进程发送一个字节;
    int fd_c2p[2];
    int fd_p2c[2];
    // 使用两个管道
    pipe(fd_c2p);
    pipe(fd_p2c);
    //通过 操作 fd 选取不同管道
    int pid = fork();
    
    int exit_status = 0;
    
    if(pid < 0){
        fprintf(2 , "fork() error!\n");
        close(fd_c2p[RD]);
        close(fd_c2p[WR]);
        close(fd_p2c[RD]);
        close(fd_p2c[WR]);
        exit(0);
    }else if(pid == 0){ // 子进程
        //读取 父进程 写 的字符 
        close(fd_c2p[RD]);
        close(fd_p2c[WR]);
        if(read(fd_p2c[RD] , &buf , sizeof(char)) != sizeof(char)){// read 阻塞先读  再写
            fprintf(2 , "child read() error!\n");
            exit_status = 1; // 标记出错
        }else {
            fprintf(1 , "%d: receive ping\n" ,getpid());
        }
        if(write(fd_c2p[WR] , &buf , sizeof(char)) != sizeof(char)){
            fprintf(2 , "child write() error\n");
            exit_status = 1;
        }
        //子进程 读写端全部关闭
        close(fd_c2p[WR]);
        close(fd_p2c[RD]);
        //关闭子进程
        exit(exit_status);
    }else {
        //父进程
        close(fd_c2p[WR]);
        close(fd_p2c[RD]);

        if(write(fd_p2c[WR] , &buf , sizeof(char)) != sizeof(char)){
            fprintf(2 , "parent write() error!\n");
            exit_status = 1;
        }
        if(read(fd_c2p[RD] , &buf , sizeof(char)) != sizeof(char)){
            fprintf(2 , "parent read()  errro!\n");
            exit_status = 1;
        }else {
            fprintf(1 , "%d: received pong\n" , getpid());
        }
        close(fd_c2p[RD]);
        close(fd_p2c[WR]);
        exit(exit_status);
    }
}