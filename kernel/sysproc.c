#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "date.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;


  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}


#ifdef LAB_PGTBL
int
sys_pgaccess(void)
{
  /*
  kernel/vm.c 中的 walk（） 对于查找正确的 PTE 非常有用。
  您需要在kernel/riscv.h中定义PTE_A，即访问位。请参阅RISC-V手册以确定其值。
  检查是否设置后，请务必清除PTE_A。否则，将无法确定自上次调用 pgaccess（） 以来是否访问了该页面（即，该位将永久设置）。
  vmprint（） 在调试页表时可能会派上用场。
  */
  // lab pgtbl: your code here.
  //todo 
  //1  你需要使用 argaddr（） 和 argint（） 解析参数。
  uint64 addr;
  int len;
  int bitmask;
  if(argaddr(0, &addr) < 0)
    return -1;
  if(argint(1, &len) < 0)
    return -1;
  if(argint(2, &bitmask) < 0)
    return -1;

//  可以设置可以扫描的页数上限。    
  if(len > 32 || len <0){
    return -1;
  }  
  struct proc *p = myproc();//获取当前进程   
  int res = 0;  
  for(int i = 0 ; i < len ; ++i){
    uint64 va = addr + i*PGSIZE;
    int abit = vm_pgaccess(p->pagetable , va);
    res = res | (abit << i);
  }
  // 2 对于输出位掩码，在内核中存储临时缓冲区并在用正确的位填充后将其复制给用户（通过 copyout（））会更容易。

  if(copyout(p->pagetable , bitmask , (char*)&res,sizeof(res)) < 0){ // 从kernel复制数据给user
    return -1;
  }  
  return 0;
}
#endif

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
