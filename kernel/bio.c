// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

// struct {
// struct spinlock lock;
// struct buf buf[NBUF];

//   // Linked list of all buffers, through prev/next.
//   // Sorted by how recently the buffer was used.
//   // head.next is most recent, head.prev is least.
//   struct buf head;
// } bcache;

#define NBUCKET 13 //定义散列桶数量
#define HASH(id) (id %  NBUCKET) // 放入桶id

struct hashbuf {
  struct buf head;      //头节点
  struct spinlock lock; //锁
};

struct {
    struct buf buf[NBUF];
    struct hashbuf buckets[NBUCKET];
} bcache;//block 缓存 防止频繁操作io读写磁盘


void
binit(void)
{
  struct buf *b;
  char lockname[16];

  // initlock(&bcache.lock, "bcache");

  // Create linked list of buffers
  // bcache.head.prev = &bcache.head;
  // bcache.head.next = &bcache.head;
  for(int i = 0 ; i < NBUCKET ; ++i){
    snprintf(lockname,sizeof(lockname),"bcache_%d",i);
    initlock(&bcache.buckets[i].lock,lockname); //初始化每个锁

    //将散列桶head的prev next 指向自身 表示为空
    bcache.buckets[i].head.prev = & bcache.buckets[i].head;
    bcache.buckets[i].head.next = & bcache.buckets[i].head;
  }

  // Create linked list of buffers
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    //利用头插法 先将缓存块 全部放在第一个桶里
    b->next = bcache.buckets[0].head.next;
    b->prev = &bcache.buckets[0].head;
    initsleeplock(&b->lock, "buffer");
    bcache.buckets[0].head.next->prev = b;
    bcache.buckets[0].head.next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  //通过blockno 找到桶 id
  int bid = HASH(blockno);

  acquire(&bcache.buckets[bid].lock);

  // Is the block already cached?
  for(b = bcache.buckets[bid].head.next; b != &bcache.buckets[bid].head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      //记录时间戳
      acquire(&tickslock);
      b->timestamp = ticks;
      release(&tickslock);

      release(&bcache.buckets[bid].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  // for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
  //   if(b->refcnt == 0) {
  //     b->dev = dev;
  //     b->blockno = blockno;
  //     b->valid = 0;
  //     b->refcnt = 1;
  //     release(&bcache.lock);
  //     acquiresleep(&b->lock);
  //     return b;
  //   }
  // }

  //没找到 遍历所有篮子
  b = 0;
  struct buf* tmp;

  for(int i = bid , cycle  = 0 ; cycle != NBUCKET ; i = (i+1)%NBUCKET){
    ++cycle;
    // 如果遍历到当前桶 前面已经 申请过锁了 就跳过
    if(i == bid) continue;
    if(!holding(&bcache.buckets[i].lock))acquire(&bcache.buckets[i].lock);
    
    //循环找最小
    for(tmp = bcache.buckets[i].head.next ; tmp != bcache.buckets[i].head ; tmp = tmp->next){
      //使用时间戳LRU算法 而不是根据节点在链表中的位置 遍历一遍 取可用的 切 b->timestamp最小的
      if(tmp->refcnt == 0 && (b == 0 || tmp ->timestamp )) b = tmp;
    }

    // 插入桶
    if(b){
      //如果是其他桶里窃取的 将其头插法放入
      if(i != bid){
        //先从i桶 链表删除
        b->next->prev = b->prev;
        b->prev->next = b->next;
        release(&bcache.buckets[i].lock);
        //插入bid 桶
        b->next = bcache.buckets[bid].head.next;
        b->prev = &bcache.buckets[bid].head;
        bcache.buckets[bid].head.next->prev = b;
        bcache.buckets[bid].head.next = b;
      }
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;

      //记录时间戳
      acquire(&tickslock);
      b->timestamp = ticks;
      
      release(&tickslock);
      acquiresleep(&b->lock);
      release(&bcache.buckets[bid].lock);

      return b;
    }else{
      // 在当前桶里未找到 释放当前的锁 bid 的锁要找到可用后释放
      if(i != bid) release(&bcache.buckets[i].lock);
    } 
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
// 更改后 不再获取全局锁 获取桶里的锁
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  //获取 bcache 所在桶号
  int bid = HASH(b->blockno);
  releasesleep(&b->lock);

  acquire(&bcache.buckets[bid].lock);
  b->refcnt--;

  acquire(&tickslock);
  b->timestamp = ticks;
  release(&tickslock);
  //不需要头插法 使用时间戳
  // if (b->refcnt == 0) {
  //   // no one is waiting for it.
  //   b->next->prev = b->prev;
  //   b->prev->next = b->next;
  //   b->next = bcache.head.next;
  //   b->prev = &bcache.head;
  //   bcache.head.next->prev = b;
  //   bcache.head.next = b;
  // }
  
  release(&bcache.buckets[bid].lock);
}

void
bpin(struct buf *b) {
  int bid = HASH(b->blockno);
  acquire(&bcache.buckets[bid].lock);
  b->refcnt++;
  release(&bcache.buckets[bid].lock);
}

void
bunpin(struct buf *b) {
  int bid = HASH(b->blockno);
  acquire(&bcache.buckets[bid].lock);
  b->refcnt--;
  release(&bcache.buckets[bid].lock);
}


