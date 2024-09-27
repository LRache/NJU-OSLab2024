#include <stdio.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <dirent.h>
#include <ctype.h>
#include <string.h>

typedef struct Process
{
   int pid;
   int parent;
   char name[64];
   struct Process *next;
   struct Process *children;
   struct Process *treePrev;
   struct Process *treeNext;
   int *tid;
   int tidCount;
} Process;

Process *header = NULL;
Process *top = NULL;

void show_version() {
    printf("This is my own pstree ");
    #if defined(__x86_64__) || defined(__ppc64__)
        printf("for 64-bit");
    #elif defined(__i386__) || defined(__ppc__)
        printf("for 32-bit");
    #endif
    printf(".\n");
}

void error(const char *message) {
    fprintf(stderr, "%s\n", message);
    exit(1);
}

static inline int get_parent(int pid) {
    char path[64];
    sprintf(path, "/proc/%d/stat", pid);
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        return -1;
    }
    int parent;
    fscanf(f, "%*s %*s %*s %d", &parent);
    fclose(f);
    return parent;
}

static inline int get_pname(int pid, char *name) {
    char path[64];
    sprintf(path, "/proc/%d/comm", pid);
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        return -1;
    }
    fscanf(f, "%s", name);
    fclose(f);
    return 0;
}

static inline int is_kernel(int pid) {
    char path[64];
    sprintf(path, "/proc/%d/status", pid);
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        return -1;
    }
    int kthread = 0;
    while (!feof(f)) {
        char buffer[128];
        fgets(buffer, sizeof(buffer), f);
        char key[24];
        sscanf(buffer, "%s", key);
        if (strcmp(key, "Kthread:") == 0) {
            sscanf(buffer, "%*s %d", &kthread);
            break;
        }
    }
    fclose(f);
    return kthread;
}

int walk_thread(int pid, int **tid) {
    char path[80];
    sprintf(path, "/proc/%d/task", pid);
    DIR *d = opendir(path);
    if (d == NULL) return -1;

    int cnt = 0;
    int limit = 4;
    while (true) {
        struct dirent *entry = readdir(d);
        if (entry == NULL) break;
        
        if (!isdigit(entry->d_name[0])) continue;
        int t = atoi(entry->d_name);
        if (t == pid) continue;
        
        cnt++;
        if (*tid == NULL) {
            *tid = malloc(sizeof(int) * 4);
        } else if (cnt == limit) {
            *tid = realloc(*tid, sizeof(int) * (limit * 2));
            limit *= 2;
        }
        *(*tid + cnt - 1) = t;
    }
    closedir(d);
    return cnt;
}

void walk_process() {
    DIR *dir = opendir("/proc");
    if (dir == NULL) error("Failed to open /proc");
    
    Process *next = NULL;
    while (true) {
        struct dirent *entry = readdir(dir);
        if (entry == NULL) break;
        char *pids = entry->d_name;
        if (isdigit(pids[0])) {
            int pid = atoi(pids);
            int parent = get_parent(pid);
            char pname[64];
            if (parent == -1) continue;
            if (get_pname(pid, pname) == -1) continue;
            int kthread = is_kernel(pid);
            if (kthread) continue;
            int *tid = NULL;
            int tidCount = walk_thread(pid, &tid);
            if (tidCount == -1) continue;

            Process *p = (Process *)malloc(sizeof(Process));
            memset(p, 0, sizeof(Process));
            strcpy(p->name, pname);
            p->pid    = pid;
            p->parent = parent;
            p->tid    = tid;
            p->tidCount = tidCount;
            
            if (header == NULL) {
                header = p;
                next = header;
            } else {
                next->next = p;
                next = next->next;
            }
            next->next = NULL;
        }
    }
    closedir(dir);
}

void build_tree() {
    Process *c = header;
    while (c) {
        if (c->parent == 0) {
            if (top == NULL) {
                top = c;
            } else {
                top->treeNext = c;
            }
            c->treeNext = NULL;
        } else {
            Process *p = header;
            while (p) {
                if (p->pid == c->parent) {
                    Process *t = p->children;
                    if (t) {
                        while (t->treeNext) t = t->treeNext;
                        t->treeNext = c;
                        c->treePrev = t;
                    } else {
                        p->children = c;
                        c->treePrev = NULL;
                    }
                    c->treeNext = NULL;
                    break;
                }
                p = p->next;
            }
        }
        c = c->next;
    }
}

bool showPid = false;

