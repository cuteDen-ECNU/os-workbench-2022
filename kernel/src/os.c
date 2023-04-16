#include <common.h>

static void os_init() {
  pmm->init();
}

static void os_run() {
  for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
    putch(*s == '*' ? '0' + cpu_current() : *s);
  }
  while (1) ;
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
