
#include <common.h>
#include <lock.h>
#include <queue.h>


// 控制块中队列的物理内存起点, 这部分对用户是透明的
uintptr_t que_physical_start;
uintptr_t que_physical_end;

spare_queue_t *q;

int print_lock = 0;

void setScope(spare_node_t *p, uintptr_t start, uintptr_t end){
    p->_start = start;
    p->_end = end;
}


int log2(size_t size){
  int i = 0;
  for(; size > 1; size = size >> 1) ++i;
  return i;
}

spare_node_t* kalloc_node(spare_queue_t *q){
  int i = 0;

  lock_acquire(&(q->bit_lock));
  for(; i < MAP_BITS; ++i)
    if (q->bit_map[i].value == 0)break;
  q->bit_map[i].value = 1; 
  lock_release(&(q->bit_lock));

  spare_node_t* tmp = (spare_node_t*)(que_physical_start + sizeof(spare_node_t) * i);
  tmp->next =NULL;
  
  #ifdef verbose_on
    lock_acquire(&print_lock);
    printf("CPU#%d#: [%p, %p) spare_node_t malloc\n", cpu_current(), tmp, tmp + sizeof(spare_node_t));
    lock_release(&print_lock);
  #endif

  return tmp; 
}

void kfree_node(spare_queue_t *q, spare_node_t *pNode){
  int pos = ((uintptr_t)pNode - que_physical_start)/sizeof(spare_node_t);
  
  assert(pos < MAP_BITS);

  lock_acquire(&(q->bit_lock));
  q->bit_map[pos].value = 0;
  lock_release(&(q->bit_lock));

  memset(pNode, 0, sizeof(spare_node_t));
  
  #ifdef verbose_on
  lock_acquire(&print_lock);
  printf("CPU#%d#: [%p, %p) spare_node_t free\n", cpu_current(), pNode, (uintptr_t)pNode + sizeof(spare_node_t));
  lock_release(&print_lock);
  #endif
}

void print_spare_queue(spare_queue_t* q){
  for(int i = 0; i < 24; ++i){
    spare_node_t* cur = q->head[i];
    char buf[0x100] = {0};
    
    sprintf(buf, "%d:\t", i);
    // print("%d:\t", i);
    while(cur != NULL && cur != q->tail[i] && cur->next != NULL){
      
      sprintf(buf, "%s[0x%x, 0x%x)->", buf, cur->_start, cur->_end);
      // print("[0x%x, 0x%x)->", cur->_start, cur->_end);
      cur = cur->next;
    }
    if(cur != NULL){
      sprintf(buf, "%s[0x%x, 0x%x)", buf, cur->_start, cur->_end);
      // print("[0x%x, 0x%x)", cur->_start, cur->_end);
    } 
    sprintf(buf, "%s head = %p, tail = %p\n", buf, q->head[i], q->tail[i]); 
    // print(" head = %p, tail = %p\n", q->head[i], q->tail[i]);
    lock_acquire(&print_lock);
    printf("%s", buf);
    lock_release(&print_lock);
  }
}

void set_que_physical(){
  que_physical_start = (uintptr_t)heap.start + sizeof(spare_queue_t);
  for(que_physical_end = (uintptr_t)heap.end; que_physical_end > que_physical_start; que_physical_end -= (uintptr_t)MAX_SIZE);
  que_physical_end += MAX_SIZE;
  printf("[%p, %p) for physical queque\n", que_physical_start, que_physical_end);
}

void Queue_Init(spare_queue_t **pQue){
  set_que_physical();
  

  // 0x300000 - 0x400000 作为存放队列的堆区
  *pQue = (spare_queue_t*)heap.start;
  printf("[%p, %p) for spare_queue_t\n", heap.start, heap.start + sizeof(spare_queue_t));

  for(int i = 0; i < FREELIST_NUM; ++i){
    (*pQue)->head[i] = NULL; 
    (*pQue)->tail[i] = NULL; 
  }

  uintptr_t start = que_physical_end;

  (*pQue)->bit_lock = 0;
  spare_node_t *tmp = kalloc_node(*pQue);
  setScope(tmp, (uintptr_t)start, (uintptr_t)(start + MAX_SIZE)); 
  start += MAX_SIZE;
  (*pQue)->head[FREELIST_NUM - 1] = tmp;
  (*pQue)->tail[FREELIST_NUM - 1] = tmp;
  
  for(; start < (uintptr_t)heap.end; start += MAX_SIZE){
    spare_node_t *tmp = kalloc_node(*pQue); 
    setScope(tmp, (uintptr_t)start, (uintptr_t)(start + MAX_SIZE));
    (*pQue)->tail[FREELIST_NUM - 1]->next = tmp;
    (*pQue)->tail[FREELIST_NUM - 1] = tmp;
  }

  for(int i = 0; i < FREELIST_NUM; ++i){
    (*pQue)->head_lock[i] = 0;
    (*pQue)->tail_lock[i] = 0;
  }
  printf("init end.\n");
}

