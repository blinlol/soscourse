// #include <inc/assert.h>
// #include <inc/memlayout.h>
// #include <inc/stdio.h>
// #include <inc/string.h>
// #include <inc/types.h>
// #include <inc/uefi.h>
// #include <inc/x86.h>
// #include <kern/kclock.h>
// #include <kern/picirq.h>
// #include <kern/pmap.h>
// #include <kern/pmap.h>
// #include <kern/timer.h>
// #include <kern/trap.h>
// #include <kern/tsc.h>

// #define kilo      (1000ULL)
// #define Mega      (kilo * kilo)
// #define Giga      (kilo * Mega)
// #define Tera      (kilo * Giga)
// #define Peta      (kilo * Tera)
// #define ULONG_MAX ~0UL

// #if LAB <= 6
// /* Early variant of memory mapping that does 1:1 aligned area mapping
//  * in 2MB pages. You will need to reimplement this code with proper
//  * virtual memory mapping in the future. */
// void *
// mmio_map_region(physaddr_t pa, size_t size) {
//     void map_addr_early_boot(uintptr_t addr, uintptr_t addr_phys, size_t sz);
//     const physaddr_t base_2mb = 0x200000;
//     uintptr_t org = pa;
//     size += pa & (base_2mb - 1);
//     size += (base_2mb - 1);
//     pa &= ~(base_2mb - 1);
//     size &= ~(base_2mb - 1);
//     map_addr_early_boot(pa, pa, size);
//     return (void *)org;
// }
// void *
// mmio_remap_last_region(physaddr_t pa, void *addr, size_t oldsz, size_t newsz) {
//     return mmio_map_region(pa, newsz);
// }
// #endif

// struct Timer timertab[MAX_TIMERS];
// struct Timer *timer_for_schedule;

// struct Timer timer_hpet0 = {
//         .timer_name = "hpet0",
//         .timer_init = hpet_init,
//         .get_cpu_freq = hpet_cpu_frequency,
//         .enable_interrupts = hpet_enable_interrupts_tim0,
//         .handle_interrupts = hpet_handle_interrupts_tim0,
// };

// struct Timer timer_hpet1 = {
//         .timer_name = "hpet1",
//         .timer_init = hpet_init,
//         .get_cpu_freq = hpet_cpu_frequency,
//         .enable_interrupts = hpet_enable_interrupts_tim1,
//         .handle_interrupts = hpet_handle_interrupts_tim1,
// };

// struct Timer timer_acpipm = {
//         .timer_name = "pm",
//         .timer_init = acpi_enable,
//         .get_cpu_freq = pmtimer_cpu_frequency,
// };

// void
// acpi_enable(void) {
//     FADT *fadt = get_fadt();
//     outb(fadt->SMI_CommandPort, fadt->AcpiEnable);
//     while ((inw(fadt->PM1aControlBlock) & 1) == 0) /* nothing */
//         ;
// }

// static void *
// acpi_find_table(const char *sign) {
//     /*
//      * This function performs lookup of ACPI table by its signature
//      * and returns valid pointer to the table mapped somewhere.
//      *
//      * It is a good idea to checksum tables before using them.
//      *
//      * HINT: Use mmio_map_region/mmio_remap_last_region
//      * before accessing table addresses
//      * (Why mmio_remap_last_region is requrired?)
//      * HINT: RSDP address is stored in uefi_lp->ACPIRoot
//      * HINT: You may want to distunguish RSDT/XSDT
//      */
//     // LAB 5: Your code here:
//     RSDP* rsdp = (RSDP*) mmio_map_region(uefi_lp->ACPIRoot, sizeof(RSDP));
//     if (strncmp(rsdp->Signature, "RSD PTR ", 8) != 0){
//         panic("wrong rsdp signature\n");
//     }
//     bool revision = rsdp->Revision;

//     RSDT* rsdt;
//     physaddr_t rsdt_phys;
//     if (revision){
//         rsdt_phys = rsdp->XsdtAddress;
//     }
//     else{
//         rsdt_phys = rsdp->RsdtAddress;
//     }
    
//     rsdt = (RSDT*)mmio_map_region(rsdt_phys, sizeof(RSDT));
//     rsdt = (RSDT*)mmio_remap_last_region(rsdt_phys, (void*)rsdt, sizeof(RSDT), rsdt->h.Length);

