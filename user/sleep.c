#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc , char* argv[]){
    if(argc <= 1){
        fprintf(2 , "usage sleep second\n");
        exit(1);
    }
    sleep(atoi(argv[0]));
    exit(0);
}