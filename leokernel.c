#include <include/leokernel.h>
#include <include/bootp.h>
#include <include/low_level.h>
#include <mm/include/segmentation.h>
#include <mm/include/memory_manager.h>
#include <int/include/int.h>
#include <tty/include/tty.h>
#include <tty/include/def_colors.h>
#include <mm/include/paging.h>
#include <acpi/include/acpi_parser.h>
#include <io/include/pci.h>
#include <include/panic.h>
#include <int/include/apic.h>
#include <include/math.h>
#include <io/include/keyboard.h>
#include <io/include/files.h>
#include <io/include/pit.h>
#include <mm/include/obj_alloc.h>
#include <include/sleep.h>
#include <tty/include/term.h>

void kmain(struct leokernel_boot_params bootp) {
    //if the boot parameters are null, halt the cpu
    if (!check_boot_param(bootp)) {
        sys_hlt();
    }

    //we have to set tty_ready to false because for some reason it's not actually set to false
    extern bool tty_ready;
    tty_ready = false;

    //initialize the screen text mode emulator
    if (!init_tty(bootp)) {
        sys_hlt();
    }

    //launch_splashscreen();
    print_title();

    //set up GDT and minimal segments
    if (!setup_gdt()) {
        fail("error setting up GDT");
    }

    //sets up IDT and a bare bone interrupt handling
    if (!setup_interrupts()) {
        fail("error setting up interrupts");
    }

    /*
    reloads the segment registers with the new gdt entries.
    this must be called after setting up the interrupts because this function use int 0x16
    */
    gdt_load_segments();

    /*
    initialize memory manager (physical allocator, virtual allocator, object allocator, kernel heap).
    kernel heap functions can be called after this point.
    */
    if (!init_mm(bootp)) {
        fail("error setting up the memory manager");
    }

    if (!init_acpi(bootp)) {
        fail("error setting up hardware stuff");
    }

    //the apic has to be set after parsing acpi tables since the madt table contains the apic base address
    if (!init_apic()) {
        fail("error setting up APIC");
    }

    /* initialize system timers */
    init_pit();
    apic_timer_init();

    printf("Configuring PCI and PCIe...\n");
    if (!init_pci()) {
        fail("error configuring PCI and PCIe");
    }

    /* initialize file handling */
    if (!init_files()) {
        fail("error initializing file handling");
    }

    /* HERE GOES THE INITIALIZATION OF THE TERMINAL */

    /*
    inits keyboard input (by default set to PS/2).
    it also adds an ioapic redirection entry to map irq1 to isr 0x17 (see isr.c)
    */
    init_keyboard();
    init_terminal();
    
    while(true);
	sys_hlt();
}

void print_title() {
    tty_color_t prev = get_tty_char_fg();
    set_tty_char_fg(GREEN_COLOR);
    printf("*** CamOS v0.1 ***\n");
    set_tty_char_fg(prev);
}

//checks validity of boot parameters
bool check_boot_param(struct leokernel_boot_params bootp) {
    if (bootp.frame_buffer == null) {return false;}
    if (bootp.frame_buffer_size == 0) {return false;}
    if (bootp.video_height == 0) {return false;}
    if (bootp.video_width == 0) {return false;}
    if (bootp.video_pitch == 0) {return false;}
    if (bootp.font == null) {return false;}
    if (bootp.map == null) {return false;}
    if (bootp.map_size == 0) {return false;}
    if (bootp.xsdt == null) {return false;}
    return true;
}