//     uint64_t count = (rsdt->h.Length - sizeof(rsdt->h)) / 4;
//     if (revision){
//         count = count / 2;
//     }

//     physaddr_t header_phys;
//     ACPISDTHeader *header;
//     for (int i = 0; i < count; i++){
//         header_phys = rsdt->PointerToOtherSDT[i];
//         header = (ACPISDTHeader*) mmio_map_region(header_phys, sizeof(ACPISDTHeader));
//         header = (ACPISDTHeader*) mmio_remap_last_region(header_phys, 
//                                                         (void*)header, 
//                                                         sizeof(ACPISDTHeader), 
//                                                         header->Length);
//         if (strncmp(header->Signature, sign, 4) == 0){
//             return header;
//         }

//     }

//     return NULL;
// }

// MCFG *
// get_mcfg(void) {
//     static MCFG *kmcfg;
//     if (!kmcfg) {
//         struct AddressSpace *as = switch_address_space(&kspace);
//         kmcfg = acpi_find_table("MCFG");
//         switch_address_space(as);
//     }

//     return kmcfg;
// }

// #define MAX_SEGMENTS 16

// uintptr_t
// make_fs_args(char *ustack_top) {

//     MCFG *mcfg = get_mcfg();
//     if (!mcfg) {
//         cprintf("MCFG table is absent!");
//         return (uintptr_t)ustack_top;
//     }

//     char *argv[MAX_SEGMENTS + 3] = {0};

//     /* Store argv strings on stack */

//     ustack_top -= 3;
//     argv[0] = ustack_top;
//     nosan_memcpy(argv[0], "fs", 3);

//     int nent = (mcfg->h.Length - sizeof(MCFG)) / sizeof(CSBAA);
//     if (nent > MAX_SEGMENTS)
//         nent = MAX_SEGMENTS;

//     for (int i = 0; i < nent; i++) {
//         CSBAA *ent = &mcfg->Data[i];

//         char arg[64];
//         snprintf(arg, sizeof(arg) - 1, "ecam=%llx:%04x:%02x:%02x",
//                  (long long)ent->BaseAddress, ent->SegmentGroup, ent->StartBus, ent->EndBus);

//         int len = strlen(arg) + 1;
//         ustack_top -= len;
//         nosan_memcpy(ustack_top, arg, len);
//         argv[i + 1] = ustack_top;
//     }

//     char arg[64];
//     snprintf(arg, sizeof(arg) - 1, "tscfreq=%llx", (long long)tsc_calibrate());
//     int len = strlen(arg) + 1;
//     ustack_top -= len;
//     nosan_memcpy(ustack_top, arg, len);
//     argv[nent + 1] = ustack_top;

//     /* Realign stack */
//     ustack_top = (char *)((uintptr_t)ustack_top & ~(2 * sizeof(void *) - 1));

//     /* Copy argv vector */
//     ustack_top -= (nent + 3) * sizeof(void *);
//     nosan_memcpy(ustack_top, argv, (nent + 3) * sizeof(argv[0]));

//     char **argv_arg = (char **)ustack_top;
//     long argc_arg = nent + 2;

//     /* Store argv and argc arguemnts on stack */
//     ustack_top -= sizeof(void *);
//     nosan_memcpy(ustack_top, &argv_arg, sizeof(argv_arg));
//     ustack_top -= sizeof(void *);
//     nosan_memcpy(ustack_top, &argc_arg, sizeof(argc_arg));

//     /* and return new stack pointer */
//     return (uintptr_t)ustack_top;
// }

// /* Obtain and map FADT ACPI table address. */
// FADT *
// get_fadt(void) {
//     // LAB 5: Your code here
//     // (use acpi_find_table)
//     // HINT: ACPI table signatures are
//     //       not always as their names
     
//     FADT* fadt = (FADT*) acpi_find_table("FACP");
//     if (!fadt){
//         panic("no fadt\n");
//     }

//     return fadt;
// }

// /* Obtain and map RSDP ACPI table address. */
// HPET *
// get_hpet(void) {
//     // LAB 5: Your code here
//     // (use acpi_find_table)
//     HPET* hpet = (HPET*) acpi_find_table("HPET");
//     if (!hpet){
//         panic("no hpet\n");
//     }
//     return hpet;
// }

