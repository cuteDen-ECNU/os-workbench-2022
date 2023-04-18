#include <common.h>
#include <devices.h>

static void tty_reader(void *arg) {
  device_t *tty = dev->lookup(arg);
  char cmd[128], resp[128], ps[16];
  snprintf(ps, 16, "(%s) $ ", arg);
  while (1) {
    tty->ops->write(tty, 0, ps, strlen(ps));
    int nread = tty->ops->read(tty, 0, cmd, sizeof(cmd) - 1);
    cmd[nread] = '\0';
    sprintf(resp, "tty reader task: got %d character(s).\n", strlen(cmd));
    tty->ops->write(tty, 0, resp, strlen(resp));
  }
}

task_t* task_alloc() {
  task_t* task = pmm->alloc(sizeof(struct task));
  printf("sizeof(*task) = %d\n", sizeof(*task));
  return task;
}

static void os_init() {
  pmm->init();
  kmt->init();
  dev->init();
  kmt->create(task_alloc(), "tty_reader", tty_reader, "tty1");
  kmt->create(task_alloc(), "tty_reader", tty_reader, "tty2");
}

static void os_run() {
  iset(true);
  yield();
}

typedef struct handler_info{
  int seq;
  int event;
  handler_t handler;
  struct handler_info* next;
}handler_info;

handler_info* handlers_sorted_by_seq; // header

static void on_irq(int seq, int event, handler_t handler){
  handler_info* p = handlers_sorted_by_seq, *pre = NULL;
  while(p){
    if(p->seq < seq)break;
    pre = p;
    p = p->next;
  }
  if(pre == NULL){
    pre = pmm->alloc(sizeof(handler_info));
    pre->seq = seq; pre->event = event; pre->handler = handler;
    pre->next = p;
    handlers_sorted_by_seq = pre;
  }
  else{
    handler_info* new_handler = pmm->alloc(sizeof(handler_info));
    new_handler->seq = seq; new_handler->event = event; new_handler->handler = handler;
    pre->next = new_handler; new_handler->next = p;
  }
}
static Context *os_trap(Event ev, Context *ctx) {
  Context *next = NULL;

  for (handler_info* h = handlers_sorted_by_seq; h != NULL; ++h) {
    if (h->event == EVENT_NULL || h->event == ev.event) {
      Context *r = h->handler(ev, ctx);
      panic_on(r && next, "returning multiple contexts");
      if (r) next = r;
    }
  }
  panic_on(!next, "returning NULL context");
  // panic_on(sane_context(next), "returning to invalid context");
  return next;
}

MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
  .trap = os_trap,
  .on_irq = on_irq,
};
