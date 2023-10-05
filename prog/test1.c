void (*volatile sys_yield)(void);
void (*volatile cprintf)(const char *, ...);

void
umain(int argc, char **argv) {
    int i, j;

    for (j = 0; j < 3; ++j) {
        for (i = 0; i < 10000; ++i)
            ;
        cprintf("core test1\n");
        sys_yield();
    }
}