// /* Getting physical HPET timer address from its table. */
// HPETRegister *
// hpet_register(void) {
//     HPET *hpet_timer = get_hpet();
//     if (!hpet_timer->address.address) panic("hpet is unavailable\n");

//     uintptr_t paddr = hpet_timer->address.address;
//     return mmio_map_region(paddr, sizeof(HPETRegister));
// }

// /* Debug HPET timer state. */
// void
// hpet_print_struct(void) {
//     HPET *hpet = get_hpet();
//     assert(hpet != NULL);
//     cprintf("signature = %s\n", (hpet->h).Signature);
//     cprintf("length = %08x\n", (hpet->h).Length);
//     cprintf("revision = %08x\n", (hpet->h).Revision);
//     cprintf("checksum = %08x\n", (hpet->h).Checksum);

//     cprintf("oem_revision = %08x\n", (hpet->h).OEMRevision);
//     cprintf("creator_id = %08x\n", (hpet->h).CreatorID);
//     cprintf("creator_revision = %08x\n", (hpet->h).CreatorRevision);

//     cprintf("hardware_rev_id = %08x\n", hpet->hardware_rev_id);
//     cprintf("comparator_count = %08x\n", hpet->comparator_count);
//     cprintf("counter_size = %08x\n", hpet->counter_size);
//     cprintf("reserved = %08x\n", hpet->reserved);
//     cprintf("legacy_replacement = %08x\n", hpet->legacy_replacement);
//     cprintf("pci_vendor_id = %08x\n", hpet->pci_vendor_id);
//     cprintf("hpet_number = %08x\n", hpet->hpet_number);
//     cprintf("minimum_tick = %08x\n", hpet->minimum_tick);

//     cprintf("address_structure:\n");
//     cprintf("address_space_id = %08x\n", (hpet->address).address_space_id);
//     cprintf("register_bit_width = %08x\n", (hpet->address).register_bit_width);
//     cprintf("register_bit_offset = %08x\n", (hpet->address).register_bit_offset);
//     cprintf("address = %08lx\n", (unsigned long)(hpet->address).address);
// }

// /* memory of hpet (memory registers) */
// static volatile HPETRegister *hpetReg;
// /* HPET timer period (in femtoseconds) */
// static uint64_t hpetFemto = 0;
// /* HPET timer frequency */
// static uint64_t hpetFreq = 0;

// /* HPET timer initialisation */
// void
// hpet_init() {
//     if (hpetReg == NULL) {
//         nmi_disable();
//         hpetReg = hpet_register();
//         uint64_t cap = hpetReg->GCAP_ID;
//         hpetFemto = (uintptr_t)(cap >> 32);
//         if (!(cap & HPET_LEG_RT_CAP)) panic("HPET has no LegacyReplacement mode");

//         // cprintf("hpetFemto = %llu\n", hpetFemto);
//         hpetFreq = (1 * Peta) / hpetFemto;
//         // cprintf("HPET: Frequency = %d.%03dMHz\n", (uintptr_t)(hpetFreq / Mega), (uintptr_t)(hpetFreq % Mega));
//         /* Enable ENABLE_CNF bit to enable timer */
//         hpetReg->GEN_CONF |= HPET_ENABLE_CNF;
//         nmi_enable();
//     }
// }

// /* HPET register contents debugging. */
// void
// hpet_print_reg(void) {
//     cprintf("GCAP_ID = %016lx\n", (unsigned long)hpetReg->GCAP_ID);
//     cprintf("GEN_CONF = %016lx\n", (unsigned long)hpetReg->GEN_CONF);
//     cprintf("GINTR_STA = %016lx\n", (unsigned long)hpetReg->GINTR_STA);
//     cprintf("MAIN_CNT = %016lx\n", (unsigned long)hpetReg->MAIN_CNT);
//     cprintf("TIM0_CONF = %016lx\n", (unsigned long)hpetReg->TIM0_CONF);
//     cprintf("TIM0_COMP = %016lx\n", (unsigned long)hpetReg->TIM0_COMP);
//     cprintf("TIM0_FSB = %016lx\n", (unsigned long)hpetReg->TIM0_FSB);
//     cprintf("TIM1_CONF = %016lx\n", (unsigned long)hpetReg->TIM1_CONF);
//     cprintf("TIM1_COMP = %016lx\n", (unsigned long)hpetReg->TIM1_COMP);
//     cprintf("TIM1_FSB = %016lx\n", (unsigned long)hpetReg->TIM1_FSB);
//     cprintf("TIM2_CONF = %016lx\n", (unsigned long)hpetReg->TIM2_CONF);
//     cprintf("TIM2_COMP = %016lx\n", (unsigned long)hpetReg->TIM2_COMP);
//     cprintf("TIM2_FSB = %016lx\n", (unsigned long)hpetReg->TIM2_FSB);
// }

