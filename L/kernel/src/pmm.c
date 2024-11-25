#include <am.h>
#include <klib-macros.h>
#include <klib.h>
#include <lock.h>
#include <kernel.h>
#include <stddef.h>
#include <stdint.h>

#define HEAP_START ((const uintptr_t)heap.start)
#define HEAP_END   ((const uintptr_t)heap.end  )

#define SEG_LENGTH (5 + 20)
#define SEG_SIZE   (1 << SEG_LENGTH)
#define BITMAP_SIZE (1 << (SEG_LENGTH - 3 - 3 + 1))

static SpinLock lock;

static uint8_t bitmap[4][BITMAP_SIZE]; // 256 kb per segment bitmap for heap

static int is_avaliable_helper(size_t heapIndex, size_t parent, int order) {
    size_t offset = parent * 2 + order;
    if ((offset >> 3) >= BITMAP_SIZE) return 1;
    size_t blockOffset = offset >> 3;
    size_t innerOffset = offset & 0b111;
    uint8_t mask = (1 << innerOffset);
    int avaliable;
    avaliable = (bitmap[heapIndex][blockOffset] & mask) == 0;
    if (!avaliable) return 0;
    avaliable = is_avaliable_helper(heapIndex, offset, 1);
    if (!avaliable) return 0;
    return is_avaliable_helper(heapIndex, offset, 2);
}

static int is_avaliable_block(size_t heapIndex, size_t parent, int order) {
    size_t offset = parent * 2 + order;
    size_t blockOffset = offset >> 3;
    size_t innerOffset = offset & 0b111;
    uint8_t mask = (1 << innerOffset);
    spin_lock(&lock);
    int avaliable = (bitmap[heapIndex][blockOffset] & mask) == 0;
    spin_unlock(&lock);
    return avaliable;
}

static int alloc_if_avaliable(size_t heapIndex, size_t parent, int order) {
    size_t offset = parent * 2 + order;
    size_t blockOffset = offset >> 3;
    size_t innerOffset = offset & 0b111;
    uint8_t mask = (1 << innerOffset);
    assert(blockOffset < BITMAP_SIZE);
    spin_lock(&lock);
    int avaliable = is_avaliable_helper(heapIndex, parent, order);
    if (avaliable) {
        bitmap[heapIndex][blockOffset] |= mask;
    }
    spin_unlock(&lock);
    return avaliable;
}

static void *binary_find_and_alloc(size_t allocLength, size_t heapIndex, size_t parent, int order, uintptr_t start, size_t length) {
    if (length < 3) return NULL;
    assert(length <= SEG_LENGTH);
    assert(length >= 3);
    assert(length >= allocLength);
    assert((start & 0b111) == 0);
    assert(start >= HEAP_START);
    
    if (start + (1 << allocLength) > HEAP_END) {
        return NULL;
    }
    if (!is_avaliable_block(heapIndex, parent, order)) {
        return NULL;
    }
    
    if (allocLength == length) {
        int avalible = alloc_if_avaliable(heapIndex, parent, order);
        return avalible ? (void *)start : NULL;
    }
    
    length--;
    if (length < 3) return NULL;
    if (length < allocLength) return NULL;
    
    void *p = NULL;
    p = binary_find_and_alloc(allocLength, heapIndex, parent * 2 + order, 1, start, length);
    if (p != NULL) return p;
    p = binary_find_and_alloc(allocLength, heapIndex, parent * 2 + order, 2, start + (1 << length), length);
    return p;
}

static void *kalloc(size_t size) {
    if (size > (1 << (SEG_LENGTH - 1))) return NULL; // reject the alloc requirment more than 16MiB
    size_t allocLength = 0;
    if (size <= 8) {
        allocLength = 3;
    } else {
        size_t allocSize = 8;
        allocLength = 3;
        while (allocSize < size) {
            allocSize <<= 1;
            allocLength ++;
        }
    }

    void *p = NULL;
    int heapIndex = 0;
    for (uintptr_t start = HEAP_START; start <= HEAP_END; start += SEG_SIZE) {
        p = binary_find_and_alloc(allocLength, heapIndex, 0, 0, start, SEG_LENGTH);
        if (p != NULL) break;
        heapIndex++;
    }

    return p;
}

static void kfree(void *ptr) {
    if (ptr == NULL) return ;
    
    uintptr_t p = (uintptr_t)ptr;
    assert((p & 0b111) == 0);
    assert(p >= HEAP_START);
    assert(p <  HEAP_END  );
    size_t offset = (((p - HEAP_START) & (SEG_SIZE - 1)) >> 3) + (1 << (SEG_LENGTH - 3)) - 1;
    size_t heapIndex = (p >> SEG_LENGTH) & 0b11;

    spin_lock(&lock);
    while (1) {
        size_t blockOffset = offset >> 3;
        size_t innerOffset = offset & 0b111;
        uint8_t mask = (1 << innerOffset);
        if ((bitmap[heapIndex][blockOffset] & mask) != 0) {
            bitmap[heapIndex][blockOffset] &= ~mask;
            break;
        }
        assert((offset & 1) == 1);  // invalid pointer to release;
        offset >>= 1;
    }
    spin_unlock(&lock);
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

    spin_lock_init(&lock, "PMM");
    
    size_t heapSize = pmsize;
    size_t heapLength = 0;
    while (heapSize) {
        heapLength ++;
        heapSize >>= 1;
    }

    if (heapLength > SEG_LENGTH + 3 + 3) {
        printf("heapLength = %u", heapLength);
        panic("Heap is too large");
    }
    printf("bitmap@%p\n", &bitmap);
}

MODULE_DEF(pmm) = {
    .init  = pmm_init,
    .alloc = kalloc,
    .free  = kfree,
};
