//$ echo hello too | xargs echo bye
//bye hello too

//$ echo "1\n2" | xargs -n 1 echo line
//line 1
//line 2

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"
#include "kernel/fs.h"

#define MSG_SIZE 16

/* tips :
1  使用fork和exec对每行输入调用命令，在父进程中使用wait等待子进程完成命令。
2  要读取单个输入行，请一次读取一个字符，直到出现换行符（'\n'）。
3  kernel/param.h声明MAXARG，如果需要声明argv数组，这可能很有用。
4  将程序添加到Makefile中的UPROGS。
5  对文件系统的更改会在qemu的运行过程中保持不变；要获得一个干净的文件系统，请运行make clean，然后make qemu
*/
int main(int argc , char*argv){
    // echo hello too | xargs echo bye
    // q1 怎么获取前面一个命令的标准化输出  通过文件描述符号 0：输入  1：输出  进行读取

    sleep(10); // 防止前面 程序执行太慢 导致前面的输出没有
    
    char buf[MSG_SIZE];// 接收前面一个命令的标准输出 字符数组

    read(0 , buf ,MSG_SIZE);//shell确保始终打开三个文件描述符（012），这三个描述符在默认情况下是控制台的文件描述符

    // q2 如何获取到自己的命令行参数 通过 argv 获取
    char* xargv[MAXARG]; //储存 argv 中自己的参数 字符串数组
    int xargc = 0;
    for(int i = 0 ; i  < argc ; ++i){
        xargv[xargc] = argv[i];
        ++xargc; 
    }
    
    char* q = &buf; // 定义一个指针指向 字符数组的第一个字符地址
    
    // q3 如何使用exec去执行命令
    for (int i = 0; i < MSG_SIZE; i++){
        if(buf[i] == '\n'){ // 则换行 调用fork 执行 exec 
            int pid = fork();
            if(pid > 0){ //父进程
                q = &buf[ i + 1 ];
                wait(0); // 等待子进程返回 
            }else{

                xargv[xargc] = q;//xargv[i] 表示的是一个 字符串的地址 即 char*
                ++xargc;
                xargv[xargc] = 0; // 后加入参数后 0 作为结束符
                ++xargc;
                exec(xargv[0],xargv);// xargv[0] 第一个参数代表 xargs 后面的程序文件
                exit(1);
            }
        }
    }
    //wait(0);
    exit(0);
}