// /* HPET main timer counter value. */
// uint64_t
// hpet_get_main_cnt(void) {
//     return hpetReg->MAIN_CNT;
// }

// /* - Configure HPET timer 0 to trigger every 0.5 seconds on IRQ_TIMER line
//  * - Configure HPET timer 1 to trigger every 1.5 seconds on IRQ_CLOCK line
//  *
//  * HINT To be able to use HPET as PIT replacement consult
//  *      LegacyReplacement functionality in HPET spec.
//  * HINT Don't forget to unmask interrupt in PIC */
// void
// hpet_enable_interrupts_tim0(void) {
//     // LAB 5: Your code here
//     // general configuration register reg HPET_ENABLE_CNF
//     hpetReg->GEN_CONF |= HPET_ENABLE_CNF | HPET_LEG_RT_CNF;
//     // t0 configuration register
//     // per_int_cap == 1 && type_cnf == 1 -> periodic interrapts
//     hpetReg->TIM0_CONF |= HPET_TN_VAL_SET_CNF |
//                           HPET_TN_TYPE_CNF |
//                           HPET_TN_INT_ENB_CNF;
//     hpetReg->TIM0_COMP = Peta / 2 / hpetFemto;
//     pic_irq_unmask(IRQ_TIMER);
// }

// void
// hpet_enable_interrupts_tim1(void) {
//     // LAB 5: Your code here
//     hpetReg->GEN_CONF |= HPET_ENABLE_CNF | HPET_LEG_RT_CNF;
//     hpetReg->TIM0_CONF |= HPET_TN_VAL_SET_CNF |
//                           HPET_TN_TYPE_CNF |
//                           HPET_TN_INT_ENB_CNF;
//     hpetReg->TIM0_COMP = 3 * Peta / 2 / hpetFemto;
//     pic_irq_unmask(IRQ_CLOCK);
// }

// void
// hpet_handle_interrupts_tim0(void) {
//     pic_send_eoi(IRQ_TIMER);
// }

// void
// hpet_handle_interrupts_tim1(void) {
//     pic_send_eoi(IRQ_CLOCK);
// }

// /* Calculate CPU frequency in Hz with the help with HPET timer.
//  * HINT Use hpet_get_main_cnt function and do not forget about
//  * about pause instruction. */
// uint64_t
// hpet_cpu_frequency(void) {
//     static uint64_t cpu_freq;

//     // LAB 5: Your code here
//     uint64_t delay = 100;
//     uint64_t delta = 0;

//     uint64_t hpet_start = hpet_get_main_cnt();

//     uint64_t tsc_start = read_tsc();
//     uint64_t tsc_end = tsc_start;
    
//     while (delta < hpetFreq / delay){
//         asm ("pause");
//         delta = hpet_get_main_cnt() - hpet_start;
//         tsc_end = read_tsc();
//     };

//     cpu_freq = (tsc_end - tsc_start) * hpetFreq / delta;
//     return cpu_freq;
// }

// uint32_t
// pmtimer_get_timeval(void) {
//     FADT *fadt = get_fadt();
//     return inl(fadt->PMTimerBlock);
// }

// /* Calculate CPU frequency in Hz with the help with ACPI PowerManagement timer.
//  * HINT Use pmtimer_get_timeval function and do not forget that ACPI PM timer
//  *      can be 24-bit or 32-bit. */
// uint64_t
// pmtimer_cpu_frequency(void) {
//     static uint64_t cpu_freq;

//     // LAB 5: Your code here

//     uint64_t delay = 100;
//     uint64_t delta = 0;

//     uint64_t pm_start = pmtimer_get_timeval();
//     uint64_t current;

//     uint64_t tsc_start = read_tsc();
//     uint64_t tsc_end;
    
