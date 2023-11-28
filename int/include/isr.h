#pragma once
#include <include/types.h>

struct x64_int_frame {
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t sp;
    uint64_t ss;
};

void print_int_frame(struct x64_int_frame *frame);
void isr_print(char *str, ...);
void int_exec_hooks(uint8_t int_n);
void int_00(struct x64_int_frame *frame);
void int_01(struct x64_int_frame *frame);
void int_02(struct x64_int_frame *frame);
void int_03(struct x64_int_frame *frame);
void int_04(struct x64_int_frame *frame);
void int_05(struct x64_int_frame *frame);
void int_06(struct x64_int_frame *frame);
void int_07(struct x64_int_frame *frame);
void int_08(struct x64_int_frame *frame);
void int_09(struct x64_int_frame *frame);
void int_0A(struct x64_int_frame *frame, uint64_t error);
void int_0B(struct x64_int_frame *frame, uint64_t error);
void int_0C(struct x64_int_frame *frame, uint64_t error);
void int_0D(struct x64_int_frame *frame, uint64_t error);
void int_0E(struct x64_int_frame *frame, uint64_t error);
void int_0F(struct x64_int_frame *frame);
void int_10(struct x64_int_frame *frame);
void int_11(struct x64_int_frame *frame, uint64_t error);
void int_12(struct x64_int_frame *frame);
void int_13(struct x64_int_frame *frame);
void int_14(struct x64_int_frame *frame);
void int_15(struct x64_int_frame *frame, uint64_t error);
void int_16(struct x64_int_frame *frame);
void irq0(struct x64_int_frame *frame);
void irq1(struct x64_int_frame *frame);
void irq2(struct x64_int_frame *frame);
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
void irq14(struct x64_int_frame *frame);
void irq15(struct x64_int_frame *frame);