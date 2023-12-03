#include <int/include/isr.h>
#include <include/types.h>
#include <tty/include/tty.h>
#include <include/low_level.h>
#include <include/panic.h>
#include <int/include/apic.h>
#include <io/include/keyboard.h>
#include <io/include/ps2_keyboard.h>
#include <io/include/port_io.h>
#include <mm/include/segmentation.h>
#include <int/include/int.h>

/*
0x00 	Division by zero
0x01 	Single-step interrupt
0x02 	NMI
0x03 	Breakpoint
0x04 	Overflow
0x05 	Bound Range Exceeded
0x06 	Invalid Opcode
0x07 	Coprocessor not available
0x08 	Double Fault
0x09 	Coprocessor Segment Overrun (386 or earlier only)
0x0A 	Invalid Task State Segment
0x0B 	Segment not present
0x0C 	Stack Segment Fault
0x0D 	General Protection Fault
0x0E 	Page Fault
0x0F 	reserved
0x10 	x87 Floating Point Exception
0x11 	Alignment Check
0x12 	Machine Check
0x13 	SIMD Floating-Point Exception
0x14 	Virtualization Exception
0x15 	Control Protection Exception
*/

extern void *isr_hooks[256][ISR_MAX_HOOKS]; //defined in int.c
uint64_t isrs[256] = {
    (uint64_t) int_00, (uint64_t) int_01, (uint64_t) int_02, (uint64_t) int_03, (uint64_t) int_04, (uint64_t) int_05,
    (uint64_t) int_06, (uint64_t) int_07, (uint64_t) int_08, (uint64_t) int_09, (uint64_t) int_0A, (uint64_t) int_0B,
    (uint64_t) int_0C, (uint64_t) int_0D, (uint64_t) int_0E, (uint64_t) int_0F, (uint64_t) int_10, (uint64_t) int_11,
    (uint64_t) int_12, (uint64_t) int_13, (uint64_t) int_14, (uint64_t) int_15, (uint64_t) int_16, (uint64_t) irq0,
    (uint64_t) irq1, (uint64_t) irq2, null, null, null, null, null, null, null, null, null, null, null, (uint64_t) irq14,
    (uint64_t) irq15
};

void print_int_frame(struct x64_int_frame *frame) {
    tty_color_t old_color = get_tty_char_fg();
    set_tty_char_fg(TTY_COLOR_WHITE);
    printf("Interrupt frame:\n");
    printf("RIP:      0x%lX\n", frame->rip);
    printf("CS:       %ld\n", frame->cs);
    printf("RFLAGS:   %ld\n", frame->rflags);
    printf("SP:       0x%lX\n", frame->sp);
    printf("SS:       %ld\n", frame->ss);
    set_tty_char_fg(old_color);
}

inline void int_exec_hooks(uint8_t int_n) {
    for (uint8_t i = 0; i < ISR_MAX_HOOKS; i++) {
        void (*handler)() = isr_hooks[int_n][i];

        if (handler) {
            (*handler)();
        }
    }
}

/*
regular interrupt service routines
*/

__attribute__((interrupt))
void int_00(struct x64_int_frame *frame) {
    printf("Division by zero\n");
    print_int_frame(frame);
    int_exec_hooks(0);
    sys_hlt();
}

__attribute__((interrupt))
void int_01(struct x64_int_frame *frame) {
    printf("Single step interrupt\n");
    print_int_frame(frame);
    int_exec_hooks(1);
    sys_hlt();
}

__attribute__((interrupt))
void int_02(struct x64_int_frame *frame) {
    printf("Non-maskable interrupt\n");
    print_int_frame(frame);
    int_exec_hooks(2);
    sys_hlt();
}

__attribute__((interrupt))
void int_03(struct x64_int_frame *frame) {
    printf("Breakpoint\n");
    print_int_frame(frame);
    int_exec_hooks(3);
    sys_hlt();
}

__attribute__((interrupt))
void int_04(struct x64_int_frame *frame) {
    printf("Overflow\n");
    print_int_frame(frame);
    int_exec_hooks(4);
    sys_hlt();
}

__attribute__((interrupt))
void int_05(struct x64_int_frame *frame) {
    printf("Bound range exceeded\n");
    print_int_frame(frame);
    int_exec_hooks(5);
    sys_hlt();
}

__attribute__((interrupt))
void int_06(struct x64_int_frame *frame) {
    print_int_frame(frame);
    int_exec_hooks(6);
    panic("invalid opcode");
    sys_hlt();
}

__attribute__((interrupt))
void int_07(struct x64_int_frame *frame) {
    printf("Coprocessor not available\n");
    print_int_frame(frame);
    int_exec_hooks(7);
    sys_hlt();
}

__attribute__((interrupt))
void int_08(struct x64_int_frame *frame) {
    printf("Double fault\n");
    print_int_frame(frame);
    int_exec_hooks(8);
    sys_hlt();
}

__attribute__((interrupt))
void int_09(struct x64_int_frame *frame) {
    printf("Coprocessor segment overrun\n");
    print_int_frame(frame);
    int_exec_hooks(9);
    sys_hlt();
}

