/*使用管道编写prime sieve(筛选素数)的并发版本。这个想法是由Unix管道的发明者Doug McIlroy提出的。
请查看这个网站(翻译在下面)，该网页中间的图片和周围的文字解释了如何做到这一点。您的解决方案应该在user/primes.c文件中。

1 请仔细关闭进程不需要的文件描述符，否则您的程序将在第一个进程达到35之前就会导致xv6系统资源不足。

2 一旦第一个进程达到35，它应该使用wait等待整个管道终止，包括所有子孙进程等等。因此，主primes进程应该只在打印完所有输出之后，并且在所有其他primes进程退出之后退出。

3 提示：当管道的write端关闭时，read返回零。

4 最简单的方法是直接将32位（4字节）int写入管道，而不是使用格式化的ASCII I/O。

5 您应该仅在需要时在管线中创建进程。

6 将程序添加到Makefile中的UPROGS
*/

#include "kernel/types.h"
#include "user/user.h"

#define RD 0
#define WR 1

const uint INT_LEN = sizeof(int);
// const uint INT_LEN = sizeof(int); uint 无符号


void transmit_data(int fd[2] , int fd_2[2] , int first){
    int data;
    //除余筛选
    while(read(fd[RD],&data,sizeof(int))==sizeof(int)){
        if(data % first){
            write(fd_2[WR], &data , sizeof(int));
        }
    }
    close(fd[RD]);
    close(fd_2[WR]);
}
// 把 第一个数 读进 first 并记录 放到 下一层进行 除法筛选 
int lpipe_first_data(int fd[2] , int* first){
    if(read(fd[RD],first , sizeof(int)) == sizeof(int)){
        printf("prime %d\n",*first);
        return 1;
    }
    return 0;
} 


void primes(int fd[2]){
    close(fd[WR]); // 关闭子进程的 写端口

    int first; // 找到第一个数并打印 写一个打印第一个素数的方法
    if(lpipe_first_data(fd , &first)){
        // 第一个素数 打印成功
        // 转移剩下的数 给下一个管道 需要fork
        int fd_2[2];
        pipe(fd_2);// 当前管道
        // 转移 筛选
        transmit_data(fd , fd_2 , first);
        int pid =  fork();
         if(pid == 0){
            primes(fd_2);
        }else {
            close(fd_2[RD]);
      wait(0);
        }
    }else {
        close(fd[RD]); // 关闭子进程的 读端
        // 打印完毕  
        wait(0);
    }

}

int main(int argc , char const* argv[]){
    int fd[2];
    // 创建管道文件 fd 控制读写开关端口
    pipe(fd);
    //写入 2 - 35 的数到文件里 然后进行筛选
    for(int i = 2  ; i <= 35 ; ++i ){
        write(fd[WR] , &i , INT_LEN);

        int pid = fork();
        if(pid == 0){//子进程 
            // 进行素数筛选 
            primes(fd);
        }else {
            // 父进程不需要读写
            close(fd[WR]);
            close(fd[RD]);

            wait(0);//参数是时间  子进程结束立即返回
        }
    }
     exit(0);
}
