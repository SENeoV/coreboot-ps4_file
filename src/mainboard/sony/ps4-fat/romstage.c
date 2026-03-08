/*
 * PS4 Fat Romstage - Memory initialization and early setup
 */

#include <arch/io.h>
#include <arch/cpu.h>
#include <console/console.h>
#include <cpu/x86/msr.h>
#include <cpu/x86/mtrr.h>
#include <cpu/x86/smm.h>
#include <device/pci.h>
#include <ramstage.h>
#include <romstage_common.h>
#include <sony/ps4.h>
#include <southbridge/sony/aeolia/aeolia.h>
#include "mainboard.h"

/* Memory timings for PS4 Fat (8GB GDDR5) */
struct ps4_mem_timings {
    uint32_t freq;          // Memory frequency in MHz
    uint32_t tCL;           // CAS latency
    uint32_t tRCD;          // RAS to CAS delay
    uint32_t tRP;           // RAS precharge
    uint32_t tRAS;          // Active to precharge delay
};

static const struct ps4_mem_timings mem_timings = {
    .freq = 5500,           // 5.5 GHz effective (GDDR5)
    .tCL = 12,
    .tRCD = 12,
    .tRP = 12,
    .tRAS = 28,
};

/* Memory map */
static const struct {
    uint64_t base;
    uint64_t size;
    uint32_t type;
} ps4_mem_regions[] = {
    { 0x80000000ULL, 0x780000000ULL, 1 },  // 8GB usable
    { 0xF8000000ULL, 0x8000000ULL, 2 },    // 128MB for GPU
    { 0x00000000ULL, 0x1000ULL, 3 },       // 4KB for BIOS
};

/* Initialize memory controller */
static void ps4_memory_controller_init(void)
{
    printk(BIOS_DEBUG, "Initializing memory controller...\n");
    
    // Set up MTRRs for caching
    msr_t msr;
    
    // Writeback for all memory
    msr.lo = 0x0606060606060606ULL & 0xFFFFFFFF;
    msr.hi = 0x0606060606060606ULL >> 32;
    wrmsr(MTRRphysBase_MSR(0), msr);
    
    msr.lo = ~((PS4_DRAM_SIZE - 1) & 0xFFFFFFFF);
    msr.hi = ~((PS4_DRAM_SIZE - 1) >> 32);
    wrmsr(MTRRphysMask_MSR(0), msr);
    
    // Enable MTRRs
    msr = rdmsr(MTRRdefType_MSR);
    msr.lo |= MTRRdefTypeEn | MTRRdefTypeFIX16K | MTRRdefTypeFIX4K;
    wrmsr(MTRRdefType_MSR, msr);
    
    printk(BIOS_INFO, "Memory: %d MB at 0x%llx\n", 
           (uint32_t)(PS4_DRAM_SIZE / 1024 / 1024),
           PS4_DRAM_BASE);
}

/* Quick memory test */
static void ps4_memory_test(void)
{
    volatile uint64_t *mem = (uint64_t *)PS4_DRAM_BASE;
    uint64_t pattern = 0xDEADBEEFDEADBEEFULL;
    int errors = 0;
    
    printk(BIOS_DEBUG, "Running quick memory test...\n");
    
    // Test first 1MB
    for (int i = 0; i < (1024 * 1024 / 8); i += 64) {
        mem[i] = pattern;
    }
    
    for (int i = 0; i < (1024 * 1024 / 8); i += 64) {
        if (mem[i] != pattern) {
            errors++;
        }
    }
    
    if (errors == 0) {
        printk(BIOS_INFO, "Memory test passed\n");
    } else {
        printk(BIOS_WARNING, "Memory test had %d errors\n", errors);
    }
}

/* Initialize PCIe root complex */
static void ps4_pcie_init(void)
{
    printk(BIOS_DEBUG, "Initializing PCIe root complex...\n");
    
    // Set up PCI configuration access
    outl(0x80000000, 0xCF8);
    uint32_t cfg = inl(0xCFC);
    
    printk(BIOS_INFO, "PCIe vendor/device: 0x%x\n", cfg);
    
    // Enable PCIe in northbridge
    pci_write_config32(PCI_DEV(0, 0, 0), 0x60, 0x80000000);
    
    printk(BIOS_INFO, "PCIe root complex initialized\n");
}

/* Romstage main entry */
void mainboard_romstage_entry(void)
{
    printk(BIOS_INFO, "\n========================================\n");
    printk(BIOS_INFO, "PS4 Fat Romstage\n");
    printk(BIOS_INFO, "========================================\n");
    
    // Initialize memory
    ps4_memory_controller_init();
    
    // Quick test
    ps4_memory_test();
    
    // Initialize PCIe
    ps4_pcie_init();
    
    // Minimal Aeolia setup for romstage
    aeolia_uart_init(AEOLIA_UART_BASE, 115200);
    
    // Detect boot device early
    int boot_dev = aeolia_boot_device_detect();
    if (boot_dev >= 0) {
        printk(BIOS_INFO, "Boot device: %s\n", 
               boot_dev == 0 ? "USB" : "SATA");
    }
    
    // Set up stack for ramstage
    void *ramstage_stack = (void *)(PS4_DRAM_BASE + 0x10000);
    void *ramstage_entry = &mainboard_ramstage_entry;
    
    printk(BIOS_INFO, "Jumping to ramstage at %p\n", ramstage_entry);
    
    // Jump to ramstage
    asm volatile (
        "mov %0, %%rsp\n"
        "mov %1, %%rax\n"
        "call *%%rax\n"
        : : "r"(ramstage_stack), "r"(ramstage_entry)
        : "memory"
    );
}