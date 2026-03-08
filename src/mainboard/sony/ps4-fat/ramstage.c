/*
 * PS4 Fat Ramstage - Device enumeration and payload loading
 */

#include <arch/io.h>
#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <ramstage.h>
#include <string.h>
#include <cbfs.h>
#include <payload_loader.h>
#include <sony/ps4.h>
#include <southbridge/sony/aeolia/aeolia.h>
#include "mainboard.h"

/* Boot parameters for Linux */
static struct ps4_boot_params boot_params __attribute__((aligned(16)));

/* Payload structure */
struct payload {
    void *entry;
    size_t size;
    uint32_t type;
};

/* Load payload from CBFS */
static int load_payload(struct payload *payload, const char *name)
{
    size_t len;
    
    printk(BIOS_INFO, "Loading payload: %s\n", name);
    
    void *data = cbfs_boot_map_with_leak(name, CBFS_TYPE_RAW, &len);
    if (!data) {
        printk(BIOS_ERR, "Failed to load payload\n");
        return -1;
    }
    
    payload->entry = data;
    payload->size = len;
    payload->type = 0;  // Raw binary
    
    printk(BIOS_INFO, "Payload loaded: %zu bytes at %p\n", len, data);
    
    return 0;
}

/* Prepare device tree */
static void prepare_device_tree(void *fdt_addr)
{
    uint32_t *fdt = (uint32_t *)fdt_addr;
    
    printk(BIOS_DEBUG, "Preparing device tree\n");
    
    // Simple FDT header
    fdt[0] = 0xEDFE0DD0;  // Magic
    fdt[1] = 0x1000;       // Total size
    fdt[2] = 0x40;         // Structure offset
    fdt[3] = 0x800;        // Strings offset
    fdt[4] = 0x28;         // Memory reservation offset
    fdt[5] = 17;           // Version
    fdt[6] = 16;           // Last compatible version
    fdt[7] = 0;            // Boot CPU ID
    fdt[8] = 0x100;        // Strings size
    fdt[9] = 0x700;        // Structure size
    
    printk(BIOS_INFO, "Device tree prepared at %p\n", fdt_addr);
}

/* Prepare boot parameters */
static void prepare_boot_params(uint64_t kernel_addr, 
                                uint64_t initrd_addr,
                                uint64_t initrd_size,
                                uint64_t dtb_addr)
{
    const char *cmdline = 
        "ps4.southbridge=aeolia "
        "console=uart8250,mmio32,0xd0340000 "
        "pcie_aspm=off pci=realloc swiotlb=512 "
        "mem=8192M radeon.cik_support=1 "
        "amdgpu.cik_support=0 nosmt quiet";
    
    printk(BIOS_DEBUG, "Preparing boot parameters\n");
    
    memset(&boot_params, 0, sizeof(boot_params));
    
    boot_params.signature = 0x30544f4234505350ULL;  // "PS4BOOT0"
    boot_params.version = 0x100;
    boot_params.model = PS4_MODEL_FAT;
    boot_params.southbridge = SOUTHBRIDGE_AEOLIA;
    boot_params.memory_base = PS4_DRAM_BASE;
    boot_params.memory_size = PS4_DRAM_SIZE;
    boot_params.kernel_addr = kernel_addr;
    boot_params.initrd_addr = initrd_addr;
    boot_params.initrd_size = initrd_size;
    boot_params.dtb_addr = dtb_addr;
    boot_params.cmdline_addr = (uint64_t)cmdline;
    boot_params.boot_flags = 0x1;
    
    printk(BIOS_INFO, "Boot parameters prepared at %p\n", &boot_params);
}

/* Jump to Linux kernel */
static void jump_to_linux(void)
{
    void (*kernel_entry)(uint64_t rdi, uint64_t rsi, void *rdx);
    kernel_entry = (void *)boot_params.kernel_addr;
    
    printk(BIOS_INFO, "\n========================================\n");
    printk(BIOS_INFO, "Booting Linux kernel at %p\n", kernel_entry);
    printk(BIOS_INFO, "========================================\n\n");
    
    // Turn on eject LED to indicate boot
    ps4_led_set(AEOLIA_GPIO_LED_EJECT);
    
    // Flush caches
    asm volatile ("wbinvd");
    
    // Jump to kernel
    kernel_entry((uint64_t)&boot_params, 0, 0);
    
    // Should never return
    printk(BIOS_ERR, "Kernel returned!\n");
}

/* Main ramstage entry */
void mainboard_ramstage_entry(void)
{
    struct payload kernel;
    struct payload initrd;
    void *dtb_addr = (void *)0x9F000000;
    
    printk(BIOS_INFO, "\n========================================\n");
    printk(BIOS_INFO, "PS4 Fat Ramstage\n");
    printk(BIOS_INFO, "========================================\n");
    
    // Full Aeolia initialization
    aeolia_init(NULL);
    
    // Enumerate PCI devices
    printk(BIOS_INFO, "Enumerating PCI devices...\n");
    pci_enumerate();
    
    // Load kernel from CBFS
    if (load_payload(&kernel, "vmlinuz-ps4") != 0) {
        printk(BIOS_ERR, "Failed to load kernel\n");
        goto halt;
    }
    
    // Load initrd (optional)
    if (load_payload(&initrd, "initramfs-ps4.img") != 0) {
        printk(BIOS_WARNING, "No initrd found, continuing\n");
        initrd.size = 0;
    }
    
    // Prepare device tree
    prepare_device_tree(dtb_addr);
    
    // Prepare boot parameters
    prepare_boot_params((uint64_t)kernel.entry,
                        (uint64_t)initrd.entry,
                        initrd.size,
                        (uint64_t)dtb_addr);
    
    // Boot
    jump_to_linux();
    
halt:
    printk(BIOS_ERR, "System halted\n");
    while (1) {
        ps4_led_set(AEOLIA_GPIO_LED_EJECT);
        mdelay(500);
        ps4_led_clear(AEOLIA_GPIO_LED_EJECT);
        mdelay(500);
    }
}