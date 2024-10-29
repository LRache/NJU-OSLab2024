typedef struct {
    int status;
    int holder;
} SpinLock;

void spin_lock_init(SpinLock *lk);
void spin_lock(SpinLock *lk);
void spin_unlock(SpinLock *lk);
