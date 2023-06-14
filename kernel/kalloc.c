// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  char* ref_page; // 用数组记录每个页表的引用次数
  int page_cnt; // 初始化时 记录 free_page 数量
  char* end_;
} kmem;


int
pagecnt(void *pa_start, void *pa_end)
{
  char *p;
  int cnt = 0;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
        ++cnt;
  return cnt;
}

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  kmem.page_cnt = pagecnt(end, (void*)PHYSTOP);

  kmem.ref_page = end ;   
  
  for (int i = 0; i < kmem.page_cnt; ++i)
  {
    kmem.ref_page[i] = 0;
  }
  //end += kmem.page_cnt; // 把end抬起来 给ref_page 留空间 
  kmem.end_  =  kmem.ref_page + kmem.page_cnt;

  freerange(kmem.end_, (void*)PHYSTOP);
}

//
int
 page_index(uint64 pa){
  pa = PGROUNDDOWN(pa);
  int res = (pa - (uint64) kmem.end_) / PGSIZE;
  if(res < 0|| res >= kmem.page_cnt ){
    printf("res: %d , pa: %p, keme.end_: %p",res , pa ,kmem.end_);
    panic("page_index illegal");
  }
  return res;
 }

//页表引用次数增加的函数 ok
void
incr(void *pa){
  acquire(&kmem.lock);
  int index = page_index((uint64)pa);
  kmem.ref_page[index]++;
  release(&kmem.lock);
}
//页表引用次数减少的函数 ok
void
desc(void *pa){
  acquire(&kmem.lock);
  int index = page_index((uint64)pa);
  kmem.ref_page[index]--;
  release(&kmem.lock);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{ 
  int index = page_index((uint64)pa);
  if(kmem.ref_page[index] > 1){
    desc(pa);
    return;
  }
  /*因为释放内存后 引用计数还是 1 ，
    第二次使用的时候 +1 引用计数就是 2
    第二次申请内存后调用一次 kfree就不会释放内存了
  */ 

  if(kmem.ref_page[index] == 1){
   desc(pa);
  }
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r){
    memset((char*)r, 5, PGSIZE); // fill with junk
    incr(r);
  }

  return (void*)r;
}
