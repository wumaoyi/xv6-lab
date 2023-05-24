#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char* fmtname(char *path){ // 获取文件名  path 是 ./文件名 
  static char buf[DIRSIZ+1]; // 存放
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), 0, DIRSIZ-strlen(p));// 设置 0 是结束符
  return buf;
}

//是否能递归 是文件夹则可以递归
int norecurse(char* path){//. 和 ..不递归
    char* buf = fmtname(path);
    if(buf[0] == '.' && buf[1] == 0){
      return 0 ; //不能递归
    }
    if(buf[0] == '.' && buf[1] == '.'&& buf[2]==0){
      return 0 ; //不能递归
    }
    return 1 ;
}

void find(char *path , const char * target){
  char buf[512], *p; // buf字符数组 装 path 传进来的参数
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, 0)) < 0){
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){ // 把文件 的 状态写入st
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }
  //  进行比较 target 
  if(strcmp(fmtname(path) , target) == 0){ // 找到符合的
     printf("%s\n" , path);
  } 

  switch(st.type ){

  case T_DIR: // 是文件夹
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("find: path too long\n");
      break;
    }

    strcpy(buf, path);//buf 装载 传入的 path 参数

    p = buf+strlen(buf);
    *p++ = '/';

    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf("ls: cannot stat %s\n", buf);
        continue;
      }
      if(norecurse(buf) ){
        find(buf , target);
      }
    }
    break;
  }
  close(fd);
}

int main(int argc, char *argv[]){
  if(argc == 1){
    fprintf(2, "usage: find <directory> <filename>\n");
    exit(1);
  }
  if(argc == 2){
    find(".",argv[1]);
    exit(0);
  } 
  if(argc == 3){
    printf("ok\n");
    find(argv[0],argv[1]);
    exit(0);
  } 
  if(argc > 3) {
    fprintf(2, "usage: find <directory> <filename>\n");
    exit(1);
  }
  exit(0);
}