#include "types.h"
#include "riscv.h"
#include "memlayout.h"
#include "spinlock.h"
#include "defs.h"

struct cow
{
    uint cow_ref;
    struct spinlock lock;
}cow[((PHYSTOP - KERNBASE) / PGSIZE)];


void 
cow_lockinit(void) {
    for (int i = 0; i < ((PHYSTOP - KERNBASE) / PGSIZE); i++) {
        initlock(&cow[i].lock, "cow");
    }
}

uint8
dec_cowref(uint64 pa) {
    struct cow *c = &cow[(pa - KERNBASE) / PGSIZE];
    acquire(&c->lock);
    uint8 ref = --c->cow_ref;
    release(&c->lock);
    return ref;
}

void
inc_cowref(uint64 pa) {
    
    struct cow *c = &cow[(pa - KERNBASE) / PGSIZE];
    acquire(&c->lock);
    ++c->cow_ref;
    release(&c->lock);
    return;
}