//     while (delta < PM_FREQ / delay){
//         asm ("pause");
//         current = pmtimer_get_timeval();
//         tsc_end = read_tsc();

//         if (pm_start <= current){
//             delta = current - pm_start;
//         }
//         else if (pm_start - current <= 0xFFFFFF){
//             delta = (uint64_t)current + 0xFFFFFF - pm_start;           
//         }
//         else{
//             delta = (uint64_t)current + 0xFFFFFFFF - pm_start;
//         }
//     };

//     cpu_freq = (tsc_end - tsc_start) * PM_FREQ / delta;

//     return cpu_freq;
// }




















































#include <inc/assert.h>
#include <inc/memlayout.h>
#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/types.h>
#include <inc/uefi.h>
#include <inc/x86.h>
#include <kern/kclock.h>
#include <kern/picirq.h>
#include <kern/pmap.h>
#include <kern/pmap.h>
#include <kern/timer.h>
#include <kern/trap.h>
#include <kern/tsc.h>

#define kilo      (1000ULL)
#define Mega      (kilo * kilo)
#define Giga      (kilo * Mega)
#define Tera      (kilo * Giga)
#define Peta      (kilo * Tera)
#define ULONG_MAX ~0UL

#if LAB <= 6
/* Early variant of memory mapping that does 1:1 aligned area mapping
 * in 2MB pages. You will need to reimplement this code with proper
 * virtual memory mapping in the future. */
void *
mmio_map_region(physaddr_t pa, size_t size) {
    void map_addr_early_boot(uintptr_t addr, uintptr_t addr_phys, size_t sz);
    const physaddr_t base_2mb = 0x200000;
    uintptr_t org = pa;
    size += pa & (base_2mb - 1);
    size += (base_2mb - 1);
    pa &= ~(base_2mb - 1);
    size &= ~(base_2mb - 1);
    map_addr_early_boot(pa, pa, size);
    return (void *)org;
}
void *
mmio_remap_last_region(physaddr_t pa, void *addr, size_t oldsz, size_t newsz) {
    return mmio_map_region(pa, newsz);
}
#endif

struct Timer timertab[MAX_TIMERS];
struct Timer *timer_for_schedule;

struct Timer timer_hpet0 = {
        .timer_name = "hpet0",
        .timer_init = hpet_init,
        .get_cpu_freq = hpet_cpu_frequency,
        .enable_interrupts = hpet_enable_interrupts_tim0,
        .handle_interrupts = hpet_handle_interrupts_tim0,
};

struct Timer timer_hpet1 = {
        .timer_name = "hpet1",
        .timer_init = hpet_init,
        .get_cpu_freq = hpet_cpu_frequency,
        .enable_interrupts = hpet_enable_interrupts_tim1,
        .handle_interrupts = hpet_handle_interrupts_tim1,
};

struct Timer timer_acpipm = {
        .timer_name = "pm",
        .timer_init = acpi_enable,
        .get_cpu_freq = pmtimer_cpu_frequency,
};

void
acpi_enable(void) {
    FADT *fadt = get_fadt();
    outb(fadt->SMI_CommandPort, fadt->AcpiEnable);
    while ((inw(fadt->PM1aControlBlock) & 1) == 0) /* nothing */
        ;
}

static bool
check_sum(int8_t *data, size_t len) {
    int8_t sum = 0;
    for (int8_t *end = data + len; data < end; data++) {
        sum += *data;
    }
    return sum == 0;
}

