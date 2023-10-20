#pragma once
#include <include/bootp.h>

void kmain(struct leokernel_boot_params);
bool check_boot_param(struct leokernel_boot_params);
void print_title();
void fake_shell();