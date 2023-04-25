#include <os.h>
#define STACK_SIZE 1024 * 8
#define MAX_CPU 8

task_t** current_tasks;

task_t* task_list = NULL;

static void print_task(task_t* head) {
#ifdef VERBOSE_PRINT_TASK
    task_t* task = head;
    if(task == NULL){
        printf("null");
    }else{
        printf("%s%d", task->name, task->status);
        while(task->fd != head){
            task = task->fd;
            printf("-> %s%d", task->name, task->status);
        }
        printf("\n");
    }
#endif
}

static void insert(task_t** head, task_t* task){
    TRACE_ENTRY;
    panic_on(task == NULL, "insert task null");
    if(*head == NULL){
        *head = task;
        (*head)->bk = task;
        (*head)->fd = task;
    }
    else{
        task->fd = *head;
        task->bk = (*head)->bk;
        (*head)->bk = task;
        task->bk->fd = task;
    }
    TRACE_EXIT;
}

static void remove(task_t** head, task_t* task){
    TRACE_ENTRY;
    panic_on(task == NULL, "remove task null"); 
    if((*head)->bk == *head){
        panic_on((*head)->fd != *head, "error task_list!");
        panic_on(task != *head, "error task_list!");
        *head = NULL;
    }
    else{
        if (*head == task){
            *head = task->fd;
        }
        task->fd->bk = task->bk;
        task->bk->fd = task->fd;
        task->fd = NULL;
        task->bk = NULL;
    }
    TRACE_EXIT;
}

static Context* kmt_context_save(Event ev, Context *context) {
    TRACE_ENTRY;
    panic_on(context == NULL, "error context!");
    int cpu = cpu_current();
    if (current_tasks[cpu] == NULL){
        current_tasks[cpu] = task_list;
    }else{
        current_tasks[cpu]->context = context; 
        if (current_tasks[cpu]->status == BLOCKED) 
            current_tasks[cpu] = task_list; 
        else if (current_tasks[cpu]->status == RUNNING)
            current_tasks[cpu]->status = RUNNABLE;
        
    }
    TRACE_EXIT;
    return NULL;
}

static Context* kmt_context_schedule(Event ev, Context *context) {
    TRACE_ENTRY;
    int cpu = cpu_current();
    panic_on(task_list == NULL, "schedule null task!");
    task_t* task = current_tasks[cpu];
    bool quit_flag = false;
    print_task(current_tasks[cpu]);
    while(1){
        current_tasks[cpu] = current_tasks[cpu]->fd;
        switch (current_tasks[cpu]->status)
        {
            case RUNNABLE:
                quit_flag = true;
                break;
            default:
                break;
        }
        if(quit_flag == true && current_tasks[cpu] != task){
            current_tasks[cpu]->status = RUNNING;
            // printf("%s%d\n", current_tasks[cpu]->name, current_tasks[cpu]->status);
            break;
        }
    }
    TRACE_EXIT;
    return current_tasks[cpu]->context;
}

void kmt_init() {
    TRACE_ENTRY;
    current_tasks = (task_t**)pmm->alloc(sizeof(task_t*) * cpu_count());
    os->on_irq(sizeof(int) == 4 ? INT32_MIN : INT64_MIN, EVENT_NULL, kmt_context_save);   // 总是最先调用
    os->on_irq(sizeof(int) == 4 ? INT32_MAX : INT64_MAX, EVENT_NULL, kmt_context_schedule);       // 总是最后调用
    TRACE_EXIT;
}

int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg) {
    TRACE_ENTRY;
    panic_on(task == NULL, "kmt create error!");
    task->name = name;
    task->entry = entry;
    task->stack = pmm->alloc(STACK_SIZE);
    task->context = kcontext((Area){task->stack, task->stack + STACK_SIZE}, entry, arg);
    task->status = RUNNABLE;
    insert(&task_list, task);
    TRACE_EXIT;
    return 0;
}

void kmt_teardown(task_t *task) {
    TRACE_ENTRY;
    panic_on(task->status != NEVER_SCHEDUlE, "kmt teardown error!");
    pmm->free(task->stack);
    TRACE_EXIT;
}

