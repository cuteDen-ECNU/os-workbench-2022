# L1 物理内存管理 (pmm)

## 阅读笔记（[Operating Systems: Three Easy Pieces](http://pages.cs.wisc.edu/~remzi/OSTEP/)）

### 空闲空间管理（17章）

问题: 主要是外部碎片的问题

外部碎片: 空闲链表中的可分配的内存太小，导致的外部碎片

内部碎片: 分配器分发大块的大于请求的内存，导致的内部碎片

First Fit算法分配

### 并发数据结构（29章）

问题：

当给定一个特定的数据结构时，为了让它正确工作，我们应该如何添加锁?

此外，我们如何添加锁使得数据结构产生高性能，支持多个线程同时访问该结构?

- 并发计数器
  
  - 普通计数器: 一把锁
  
  - 近似计数器: 局部锁和全局锁，提高并发的性能和伸缩性

- 链表
  
  - 普通链表: 加锁并发操作
  
  - 伸缩链表: hand-over-hand locking (a.k.a. lock coupling)
    
    - lock coupling: 不是整个列表中都只有一个锁，而是为列表的每个节点添加一个锁。
    
    - 当遍历列表时，代码首先获取下一个节点的锁，然后释放当前节点的锁。
  
  - tips: 警惕控制流改变导致的函数停止执行。
  
  - 但是如果每个节点都有一把锁，是开销非常大的，所以隔几个节点设置一个锁是更好的选择。

- 队列
  
  - Michael and Scott 提出的并发队列

### 初始化队列

最终决定使用 Michael and Scott 提出的并发队列，先观察linux是如果分配堆空间

sizeof queue_t = 96

```gdb
(gdb) p q
$1 = (queue_t *) 0x5555555592a0
(gdb) p tmp
$2 = (node_t *) 0x555555559310
```

发现申请一段堆空间会多预留16个字节。

## tcmalloc

page, span

thread_cache

global_cache

很复杂，算了

### 伙伴系统

维护多个freelist，每个list中的空闲块大小相等，且大小都是2的幂。

在本实验中维护24个freelist（16MiB = 2^{24}），每个freelist队头一把锁，队尾一把锁。

分配: 分配一个log2(size)+1大小的空闲块，

回收: 在2^n大小的队列中找有没有伙伴，如果有那么合并

friendpos = freeptr+2^n *(-1)^{{(freeptr - pagestart)/(2^n)mode2}}

### 队列的物理存放

gdb的wa太好用了，debug的时候可以看到值是在什么时刻变化的！

### Free

发现没办法确定要free的内存大小是多少，只知道指针的信息。

Linux的申请的内存的大小信息是存在这段内存的的前8个字节当中。

kalloc施工ing

kfree施工ing

## 码代码中的debug（还在使用纯种gdb的萌新阶段）

### klib

实现printf过程中遇到的问题

- va_list

```c
// 从man手册中摘抄的代码
int foo(const char *fmt, ...) /* '...' is C syntax for a variadic function */
{
    va_list ap;
    int d;
    char c, *s;

    va_start(ap, fmt);
    while (*fmt)
        switch (*fmt++)
        {
        case 's': /* string */
            s = va_arg(ap, char *);
            printf("string %s\n", s);
            break;
        case 'd': /* int */
            d = va_arg(ap, int);
            printf("int %d\n", d);
            break;
        case 'c': /* char */
            /* need a cast here since va_arg only
               takes fully promoted types */
            c = (char)va_arg(ap, int);
            printf("char %c\n", c);
            break;
        }
    va_end(ap);
}
// 使用方法

int main()
{
    char s[2014] = "Hello World\n";
    char *p = '\0';

    // p += strlen(s);
    int x = 1;
    foo("%s %d\n", s, x);
    return 0;
}
```

使用gdb进行调试的方法

```gdb
p *(char**)(ap[0].reg_save_area + 8) 
$7 = 0x7fffffffd880 "Hello World\n"
```

klib中的printf并不能奏效（$7 没有该变量）：可能是因为内存管理部分代码没有完成，而va_list的实现是基于内存的。(通过看jyy实验手册L0得到的知识点否定了这一可能)

实际上va_list是一个内置结构体：

The freestanding headers are: `float.h`, `iso646.h`, `limits.h`, `stdalign.h`, `stdarg.h`, `stdbool.h`, `stddef.h`, `stdint.h`, and `stdnoreturn.h`. You should be familiar with these headers as they contain useful declarations you shouldn't do yourself. GCC also comes with additional freestanding headers for CPUID, SSE and such.

没有实现内存管理也是可以运行的

问题出在memset上，memset的代码实现的有问题，问题太太太太太智障了，找的时候用了一个下午+一个晚上（正确性夺重要啊）

```gdb
args[0] = (struct __va_list_tag *) 0x207990 <__am_cpuinfo+7216>
```

### 测试用例

jyy老师给的用例是这样的，但是random_op 和alloc_check函数都没有给定，只能自己写。

alloc_check用c写太难搞了，不如打印log然后用python做个可视化。

```c
#include <common.h>
#include <klib.h>

enum ops { OP_ALLOC = 1, OP_FREE };
struct malloc_op {
  enum ops type;
  union { size_t sz; void *addr; };
};

void stress_test() {
  while (1) {
    struct malloc_op op = random_op();
    switch (op.type) {
      case OP_ALLOC: alloc_check(pmm->alloc(op.sz), op.sz); break;
      case OP_FREE:  free(op.addr); break;
    }
  }
}

int main() {
  os->init();
  mpe_init(stress_test);
}
```

为了查bug，增加一个队列的打印（可视化还是很重要滴）

- 单核测试:
  
  ![截屏2022-09-18 下午2.16.26.png](/var/folders/3d/0s1z_slx41j5c538glbwhgtr0000gn/T/TemporaryItems/（screencaptureui正在存储文稿，已完成26）/截屏2022-09-18%20下午2.16.26.png)

- 并发之后，全崩QAQ
  
  ![截屏2022-09-18 下午9.00.18.png](/Users/Wendy/Desktop/截屏2022-09-18%20下午9.00.18.png)

        ~~加延迟不太能保证bug再现~~。

        printf是线程安全的。

        gdb一步一步调试，发现是锁的代码被gcc优化掉了，加了一行volatile成功了，换了一个版本的gcc也成功了。
