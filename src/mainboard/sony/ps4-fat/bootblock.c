/*
 * PS4 Fat Bootblock - Early initialization
 */

#include <bootblock_common.h>
#include <arch/io.h>
#include <arch/cpu.h>
#include <console/console.h>
#include <sony/ps4.h>
#include "mainboard.h"

/* Early UART for debugging before memory init */
static void early_uart_init(void)
{
    uint32_t base = AEOLIA_UART_BASE;
    
    // Minimal UART init for early debug
    outb(0x80, base + AEOLIA_UART_LCR);  // DLAB
    outb(0x01, base + AEOLIA_UART_DLL);  // 115200
    outb(0x00, base + AEOLIA_UART_DLM);
    outb(0x03, base + AEOLIA_UART_LCR);  // 8N1
    outb(0x07, base + AEOLIA_UART_FCR);  // FIFO on
    
    // Say hello
    const char *msg = "\n[PS4] Bootblock starting...\n";
    while (*msg) {
        while (!(inb(base + AEOLIA_UART_LSR) & 0x20));
        outb(*msg++, base + AEOLIA_UART_THR);
    }
}

/* Early CPU initialization */
static void early_cpu_init(void)
{
    uint32_t eax, ebx, ecx, edx;
    
    // Check CPU vendor
    asm volatile ("cpuid" 
                  : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                  : "a"(0));
    
    printk(BIOS_DEBUG, "CPU: %c%c%c%c\n", 
           ebx & 0xFF, (ebx >> 8) & 0xFF, 
           (ebx >> 16) & 0xFF, (ebx >> 24) & 0xFF);
    
    // Enable SSE
    asm volatile (
        "mov %%cr0, %%rax\n"
        "and $0xFFFB, %%ax\n"    // Clear CR0.EM
        "or $0x2, %%ax\n"         // Set CR0.MP
        "mov %%rax, %%cr0\n"
        "mov %%cr4, %%rax\n"
        "or $0x600, %%rax\n"      // Set CR4.OSFXSR and CR4.OSXMMEXCPT
        "mov %%rax, %%cr4\n"
        : : : "rax"
    );
}

/* Bootblock entry point */
void bootblock_mainboard_init(void)
{
    early_uart_init();
    printk(BIOS_INFO, "PS4 Fat (CUH-1004A) Bootblock\n");
    
    // Minimal Aeolia init
    outl(0, AEOLIA_GPIO_BASE);  // Turn off LEDs initially
    
    early_cpu_init();
    
    // Check power button
    if (inl(AEOLIA_GPIO_BASE) & AEOLIA_GPIO_BTN_POWER) {
        printk(BIOS_INFO, "Power button pressed\n");
    }
    
    printk(BIOS_INFO, "Bootblock complete, jumping to romstage...\n");
}