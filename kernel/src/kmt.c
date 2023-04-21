#include <os.h>
#define STACK_SIZE 1024 * 8
#define MAX_CPU 8

task_t** current_tasks;

task_t* task_list = NULL;

static void print_task(task_t* head) {
#ifdef PRINT_TASK
    task_t* task = head;
    if(task == NULL){
        printf("null");
    }else{
        printf("%s", task->name);
        while(task->fd != head){
            task = task->fd;
            printf("-> %s", task->name);
        }
        printf("\n");
    }
#endif
}

static void insert(task_t* head, task_t* task){
    panic_on(task == NULL, "insert task null");
    if(head == NULL){
        head = task;
        head->bk = task;
        head->fd = task;
    }
    else{
        task->fd = head;
        task->bk = head->bk;
        head->bk = task;
        task->bk->fd = task;
    }
    print_task(head);
}

static void remove(task_t** head, task_t* task){
    panic_on(task == NULL, "remove task null"); 
    if((*head)->bk == *head){
        panic_on((*head)->fd == *head && task == *head, "error task_list!");
        head = NULL;
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
    print_task(*head);
}

static Context* kmt_context_save(Event ev, Context *context) {
    panic_on(context == NULL, "error context!");
    int cpu = cpu_current();
    if (current_tasks[cpu] == NULL){
        current_tasks[cpu] = task_list;
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
        current_tasks[cpu] = current_tasks[cpu]->fd;
        switch (current_tasks[cpu]->status)
        {
            case RUNNABLE:
                quit_flag = true;
                break;
            default:
                current_tasks[cpu] = current_tasks[cpu]->fd; 
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
    insert(task_list, task);
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

void kmt_spin_unlock(spinlock_t *lk) {
    atomic_xchg(&lk->locked, UNLOCK);
}

void kmt_sem_init(sem_t *sem, const char *name, int value) {
    panic_on(sem == NULL, "kmt sem init error!");
    kmt_spin_init(&sem->lock, name);
    sem->count = value;
}

void kmt_sem_wait(sem_t *sem) {
    int success = true;
    kmt_spin_lock(&sem->lock); // 获得自旋锁
    sem->count--; // 自旋锁保证原子性
    if (sem->count < 0) {
        // 没有资源，需要等待
        task_t* task = current_tasks[cpu_current()];
        remove(&task_list, task);
        insert(sem->list_head, task);
        // 当前线程不能再执行
        task->status = BLOCKED;
        success = false;
    }
    kmt_spin_unlock(&sem->lock);
    if (success == false) {  // 如果 P 失败，不能继续执行
                // (注意此时可能有线程执行 V 操作)
        yield(); // 引发一次上下文切换
    }
}

void kmt_sem_signal(sem_t *sem) {
    kmt_spin_lock(&sem->lock); 
    sem->count++;
    if (sem->count <= 0){
        //释放链表中的一个线程
        task_t* task = sem->list_head; 
        remove(&sem->list_head, task);
        insert(task_list, task);
        
        task->status = RUNNABLE;
    } 
    kmt_spin_unlock(&sem->lock);  
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
};
