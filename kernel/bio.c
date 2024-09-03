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

struct {
  
  struct buf buf[NBUF];
  struct spinlock locks[NBUCKET];
  struct buf buckets[NBUCKET];
} bcache;

void
binit(void)
{
  struct buf *b;

  for(int i = 0; i < NBUCKET; i++)
    initlock(&bcache.locks[i], "bcache");

  // 初始化哈希桶列表
  for(int i = 0; i < NBUCKET; i++) {
    bcache.buckets[i].prev = &bcache.buckets[i];
    bcache.buckets[i].next = &bcache.buckets[i];
  }
  // 将缓存块链接到哈希桶0
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.buckets[0].next;
    b->prev = &bcache.buckets[0];
    initsleeplock(&b->lock, "buffer");
    bcache.buckets[0].next->prev = b;
    bcache.buckets[0].next = b;
  }
}

static uint
Myhash(uint blockno) {
  return blockno % NBUCKET;
}

void
buf_init(struct buf *b, uint dev, uint blockno) {
  b->dev = dev;
  b->blockno = blockno;
  b->valid = 0;
  b->refcnt = 1;  
  return;
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  uint id = Myhash(blockno);
  acquire(&bcache.locks[id]);

  // Is the block already cached?
  for(b = bcache.buckets[id].next; b != &bcache.buckets[id]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.locks[id]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // No cached.
  // 再次遍历一遍哈希桶，如果发现空闲的缓存块，则直接分配一个
  uint mintime = ticks;
  struct buf *minb = 0;
  for(b = bcache.buckets[id].next; b != &bcache.buckets[id]; b = b->next) {
    if(b->refcnt == 0 && b->lastuse <= mintime){
        mintime = b->lastuse;
        minb = b;
    }
  }
  
  if(minb != 0) {
    buf_init(minb, dev, blockno);  
    release(&bcache.locks[id]);
    acquiresleep(&minb->lock);
    return minb;
  }else {
    //缓存已满，从其他哈希桶中挖 buf
    
    for(int i = 0; i < NBUCKET; i++) {
      if(i == id) continue;
      acquire(&bcache.locks[i]);
      mintime = ticks;
      for(b = bcache.buckets[i].next; b != &bcache.buckets[i]; b = b->next){
        if(b->refcnt == 0  && b->lastuse <= mintime) {
          // block_init(b, dev, blockno);
          // b->prev->next = b->next;
          // b->next->prev = b->prev;
          // b->next = bcache.buckets[id].next;
          // b->prev = &bcache.buckets[id];
          // bcache.buckets[id].next->prev = b;
          // bcache.buckets[id].next = b;
          // release(&bcache.locks[i]);
          // release(&bcache.locks[id]);
          // acquiresleep(&b->lock);
          // return b;
          mintime = b->lastuse;
          minb = b;
        }
      }

      if(!minb) {
        release(&bcache.locks[i]);
        continue;
      }
      //初始化缓存块
      buf_init(minb, dev, blockno);
      // 将缓存块从哈希桶中删除
      minb->prev->next = minb->next;
      minb->next->prev = minb->prev;
      release(&bcache.locks[i]);
      // 将缓存块加入到哈希桶id中
      minb->next = bcache.buckets[id].next;
      minb->prev = &bcache.buckets[id];
      bcache.buckets[id].next->prev = minb;
      bcache.buckets[id].next = minb;
      release(&bcache.locks[id]);
      acquiresleep(&minb->lock);
      return minb;
    }
  }
  release(&bcache.locks[id]);
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
// 取消LRU表机制，设置ticks作为最后访问时间
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  uint id = Myhash(b->blockno);
  acquire(&bcache.locks[id]);
  
  b->refcnt--;
  if (b->refcnt == 0) {
    b->lastuse = ticks;
  }
  
  release(&bcache.locks[id]);
}

void
bpin(struct buf *b) {
  uint id = Myhash(b->blockno);
  acquire(&bcache.locks[id]);
  b->refcnt++;
  release(&bcache.locks[id]);
}

void
bunpin(struct buf *b) {
  uint id = Myhash(b->blockno);
  acquire(&bcache.locks[id]);
  b->refcnt--;
  release(&bcache.locks[id]);
}