__attribute__((interrupt))
void int_0A(struct x64_int_frame *frame, uint64_t error) {
    printf("Invalid TSS\n");
    print_int_frame(frame);
    printf("Segment selector index: %ld\n", error);
    int_exec_hooks(10);
    sys_hlt();
}

__attribute__((interrupt))
void int_0B(struct x64_int_frame *frame, uint64_t error) {
    printf("Segment not present\n");
    print_int_frame(frame);
    printf("Segment selector index: %ld\n", error);
    int_exec_hooks(11);
    sys_hlt();
}

__attribute__((interrupt))
void int_0C(struct x64_int_frame *frame, uint64_t error) {
    printf("Stack segment fault\n");
    print_int_frame(frame);

    if (error != 0) {
        printf("Stack segment elector index: %ld\n", error);
    }

    int_exec_hooks(12);
    sys_hlt();
}

__attribute__((interrupt))
void int_0D(struct x64_int_frame *frame, uint64_t error) {
    printf("General protection fault\n");
    print_int_frame(frame);

    if (error != 0) {
        printf("Segment selector index: %ld\n", error);
    } else {
        printf("not segment related\n");
    }

    int_exec_hooks(13);
    panic("general protection fault");
    sys_hlt();
}

__attribute__((interrupt))
void int_0E(struct x64_int_frame *frame, uint64_t error) {
    printf("Page fault\n");
    print_int_frame(frame);
    
    if (error & 1) {printf("page access rights violated\n");}

    if (error >> 1 & 1) {
        printf("write right ");
    } else {
        printf("read right ");
    }
    
    if (error >> 2 & 1) {
        printf("user mode ");
    }

    if (error >> 4 & 1) {
        printf("fetch ");
    }

    uint64_t page = get_cr2();
    printf("\naddress that caused the fault: 0x%x\n", page);
    int_exec_hooks(14);
    sys_hlt();
}

__attribute__((interrupt))
void int_0F(struct x64_int_frame *frame) {
    printf("Reserved\n");
    print_int_frame(frame);
    int_exec_hooks(15);
    sys_hlt();
}

__attribute__((interrupt))
void int_10(struct x64_int_frame *frame) {
    printf("x87 Floating Point Exception\n");
    print_int_frame(frame);
    int_exec_hooks(16);
    sys_hlt();
}

__attribute__((interrupt))
void int_11(struct x64_int_frame *frame, uint64_t error) {
    printf("Alignment check\n");
    print_int_frame(frame);
    int_exec_hooks(17);
    sys_hlt();
}

__attribute__((interrupt))
void int_12(struct x64_int_frame *frame) {
    printf("Machine check\n");
    print_int_frame(frame);
    int_exec_hooks(18);
    sys_hlt();
}

__attribute__((interrupt))
void int_13(struct x64_int_frame *frame) {
    printf("SIMD Floating-Point Exception\n");
    print_int_frame(frame);
    int_exec_hooks(19);
    sys_hlt();
}

__attribute__((interrupt))
void int_14(struct x64_int_frame *frame) {
    printf("Virtualization Exception\n");
    print_int_frame(frame);
    int_exec_hooks(20);
    sys_hlt();
}

__attribute__((interrupt))
void int_15(struct x64_int_frame *frame, uint64_t error) {
    printf("Control Protection Exception\n");
    print_int_frame(frame);
    int_exec_hooks(21);
    sys_hlt();
}

/*
this interrupt is used only to set the right segment registers after setting up the gdt and interrupts
*/
__attribute__((interrupt))
void int_16(struct x64_int_frame *frame) {
    frame->ss = GDT_KERNEL_DS * sizeof(gdt_t);
    frame->cs = GDT_KERNEL_CS * sizeof(gdt_t);
    int_exec_hooks(22);
}

/*
interrupt request handlers
*/

//system timer (PIT)(int 0x17)
__attribute__((interrupt))
void irq0(struct x64_int_frame *frame) {
    int_exec_hooks(23);
    enable_int();
    send_eoi();
}

//ps/2 keyboard (int 0x18)
__attribute__((interrupt))
void irq1(struct x64_int_frame *frame) {
    keypressed(ps2_in());
    int_exec_hooks(24);
    enable_int();
    send_eoi();
}

//system timer (APIC timer)(int 0x19)
__attribute__((interrupt))
void irq2(struct x64_int_frame *frame) {
    int_exec_hooks(25);
    enable_int();
    send_eoi();
}

void irq3();
void irq4();
void irq5();
void irq6();
void irq7();
void irq8();
void irq9();
void irq10();
void irq11();
void irq12();
void irq13();

//primary IDE bus
__attribute__((interrupt))
void irq14(struct x64_int_frame *frame) {
    printf("primary ide irq fired\n");
    int_exec_hooks(37);
    enable_int();
    send_eoi();
}

//secondary IDE bus
__attribute__((interrupt))
void irq15(struct x64_int_frame *frame) {
    printf("secondary ide irq fired\n");
    int_exec_hooks(38);
    enable_int();
    send_eoi();
}