#include "am.h"
#include <common.h>
#include <lock.h>
#include <stddef.h>
#include <stdint.h>

typedef size_t Header;
typedef size_t Foot;


static SpinLock lock;
static uintptr_t top;

static void *kalloc(size_t size) {
    if (size >= (1 >> 24)) return NULL; // reject the alloc requirment more than 16MiB
    

    return NULL;
}

static void kfree(void *ptr) {
    // TODO
    // You can add more .c files to the repo.
}

static void pmm_init() {
    uintptr_t pmsize = (
        (uintptr_t)heap.end
        - (uintptr_t)heap.start
    );

    printf(
        "Got %d MiB heap: [%p, %p)\n",
        pmsize >> 20, heap.start, heap.end
    );

    spin_lock_init(&lock);
    top = (uintptr_t)heap.start;
    size_t heapSize = (uintptr_t)heap.end - (uintptr_t)heap.start;
    size_t freeSize = heapSize - sizeof(Header) - sizeof(Foot);
    *(size_t *)heap.start = freeSize & (~0x3);
    *(size_t *)(heap.end - sizeof(Foot)) = (freeSize & (~0x3)) | 0b10; 
}

MODULE_DEF(pmm) = {
    .init  = pmm_init,
    .alloc = kalloc,
    .free  = kfree,
};