static void *
acpi_find_table(const char *sign) {
    /*
     * This function performs lookup of ACPI table by its signature
     * and returns valid pointer to the table mapped somewhere.
     *
     * It is a good idea to checksum tables before using them.
     *
     * HINT: Use mmio_map_region/mmio_remap_last_region
     * before accessing table addresses
     * (Why mmio_remap_last_region is requrired?)
     * HINT: RSDP address is stored in uefi_lp->ACPIRoot
     * HINT: You may want to distunguish RSDT/XSDT
     */
    // LAB 5: Your code here:
    RSDP *rsdp = (void *) uefi_lp->ACPIRoot;
    rsdp = mmio_map_region((uintptr_t) rsdp, sizeof *rsdp);
    assert(strncmp(rsdp->Signature, "RSD PTR ", 8) == 0 &&
        check_sum((void *) rsdp, 20) && check_sum((void *) rsdp, rsdp->Length) &&
        rsdp->Revision == 2);

    ACPISDTHeader *xsdt_phys;
    if ((xsdt_phys = (void *) rsdp->XsdtAddress)) {
        ACPISDTHeader *xsdt = mmio_map_region((uintptr_t) xsdt_phys, sizeof *xsdt_phys);
        assert(strncmp(xsdt->Signature, "XSDT", 4) == 0 &&
            check_sum((void *) xsdt, xsdt->Length));

        if (xsdt->Length > PAGE_SIZE) {
            xsdt = mmio_remap_last_region((uintptr_t) xsdt_phys, xsdt, sizeof *xsdt_phys, xsdt->Length);
        }

        for (ACPISDTHeader **entry = (void *) &xsdt[1], **end = (void *) xsdt + xsdt->Length; 
             entry < end;
             entry++) {
            ACPISDTHeader *table =  mmio_map_region((uintptr_t) *entry, sizeof **entry);
            if (strncmp(table->Signature, sign, 4) == 0) {
                if (table->Length > PAGE_SIZE) {
                    table = mmio_remap_last_region((uintptr_t) *entry, table, sizeof *table, table->Length);
                }
                return table;
            }
        }

    }

    cprintf("ERROR: XSDT is not present or the required signature (%s) was not found\n", sign);
    return NULL;
}

MCFG *
get_mcfg(void) {
    static MCFG *kmcfg;
    if (!kmcfg) {
        struct AddressSpace *as = switch_address_space(&kspace);
        kmcfg = acpi_find_table("MCFG");
        switch_address_space(as);
    }

    return kmcfg;
}

#define MAX_SEGMENTS 16

uintptr_t
make_fs_args(char *ustack_top) {

    MCFG *mcfg = get_mcfg();
    if (!mcfg) {
        cprintf("MCFG table is absent!");
        return (uintptr_t)ustack_top;
    }

    char *argv[MAX_SEGMENTS + 3] = {0};

    /* Store argv strings on stack */

    ustack_top -= 3;
    argv[0] = ustack_top;
    nosan_memcpy(argv[0], "fs", 3);

    int nent = (mcfg->h.Length - sizeof(MCFG)) / sizeof(CSBAA);
    if (nent > MAX_SEGMENTS)
        nent = MAX_SEGMENTS;

    for (int i = 0; i < nent; i++) {
        CSBAA *ent = &mcfg->Data[i];

        char arg[64];
        snprintf(arg, sizeof(arg) - 1, "ecam=%llx:%04x:%02x:%02x",
                 (long long)ent->BaseAddress, ent->SegmentGroup, ent->StartBus, ent->EndBus);

        int len = strlen(arg) + 1;
        ustack_top -= len;
        nosan_memcpy(ustack_top, arg, len);
        argv[i + 1] = ustack_top;
    }

    char arg[64];
    snprintf(arg, sizeof(arg) - 1, "tscfreq=%llx", (long long)tsc_calibrate());
    int len = strlen(arg) + 1;
    ustack_top -= len;
    nosan_memcpy(ustack_top, arg, len);
    argv[nent + 1] = ustack_top;

    /* Realign stack */
    ustack_top = (char *)((uintptr_t)ustack_top & ~(2 * sizeof(void *) - 1));

    /* Copy argv vector */
    ustack_top -= (nent + 3) * sizeof(void *);
    nosan_memcpy(ustack_top, argv, (nent + 3) * sizeof(argv[0]));

    char **argv_arg = (char **)ustack_top;
    long argc_arg = nent + 2;

    /* Store argv and argc arguemnts on stack */
    ustack_top -= sizeof(void *);
    nosan_memcpy(ustack_top, &argv_arg, sizeof(argv_arg));
    ustack_top -= sizeof(void *);
    nosan_memcpy(ustack_top, &argc_arg, sizeof(argc_arg));

    /* and return new stack pointer */
    return (uintptr_t)ustack_top;
}

/* Obtain and map FADT ACPI table address. */
FADT *
get_fadt(void) {
    // LAB 5: Your code here
    // (use acpi_find_table)
    // HINT: ACPI table signatures are
    //       not always as their names
    return acpi_find_table("FACP");
}

/* Obtain and map RSDP ACPI table address. */
HPET *
get_hpet(void) {
    // LAB 5: Your code here
    // (use acpi_find_table)
    return acpi_find_table("HPET");
}

