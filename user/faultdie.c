/* Test user-level fault handler -- just exit when we fault */

#include <inc/lib.h>

bool
handler(struct UTrapframe *utf) {
    void *addr = (void *)utf->utf_fault_va;
    uint64_t err = utf->utf_err;

    cprintf("handler: addr %lx\n", (unsigned long)&addr);
    cprintf("handler: err %lx\n", (unsigned long)&err);

    cprintf("i faulted at va %lx, err %x\n",
            (unsigned long)addr, (unsigned)(err & 7));
    
    cprintf("handler\n");    
    int32_t envid = sys_getenvid();
    cprintf("handler: %d\n", envid);

    sys_env_destroy(sys_getenvid());

    return 1;
}

void
umain(int argc, char **argv) {
    cprintf("handler umain: %d\n", sys_getenvid());
    add_pgfault_handler(handler);
    *(volatile int *)0xDEADBEEF = 0;
}


// add-symbol-file ./obj/user/faultdie
/*
add-symbol-file ./obj/user/faultdie
b platform_asan_fatal 
c
bt
fr 3

00: 0x8042154dcb
01: 0x8042154d81
02: 0x804214f5f8
03: 0x804213cb9d
04: 0x8042147119
05: 0x8042141273
06: 0x80421438b6
07: 0x00800456
08: 0x008000b6
09: 0x00801c1a
10: 0x00801ef7
11: 0x00800250
12: 0x00800023

i faulted at va deadbeef, err 6n0
*/