inline int print_name(const Process *p) {
    if (showPid) {
        int digitCount = 0, t = 1;
        while (t < p->pid) {t *= 10; digitCount++;}
        
        printf("%s(%d)", p->name, p->pid);
        return strlen(p->name) + 2 + digitCount;
    } else {
        printf("%s", p->name);
        return strlen(p->name);
    }
}

static int space[20];
static bool tail[20];

static int cmp_pid(const Process *p1, const Process *p2) {
    return p1->pid < p2->pid;
}

static int cmp_name(const Process *p1, const Process *p2) {
    return strcmp(p1->name, p2->name) < 0;
}

int (*cmp)(const Process *, const Process *) = cmp_name;

static inline void insert_sorted(Process **head, Process *newProcess) {
    Process* current;
    if (*head == NULL || cmp(newProcess, *head)) {
        newProcess->treeNext = *head;
        newProcess->treePrev = NULL;
        if (*head != NULL)
            (*head)->treePrev = newProcess;
        *head = newProcess;
    } else {
        current = *head;
        while (current->treeNext != NULL && cmp(current->treeNext, newProcess)) {
            current = current->treeNext;
        }
        newProcess->treeNext = current->treeNext;
        if (current->treeNext != NULL)
            current->treeNext->treePrev = newProcess;
        current->treeNext = newProcess;
        newProcess->treePrev = current;
    }
}

static inline Process *sort_list(Process *head) {
    Process *sorted = NULL;
    Process *current = head;

    while (current != NULL) {
        Process* next = current->treeNext;
        insert_sorted(&sorted, current);
        current = next;
    }
    return sorted;
}

static inline void print_space(int level) {
    for (int i = 0; i < level; i++) {
        for (int j = 0; j < space[i]; j++) putchar(' ');
        printf(tail[i] ? " " : "│");
    }
    for (int i = 0; i < space[level]; i++) putchar(' ');
}

void print_children(Process *p, int level) {
    bool firstLine = true;
    Process *c = p->children;
    c = sort_list(c);
    while (c) {
        bool isTail = c->treeNext == NULL && p->tidCount == 0;
        if (firstLine) {
            firstLine = false;
            printf(isTail ? "──" : "─┬");
        } else {
            print_space(level);
            if (isTail) {
                printf("└");
            } else {
                printf("├");
            }
        }
        printf("─");
        int length = print_name(c);
        space[level + 1] = 2 + length;
        tail[level] = isTail;
        print_children(c, level + 1);
        space[level + 1] = 0;

        c = c->treeNext;
    }
    if (p->tidCount != 0) {
        if (showPid) {
            for (int i = 0; i < p->tidCount; i++) {
                if (firstLine) {
                    firstLine = false;
                    printf(i == p->tidCount - 1 ? "──" : "─┬");
                } else {
                    print_space(level);
                    printf(i == p->tidCount - 1 ? "└" : "├");
                }
                printf("{%s}(%d)\n", p->name, p->tid[i]);
            }
        } else { 
            if (firstLine) printf("──");
            else print_space(level);
            if (firstLine) {
                firstLine = false;
                printf("──");
            } else printf("└─");
            if (p->tidCount == 1) printf("{%s}\n", p->name);
            else printf("%d*[{%s}]\n", p->tidCount, p->name);
        }
    }
    if (p->children == NULL && p->tidCount == 0) putchar('\n');
}

void print_process() {
    Process *t = top;
    while (t) {
        char name[80];
        if (showPid) {
            snprintf(name, 80, "%s(%d)", t->name, t->pid);
        } else {
            strcpy(name, t->name);
        }
        printf("%s─", name);
        space[0] = 2 + strlen(name);
        print_children(t, 0);
        t = t->treeNext;
    }
}

void free_process() {
    Process *c = header;
    while (c) {
        Process *t = c->next;
        free(c->tid);
        free(c);
        c = t;
    }
}

int main(int argc, char **argv) {
    static struct option options[] = {
        {"show_pids",       no_argument, NULL, 'p'},
        {"numeric-sort",    no_argument, NULL, 'n'},
        {"verion",          no_argument, NULL, 'V'},
        {0, 0, 0, 0},
    };
    char c;
    while ((c = getopt_long(argc, argv, "pnV", options, NULL)) != -1) {
        switch (c)
        {
            case 'V': show_version(); return 0; break;
            case 'p': showPid = true; break;
            case 'n': cmp = cmp_pid; break;
        }
    }
    walk_process();
    build_tree();
    print_process();
    free_process();
    return 0;
}
