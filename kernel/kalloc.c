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
  struct run *next; // 将内存分为一块一块的，直到没有内存为止
};

struct {
  struct spinlock lock;
  struct run *freelist; // 指向可用（空）内存的链表
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");  // 初始化spinlock
  freerange(end, (void*)PHYSTOP); 
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
  // 使用锁 (kmem.lock) 来确保对空闲内存列表的访问是线程安全的
  // 获取锁 (acquire(&kmem.lock))，以保证对 kmem.freelist 的独占访问
  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  // 释放锁 (release(&kmem.lock))，以允许其他线程访问 kmem.freelist
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

uint64
get_frame() {
  uint64 ret = 0;
  acquire(&kmem.lock);
  struct run *pagelist = kmem.freelist;
  while (pagelist)
  {
    pagelist = pagelist->next;
    ret++;
  }
  release(&kmem.lock);
  return ret * PGSIZE;
}