void kmt_spin_init(spinlock_t *lk, const char *name) {
    TRACE_ENTRY;
    panic_on(lk == NULL, "kmt spin init error!");
    lk->name = name;
    lk->locked = UNLOCK;
    TRACE_EXIT;
}

void kmt_spin_lock(spinlock_t *lk){
    while (atomic_xchg(&lk->locked, LOCK)) {
        yield(); // 切换    
    }
}

void kmt_spin_unlock(spinlock_t *lk) {
    atomic_xchg(&lk->locked, UNLOCK);
}

void kmt_sem_init(sem_t *sem, const char *name, int value) {
    TRACE_ENTRY;
    panic_on(sem == NULL, "kmt sem init error!");
    kmt_spin_init(&sem->lock, name);
    sem->count = value;
    TRACE_EXIT;
}

void kmt_sem_wait(sem_t *sem) {
    TRACE_ENTRY;
    int success = true;
    kmt_spin_lock(&sem->lock); // 获得自旋锁
    sem->count--; // 自旋锁保证原子性
    if (sem->count < 0) {
        // 没有资源，需要等待
        task_t* task = current_tasks[cpu_current()];
        remove(&task_list, task);
        insert(&sem->wait_list, task);
        // 当前线程不能再执行
        task->status = BLOCKED;
        success = false;
    }
    kmt_spin_unlock(&sem->lock);
    if (success == false) {  // 如果 P 失败，不能继续执行
                 // (注意此时可能有线程执行 V 操作)
        yield(); // 引发一次上下文切换
    }
    TRACE_EXIT;
}

void kmt_sem_signal(sem_t *sem) {
    TRACE_ENTRY;
    kmt_spin_lock(&sem->lock); 
    sem->count++;
    if (sem->count <= 0){
        //释放链表中的一个线程
        task_t* task = sem->wait_list; 
        remove(&sem->wait_list, task);
        insert(&task_list, task);
        
        task->status = RUNNABLE;
    } 
    kmt_spin_unlock(&sem->lock);  
    TRACE_EXIT;
}

void kmt_mutex_init(mutexlock_t *lk, const char *name) {
    TRACE_ENTRY;
    lk->locked = 0;
    lk->wait_list = NULL;
    kmt_spin_init(&lk->lock, name);
    TRACE_EXIT;
}

void kmt_mutex_lock(mutexlock_t *lk) {
    TRACE_ENTRY;
    int acquired = 0;
    kmt_spin_lock(&lk->lock);
    if (lk->locked != 0) {
        task_t* task = current_tasks[cpu_current()];
        remove(&task_list, task);
        insert(&lk->wait_list, task);
        task->status = BLOCKED;
    } else {
        lk->locked = 1;
        acquired = 1;
    }
    kmt_spin_unlock(&lk->lock);
    if (!acquired) yield(); // 主动切换到其他线程执行
    TRACE_EXIT;
}

void kmt_mutex_unlock(mutexlock_t *lk) {
    TRACE_ENTRY;
    kmt_spin_lock(&lk->lock);
    if (!lk->wait_list) {
        task_t* task = lk->wait_list; 
        remove(&lk->wait_list, task);
        insert(&task_list, task);
        task->status = RUNNABLE; // 唤醒之前睡眠的线程
    } else {
        lk->locked = 0;
    }
    kmt_spin_unlock(&lk->lock);
    TRACE_EXIT;
}

MODULE_DEF(kmt) = {
    .init = kmt_init,
    .create = kmt_create,
    .teardown = kmt_teardown,
    .spin_init = kmt_spin_init,
    .spin_lock = kmt_spin_lock,
    .sem_init = kmt_sem_init,
    .sem_wait = kmt_sem_wait,
    .sem_signal = kmt_sem_signal,
    .mutex_init = kmt_mutex_init,
    .mutex_lock = kmt_mutex_lock,
    .mutex_unlock = kmt_mutex_unlock,
};
