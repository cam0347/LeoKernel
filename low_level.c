#include <include/low_level.h>
#include <include/types.h>

__attribute__((always_inline))
inline void sys_hlt() {
  asm volatile("cli");
  asm volatile("hlt");
}

__attribute__((always_inline))
inline void disable_int() {
  asm volatile("cli");
}

__attribute__((always_inline))
inline void enable_int() {
  asm volatile("sti");
}

uint64_t get_cr3() {
  uint64_t cr3;
  asm volatile("mov %%cr3, %0" : "=r"(cr3));
  return cr3;
}

uint64_t get_cr2() {
  uint64_t cr2;
  asm volatile("mov %%cr2, %0" : "=r"(cr2));
  return cr2;
}

void cpuid(uint32_t leaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
  asm volatile("mov %0, %%eax" : : "r"(leaf));
  asm volatile("cpuid");

  if (eax != null) {asm volatile("mov %%eax, %0" : "=r"(*eax));}
  if (ebx != null) {asm volatile("mov %%ebx, %0" : "=r"(*ebx));}
  if (ecx != null) {asm volatile("mov %%ecx, %0" : "=r"(*ecx));}
  if (edx != null) {asm volatile("mov %%edx, %0" : "=r"(*edx));}
}

void get_msr(uint32_t msr, uint32_t *lo, uint32_t *hi) {
  asm volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}
 
void set_msr(uint32_t msr, uint32_t lo, uint32_t hi) {
  asm volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}