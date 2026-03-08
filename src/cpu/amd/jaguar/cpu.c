/*
 * AMD Jaguar CPU for PS4
 */

#include <arch/io.h>
#include <console/console.h>
#include <cpu/cpu.h>
#include <cpu/x86/msr.h>
#include <cpu/x86/mtrr.h>
#include <cpu/x86/lapic.h>
#include <cpu/x86/cache.h>
#include "cpu.h"

/* MSR definitions for Jaguar */
#define MSR_PSTATE_CUR_LIMIT   0xC0010061
#define MSR_PSTATE_CTL         0xC0010062
#define MSR_PSTATE_STATUS      0xC0010063
#define MSR_PSTATE0            0xC0010064
#define MSR_PSTATE1            0xC0010065
#define MSR_PSTATE2            0xC0010066
#define MSR_PSTATE3            0xC0010067
#define MSR_PSTATE4            0xC0010068
#define MSR_PSTATE5            0xC0010069
#define MSR_PSTATE6            0xC001006A
#define MSR_PSTATE7            0xC001006B
#define MSR_HWCR               0xC0010015
#define MSR_IC_CFG             0xC0011021

/* CPU configuration */
struct jaguar_cpu_config {
    uint32_t num_cores;
    uint32_t freq_mhz;        // Core frequency
    uint32_t voltage_mv;       // Core voltage
    uint32_t pstate_enabled;
};

static struct jaguar_cpu_config cpu_config = {
    .num_cores = 8,            // Jaguar has 8 cores
    .freq_mhz = 1600,          // 1.6 GHz
    .voltage_mv = 1100,        // 1.1V
    .pstate_enabled = 1,
};

/* Initialize CPU */
static void jaguar_cpu_init(struct device *dev)
{
    msr_t msr;
    uint32_t eax, ebx, ecx, edx;
    
    printk(BIOS_INFO, "Jaguar: Initializing CPU\n");
    
    // Get CPU info
    asm volatile ("cpuid" 
                  : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                  : "a"(0x80000000));
    
    if (eax >= 0x80000004) {
        char brand[48];
        asm volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) 
                     : "a"(0x80000002));
        ((uint32_t *)brand)[0] = eax;
        ((uint32_t *)brand)[1] = ebx;
        ((uint32_t *)brand)[2] = ecx;
        ((uint32_t *)brand)[3] = edx;
        
        asm volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) 
                     : "a"(0x80000003));
        ((uint32_t *)brand)[4] = eax;
        ((uint32_t *)brand)[5] = ebx;
        ((uint32_t *)brand)[6] = ecx;
        ((uint32_t *)brand)[7] = edx;
        
        asm volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) 
                     : "a"(0x80000004));
        ((uint32_t *)brand)[8] = eax;
        ((uint32_t *)brand)[9] = ebx;
        ((uint32_t *)brand)[10] = ecx;
        ((uint32_t *)brand)[11] = edx;
        
        printk(BIOS_INFO, "CPU: %s\n", brand);
    }
    
    // Configure P-states if enabled
    if (cpu_config.pstate_enabled) {
        // Set P-state 0 (max performance)
        msr.hi = 0;
        msr.lo = (cpu_config.freq_mhz << 8) | cpu_config.voltage_mv;
        wrmsr(MSR_PSTATE0, msr);
        
        // Enable P-states
        msr = rdmsr(MSR_PSTATE_CUR_LIMIT);
        msr.lo |= 0x1;  // Enable P-states
        wrmsr(MSR_PSTATE_CUR_LIMIT, msr);
        
        // Set P-state 0 as current
        msr.lo = 0;
        msr.hi = 0;
        wrmsr(MSR_PSTATE_CTL, msr);
    }
    
    // Enable caching
    x86_enable_cache();
    
    // Enable LAPIC
    enable_lapic();
    
    printk(BIOS_INFO, "Jaguar: CPU initialized (%d cores at %d MHz)\n",
           cpu_config.num_cores, cpu_config.freq_mhz);
}

/* Initialize each core */
static void jaguar_core_init(struct device *dev)
{
    printk(BIOS_DEBUG, "Jaguar: Initializing core\n");
    
    // Enable local APIC
    setup_lapic();
    
    // Set up MTRRs
    x86_setup_mtrrs(36);
}

/* Device operations */
static struct device_operations jaguar_cpu_ops = {
    .init = jaguar_cpu_init,
};

static struct device_operations jaguar_core_ops = {
    .init = jaguar_core_init,
};

/* CPU device IDs */
static const struct cpu_driver jaguar_cpu_driver __cpu_driver = {
    .ops = &jaguar_cpu_ops,
    .vendor = X86_VENDOR_AMD,
    .device = 0x700,  // Jaguar family
};

/* Lapic device */
static const struct cpu_driver jaguar_lapic_driver __cpu_driver = {
    .ops = &jaguar_core_ops,
    .vendor = X86_VENDOR_ANY,
    .device = LAPIC_DEVICE_ID,
};