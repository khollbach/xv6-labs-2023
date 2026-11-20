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
} kmem;

#define RAMBASE 0x80000000

int refcounts[(PHYSTOP - RAMBASE) / PGSIZE]; // (This should be an upper bound.)

int *get_rc(void *pa) {
  return &refcounts[((uint64)pa - RAMBASE) / PGSIZE];
}

void
kinit()
{
  initlock(&kmem.lock, "kmem");
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

// Free the page of physical memory pointed at by pa,
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

  if (*get_rc(r) > 1) {
    return; // no-op
  }

  *get_rc(r) = 0; // set refcount to 0 as an invalid value

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Takes a page that was previously kalloc'd. Increments the refcount.
void inc_rc(void *pa) {
  (*get_rc(pa))++;
}

// Panic if the page already had one refcount.
void dec_rc(void *pa) {
  (*get_rc(pa))--;

  if (*get_rc(pa) <= 0) {
    panic("dec_rc pa");
  }
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

  if(r) {
    if (*get_rc(r) != 0) {
      panic("kalloc: expected refcount to be 0");
    }
    memset((char*)r, 5, PGSIZE); // fill with junk
    *get_rc(r) = 1; // init refcount
  }
  return (void*)r;
}
