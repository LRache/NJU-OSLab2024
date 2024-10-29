#include "am.h"
#include "klib-macros.h"
#include <lock.h>
#include <common.h>
#include ARCH_H

#define UNLOCK 0
#define LOCKED 1

void spin_lock_init(SpinLock *lk) {
    lk->status = UNLOCK;
}

void spin_lock(SpinLock *lk) {
    if (lk->status == LOCKED && lk->holder == cpu_current()) {
        panic("Try to lock.");
    }
    

    while (1) {
        int got = atomic_xchg(&lk->status, LOCKED);
        if (got == UNLOCK) break;
    }
    lk->holder = cpu_current();
}

void spin_unlock(SpinLock *lk) {
    if (lk->status == UNLOCK) {
        panic("Try to unlock a unlocked spin lock.");
    }
    lk->holder = -1;
    atomic_xchg(&lk->status, UNLOCK);
}
