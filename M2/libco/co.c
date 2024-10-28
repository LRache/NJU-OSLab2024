#include "co.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#define STACK_SIZE (8 * 1024 * 1024)

struct co {
    char name[32];

    jmp_buf context;
    bool isRunning;

#ifdef __x86_64__
    uint8_t stack[STACK_SIZE] __attribute__((aligned(16)));
#else
    uint8_t stack[STACK_SIZE] __attribute__((aligned(4)));
#endif

    struct co *next;
    struct co *prev;
};

struct co *head;
struct co *current;

static void append_node(struct co *t) {
    struct co *c = head;
    while (c->next) c = c->next;
    c->next = t;
    t->prev = c;
    t->next = NULL;
}

static void remove_node(struct co *t) {
    if (t->prev != NULL) t->prev->next = t->next;
    if (t->next != NULL) t->next->prev = t->prev;
}

static void switch_context(struct co *to) {
    // printf("Switch to %d\n", to == head);
    if (setjmp(current->context) == 0) {
        current = to;
        longjmp(to->context, 1);
    }
}

static void at_co_exit() {
    assert(current != NULL);
    assert(current != head);
    remove_node(current);
    current->isRunning = false;
    switch_context(head);
}

void switch_and_call_from_main(struct co *next, void (*entry)(void *), void *arg);

void wrapper(void (*entry)(void *), void *arg) {
    entry(arg);
    at_co_exit();
}

struct co *co_start(const char *name, void (*entry)(void *), void *arg) {
    if (head == NULL) {
        head = (struct co *) malloc(sizeof(struct co));
        head->isRunning = true;
    }

    struct co *t = (struct co *)malloc(sizeof(struct co));
    t->isRunning = true;
    append_node(t);
    strcpy(t->name, name);
    if (setjmp(head->context) == 0) {
        switch_and_call_from_main(t, entry, arg);
    }
    
    return t;
}

void co_wait(struct co *co) {
    while (co->isRunning) {
        co_yield();
    }
}

void co_yield() {
    struct co *next = current->next == NULL ? head : current->next;
    switch_context(next);
}

void switch_and_call_from_main(struct co *next, void (*entry)(void *), void *arg) {
    current = next;
    intptr_t stack = (intptr_t)next->stack + STACK_SIZE;

#ifdef __x86_64__
    asm volatile (
        "movq %0, %%rsp; movq %1, %%rdi; movq %2, %%rsi; call *%3; "
        :
        : "b"(stack),
          "d"(entry),
          "c"(arg),
          "a"(wrapper)
        : "memory"
    );
#else
    stack -= 8;
    *(intptr_t *)(stack + 0) = (intptr_t)entry;
    *(intptr_t *)(stack + 4) = (intptr_t)arg;
    asm volatile (
        "movl %0, %%esp; call *%1"
        :
        : "b"(stack),
          "d"(wrapper)
        : "memory"
    );
#endif
}
