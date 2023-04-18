#include <os.h>
#define STACK_SIZE 1024 * 8

void kmt_init() {
    int* p = (int*)pmm->alloc(4);
    *p = 3;
    printf("%d\n", *p);
}

int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg) {
    panic_on(task == NULL, "kmt create error!");
    task->name = name;
    task->entry = entry;
    task->stack = pmm->alloc(STACK_SIZE);
    task->context = kcontext((Area){task->stack, task->stack + STACK_SIZE}, entry, arg);
    return 0;
}

void kmt_teardown(task_t *task) {
    panic_on(task->status != NEVER_SCHEDUlE, "kmt teardown error!");
    pmm->free(task->stack);
}

void kmt_spin_init(spinlock_t *lk, const char *name) {
    panic_on(lk == NULL, "kmt spin init error!");
    lk->name = name;
    lk->lock = UNLOCK;
}



MODULE_DEF(kmt) = {
    .init = kmt_init,
    .create = kmt_create,
    .teardown = kmt_teardown,
    .spin_init = kmt_spin_init,
};
