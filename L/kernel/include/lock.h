typedef struct {
    int status;
    int holder;
    const char *name;
} SpinLock;

void spin_lock_init(SpinLock *lk, const char *name);
void spin_lock(SpinLock *lk);
void spin_unlock(SpinLock *lk);