/* Getting physical HPET timer address from its table. */
HPETRegister *
hpet_register(void) {
    HPET *hpet_timer = get_hpet();
    if (!hpet_timer->address.address) panic("hpet is unavailable\n");

    uintptr_t paddr = hpet_timer->address.address;
    return mmio_map_region(paddr, sizeof(HPETRegister));
}

/* Debug HPET timer state. */
void
hpet_print_struct(void) {
    HPET *hpet = get_hpet();
    assert(hpet != NULL);
    cprintf("signature = %s\n", (hpet->h).Signature);
    cprintf("length = %08x\n", (hpet->h).Length);
    cprintf("revision = %08x\n", (hpet->h).Revision);
    cprintf("checksum = %08x\n", (hpet->h).Checksum);

    cprintf("oem_revision = %08x\n", (hpet->h).OEMRevision);
    cprintf("creator_id = %08x\n", (hpet->h).CreatorID);
    cprintf("creator_revision = %08x\n", (hpet->h).CreatorRevision);

    cprintf("hardware_rev_id = %08x\n", hpet->hardware_rev_id);
    cprintf("comparator_count = %08x\n", hpet->comparator_count);
    cprintf("counter_size = %08x\n", hpet->counter_size);
    cprintf("reserved = %08x\n", hpet->reserved);
    cprintf("legacy_replacement = %08x\n", hpet->legacy_replacement);
    cprintf("pci_vendor_id = %08x\n", hpet->pci_vendor_id);
    cprintf("hpet_number = %08x\n", hpet->hpet_number);
    cprintf("minimum_tick = %08x\n", hpet->minimum_tick);

    cprintf("address_structure:\n");
    cprintf("address_space_id = %08x\n", (hpet->address).address_space_id);
    cprintf("register_bit_width = %08x\n", (hpet->address).register_bit_width);
    cprintf("register_bit_offset = %08x\n", (hpet->address).register_bit_offset);
    cprintf("address = %08lx\n", (unsigned long)(hpet->address).address);
}

static volatile HPETRegister *hpetReg;
/* HPET timer period (in femtoseconds) */
static uint64_t hpetFemto = 0;
/* HPET timer frequency */
static uint64_t hpetFreq = 0;

/* HPET timer initialisation */
void
hpet_init() {
    if (hpetReg == NULL) {
        nmi_disable();
        hpetReg = hpet_register();
        uint64_t cap = hpetReg->GCAP_ID;
        hpetFemto = (uintptr_t)(cap >> 32);
        if (!(cap & HPET_LEG_RT_CAP)) panic("HPET has no LegacyReplacement mode");

        // cprintf("hpetFemto = %llu\n", hpetFemto);
        hpetFreq = (1 * Peta) / hpetFemto;
        // cprintf("HPET: Frequency = %d.%03dMHz\n", (uintptr_t)(hpetFreq / Mega), (uintptr_t)(hpetFreq % Mega));
        /* Enable ENABLE_CNF bit to enable timer */
        hpetReg->GEN_CONF |= HPET_ENABLE_CNF;
        nmi_enable();
    }
}

/* HPET register contents debugging. */
void
hpet_print_reg(void) {
    cprintf("GCAP_ID = %016lx\n", (unsigned long)hpetReg->GCAP_ID);
    cprintf("GEN_CONF = %016lx\n", (unsigned long)hpetReg->GEN_CONF);
    cprintf("GINTR_STA = %016lx\n", (unsigned long)hpetReg->GINTR_STA);
    cprintf("MAIN_CNT = %016lx\n", (unsigned long)hpetReg->MAIN_CNT);
    cprintf("TIM0_CONF = %016lx\n", (unsigned long)hpetReg->TIM0_CONF);
    cprintf("TIM0_COMP = %016lx\n", (unsigned long)hpetReg->TIM0_COMP);
    cprintf("TIM0_FSB = %016lx\n", (unsigned long)hpetReg->TIM0_FSB);
    cprintf("TIM1_CONF = %016lx\n", (unsigned long)hpetReg->TIM1_CONF);
    cprintf("TIM1_COMP = %016lx\n", (unsigned long)hpetReg->TIM1_COMP);
    cprintf("TIM1_FSB = %016lx\n", (unsigned long)hpetReg->TIM1_FSB);
    cprintf("TIM2_CONF = %016lx\n", (unsigned long)hpetReg->TIM2_CONF);
    cprintf("TIM2_COMP = %016lx\n", (unsigned long)hpetReg->TIM2_COMP);
    cprintf("TIM2_FSB = %016lx\n", (unsigned long)hpetReg->TIM2_FSB);
}

