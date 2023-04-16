#include <os.h>

void kmt_init() {
    int* p = (int*)pmm->alloc(4);
    *p = 3;
    printf("%d\n", *p);
}

MODULE_DEF(kmt) = {
    .init = kmt_init
};
