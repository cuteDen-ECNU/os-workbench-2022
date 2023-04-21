#include <os.h>
#define STACK_SIZE 1024 * 8
#define MAX_CPU 8

task_t** current_tasks;

task_t* task_head = NULL, *task_tail = NULL;

static void insert(task_t* task){
    panic_on(task == NULL, "insert task null");
    if(task_tail == NULL){
        task_tail = task_head = task;
        task->next = task;
    }
    else{
        task->next = task_tail->next;
        task_tail->next = task;
        task_tail = task;
    }
}

static Context* kmt_context_save(Event ev, Context *context) {
    panic_on(context == NULL, "error context!");
    int cpu = cpu_current();
    if (current_tasks[cpu] == NULL){
        current_tasks[cpu] = task_head;
    }else{
        panic_on(current_tasks[cpu]->status == RUNNING, "current task not running!");
        current_tasks[cpu]->context = context; 
        current_tasks[cpu]->status = RUNNABLE;
    }
    
    return NULL;
}

static Context* kmt_context_schedule(Event ev, Context *context) {
    int cpu = cpu_current();
    task_t* task = current_tasks[cpu];
    bool quit_flag = false;
    while(1){
        current_tasks[cpu] = current_tasks[cpu]->next;
        switch (current_tasks[cpu]->status)
        {
            case RUNNABLE:
                quit_flag = true;
                break;
            default:
                current_tasks[cpu] = current_tasks[cpu]->next; 
                break;
        }
        if(quit_flag == true && current_tasks[cpu] != task){
            current_tasks[cpu]->status = RUNNING;
            break;
        }
    }
    
    return task->context;
}

void kmt_init() {
    
    current_tasks = (task_t**)pmm->alloc(sizeof(task_t*) * cpu_count());
    os->on_irq(sizeof(int) == 4 ? INT32_MIN : INT64_MIN, EVENT_NULL, kmt_context_save);   // 总是最先调用
    os->on_irq(sizeof(int) == 4 ? INT32_MAX : INT64_MAX, EVENT_NULL, kmt_context_schedule);       // 总是最后调用
}

int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg) {
    panic_on(task == NULL, "kmt create error!");
    task->name = name;
    task->entry = entry;
    task->stack = pmm->alloc(STACK_SIZE);
    task->context = kcontext((Area){task->stack, task->stack + STACK_SIZE}, entry, arg);
    insert(task);
    return 0;
}

void kmt_teardown(task_t *task) {
    panic_on(task->status != NEVER_SCHEDUlE, "kmt teardown error!");
    pmm->free(task->stack);
}

void kmt_spin_init(spinlock_t *lk, const char *name) {
    panic_on(lk == NULL, "kmt spin init error!");
    lk->name = name;
    lk->locked = UNLOCK;
}

void kmt_spin_lock(spinlock_t *lk){
    while (atomic_xchg(&lk->locked, LOCK)) {
        yield(); // 切换    
    }
}

void kmt_spin_unlock(spinlock_t *lk){
    atomic_xchg(&lk->locked, UNLOCK);
}

MODULE_DEF(kmt) = {
    .init = kmt_init,
    .create = kmt_create,
    .teardown = kmt_teardown,
    .spin_init = kmt_spin_init,
    .spin_lock = kmt_spin_lock,
};