/* HPET main timer counter value. */
uint64_t
hpet_get_main_cnt(void) {
    return hpetReg->MAIN_CNT;
}

/* - Configure HPET timer 0 to trigger every 0.5 seconds on IRQ_TIMER line
 * - Configure HPET timer 1 to trigger every 1.5 seconds on IRQ_CLOCK line
 *
 * HINT To be able to use HPET as PIT replacement consult
 *      LegacyReplacement functionality in HPET spec.
 * HINT Don't forget to unmask interrupt in PIC */
void
hpet_enable_interrupts_tim0(void) {
    // LAB 5: Your code here
    hpetReg->GEN_CONF &= ~HPET_ENABLE_CNF;
    hpetReg->MAIN_CNT = 0;

    hpetReg->TIM0_CONF |= HPET_TN_VAL_SET_CNF | HPET_TN_TYPE_CNF | HPET_TN_INT_ENB_CNF;
    hpetReg->TIM0_COMP = hpetFreq / 2;

    hpetReg->GEN_CONF |= HPET_ENABLE_CNF | HPET_LEG_RT_CNF;
    pic_irq_unmask(IRQ_TIMER);
}

void
hpet_enable_interrupts_tim1(void) {
    // LAB 5: Your code here
    hpetReg->GEN_CONF &= ~HPET_ENABLE_CNF;
    hpetReg->MAIN_CNT = 0;

    hpetReg->TIM1_CONF |= HPET_TN_VAL_SET_CNF | HPET_TN_TYPE_CNF;
    hpetReg->TIM1_COMP = hpetFreq + hpetFreq / 2;

    hpetReg->GEN_CONF |= HPET_ENABLE_CNF | HPET_LEG_RT_CNF;
    pic_irq_unmask(IRQ_CLOCK);
}

void
hpet_handle_interrupts_tim0(void) {
    pic_send_eoi(IRQ_TIMER);
}

void
hpet_handle_interrupts_tim1(void) {
    pic_send_eoi(IRQ_CLOCK);
}

/* Calculate CPU frequency in Hz with the help with HPET timer.
 * HINT Use hpet_get_main_cnt function and do not forget about
 * about pause instruction. */
uint64_t
hpet_cpu_frequency(void) {
    static uint64_t cpu_freq = 0;

    // LAB 5: Your code here
    if (!cpu_freq) {
        // we will wait for 200ms
        uint64_t wait_for = hpetFreq / 5;
        uint64_t tsc;

        uint64_t next_hpet = hpet_get_main_cnt() + wait_for;
        tsc = read_tsc();
        while (hpet_get_main_cnt() < next_hpet) asm ("pause");

        cpu_freq = (read_tsc() - tsc) * 5;
    }    

    return cpu_freq;
}

uint32_t
pmtimer_get_timeval(void) {
    FADT *fadt = get_fadt();
    return inl(fadt->PMTimerBlock);
}

static inline uint32_t
like_24(uint32_t val) {
    return val & (~0U >> 8);
}

/* Calculate CPU frequency in Hz with the help with ACPI PowerManagement timer.
 * HINT Use pmtimer_get_timeval function and do not forget that ACPI PM timer
 *      can be 24-bit or 32-bit. */
uint64_t
pmtimer_cpu_frequency(void) {
    static uint64_t cpu_freq = 0;

    // LAB 5: Your code here
    if (!cpu_freq) {
        uint64_t tsc;
        uint32_t limit;
        // we will wait for 200ms
        const uint32_t wait_for = PM_FREQ / 5;

        uint32_t pm_cur = pmtimer_get_timeval();
        tsc = read_tsc();

        pm_cur = like_24(pm_cur);
        limit = like_24(pm_cur + wait_for);

        while (like_24(pmtimer_get_timeval()) > limit) asm ("pause");
        while (like_24(pmtimer_get_timeval()) < limit) asm ("pause");
        
        cpu_freq = (read_tsc() - tsc) * 5;
    }

    return cpu_freq;
}