void Queue_Enqueue(spare_queue_t *q, uintptr_t start, size_t size){
    
    assert(log2(size) - 1 < FREELIST_NUM && log2(size) - 1 >= 0);

    lock_acquire(&q->tail_lock[log2(size) - 1]);
    spare_node_t *tmp = kalloc_node(q);
    setScope(tmp, start, start + (uintptr_t)size);
    tmp->next = NULL;
    assert(tmp != NULL);
    
    if(q->tail[log2(size) - 1] == NULL || q->head[log2(size) - 1] == NULL) {
      q->head[log2(size) - 1] = tmp;
      q->tail[log2(size) - 1] = tmp;
    }
    else{
      q->tail[log2(size) - 1]->next = tmp;
      q->tail[log2(size) - 1] = tmp;
    }

    #ifdef verbose_on
      lock_acquire(&print_lock);
      printf("CPU#%d#: spare_node [%p, %p) enter queue %d\n", cpu_current(), tmp->_start, tmp->_end, log2(size) - 1);
      lock_release(&print_lock);
    #endif
    
    lock_release(&q->tail_lock[log2(size) - 1]);
    
}

int Queue_Dequeue(spare_queue_t *q, uintptr_t* start, size_t size){
    
    assert(log2(size) - 1 < FREELIST_NUM);

    lock_acquire(&q->head_lock[log2(size) - 1]);
    
    spare_node_t *tmp = q->head[log2(size) - 1];
    
    if (tmp == NULL)
    {
        lock_release(&q->head_lock[log2(size) - 1]);
        return -1; // queue was empty
    }

    
    spare_node_t *new_head = tmp->next;
    *start = tmp->_start; 
    q->head[log2(size) - 1] = new_head;
    
    lock_release(&q->head_lock[log2(size) - 1]);
    
    #ifdef verbose_on
      lock_acquire(&print_lock);
      printf("CPU#%d#: spare_node [%p, %p) departure queue %d\n", cpu_current(), tmp->_start, tmp->_end, log2(size) - 1);
      lock_release(&print_lock);
    #endif 
    kfree_node(q, tmp);    
    return 0;
} 

uintptr_t Queue_Merge(spare_queue_t *q, uintptr_t start, size_t size){
    uintptr_t friend_pos = start + size * (((start - que_physical_end) / size % 2)==0?1:-1);
    int que_number = log2(size) - 1; 
    // 遍历队列，队尾插入个标记
    
    lock_acquire(&q->tail_lock[que_number]); lock_acquire(&q->head_lock[que_number]);
    if(q->head[que_number] == NULL){
      lock_release(&q->tail_lock[que_number]); lock_release(&q->head_lock[que_number]);
      return 0; 
    }
    spare_node_t *tag = kalloc_node(q);
    q->tail[que_number]->next = tag;
    q->tail[que_number] = tag; 

    while(1){
      spare_node_t *cur = q->head[que_number];
      if(cur->_start == friend_pos || tag == cur)break;
      q->head[que_number] = q->head[que_number]->next;
      q->tail[que_number]->next = cur; cur->next = NULL;
      q->tail[que_number] = cur;
    }

    if(q->head[que_number] == tag){
      // 说明队列中没有伙伴
      q->head[que_number] = tag->next;
      kfree_node(q, tag);
      lock_release(&q->tail_lock[que_number]); lock_release(&q->head_lock[que_number]);
      return 0;
    }else{
      // 队头是伙伴
      spare_node_t *cur = q->head[que_number]; 
      q->head[que_number] = cur->next; 
      kfree_node(q, cur);
      cur = q->head[que_number];
      
      if(cur == tag){
        // 队列中仅有伙伴
        q->head[que_number] = cur->next; 
        kfree_node(q, tag);
      }else{
        while(cur->next != tag){
          if(cur == q->tail[que_number]){
            panic("Error! No tag in queue!");
            break;
          } 
          assert(cur->next != NULL);
          cur = cur->next;
        }

        assert(cur->next == tag);
        cur->next = cur->next->next;
        kfree_node(q, tag);
        if(cur->next == NULL )q->tail[que_number] = cur;
      }
      
      if(q->head[que_number] == NULL)q->tail[que_number] = NULL;
      lock_acquire(&print_lock);
      printf("CPU#%d#: spare_node [%p, %p) and [%p, %p) merge \n", cpu_current(), start, start + (uintptr_t)size, friend_pos, friend_pos + (uintptr_t)size);
      lock_release(&print_lock);
      lock_release(&q->tail_lock[que_number]); lock_release(&q->head_lock[que_number]);
      return friend_pos > start? start : friend_pos;
    }
}