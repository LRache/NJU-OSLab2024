#include "am.h"
#include <common.h>
#include <lock.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

SpinLock lock;

static void os_init() {
    pmm->init();
    spin_lock_init(&lock, "OSRun");
}

static void os_run() {
    printf("Hello World from CPU #%d\n", cpu_current());
    void *array[1000] = {};
    for (int i = 0; i < 20; i++) {
        size_t s = i + 4;
        void *p = pmm->alloc(s);
        spin_lock(&lock);
        printf("%d %d alloc %p %p\n", cpu_current(), i, p, p + s);
        spin_unlock(&lock);
        array[i] = p;
    }
    for (int i = 0; i < 20; i++) {
        void *p = array[i];
        spin_lock(&lock);
        printf("%d %d free %p\n", cpu_current(), i, p);
        spin_unlock(&lock);
        pmm->free(p);
    }
    while (1) ;
}

MODULE_DEF(os) = {
    .init = os_init,
    .run  = os_run,
};
