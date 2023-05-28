#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sysinfo.h"

uint64 acquire_freemen();
uint64 accquire_nproc();

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


/*uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}
*/
uint64 sys_trace(void){
  int mask;//参考上面 n 是接收用户态 传入的参数 掩码：指要追踪的系统调用
  if(argint(0, &mask) < 0)// 将参数 存到 proc 进程结构体中 从a0寄存器中取出  第一个参数赋值给mask 
    return -1;
  struct proc* p = myproc();
  p->trace_mask = mask;
  return 0;
}

uint64 sys_sysinfo(void){
  struct sysinfo info;

  struct proc*  p = myproc();

  uint64 addr;//指向 info 的地址

  info.freemem = acquire_freemen();//空闲字节数
  info.nproc =  accquire_nproc();//使用的进程数量
  if(argaddr(0, &addr) < 0)//获取第一个参数 写入&st
    return -1;

  if(copyout(p->pagetable, addr, (char *)&info, sizeof(info)) < 0)//将 进程 p 的 info 信息复制给 addr
      return -1;
  return 0;
}