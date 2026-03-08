/*
 * AMD Belize (Liverpool) Northbridge for PS4
 */

#include <arch/io.h>
#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include "northbridge.h"

/* Northbridge PCI device IDs */
#define BELIZE_VID           0x1002  // AMD
#define BELIZE_DID_HOST      0x130A  // Host bridge
#define BELIZE_DID_GPU       0x130B  // GPU
#define BELIZE_DID_HDMI      0x130C  // HDMI audio

/* GPU configuration */
struct belize_gpu_config {
    uint32_t dc_enable;      // Display core enable
    uint32_t dpm_enable;     // Dynamic power management
    uint32_t cik_support;    // CIK support (for radeon driver)
    uint32_t mem_size;        // GPU memory size (128MB)
    uint64_t mem_base;        // GPU memory base
};

static struct belize_gpu_config gpu_config = {
    .dc_enable = 1,
    .dpm_enable = 1,
    .cik_support = 1,
    .mem_size = 0x8000000,    // 128MB
    .mem_base = 0xF8000000ULL,
};

/* Initialize host bridge */
static void belize_host_init(struct device *dev)
{
    printk(BIOS_DEBUG, "Belize: Initializing host bridge\n");
    
    // Set up PCIe root complex
    pci_write_config32(dev, 0x60, 0x80000000);  // Enable PCIe
    
    // Set up memory map
    pci_write_config32(dev, 0x68, 0x80000000);  // Memory base
    
    printk(BIOS_INFO, "Belize: Host bridge initialized\n");
}

/* Initialize GPU */
static void belize_gpu_init(struct device *dev)
{
    printk(BIOS_DEBUG, "Belize: Initializing GPU\n");
    
    // Enable GPU memory
    pci_write_config32(dev, 0x14, gpu_config.mem_base);
    pci_write_config32(dev, 0x18, gpu_config.mem_size);
    
    // Set GPU configuration
    uint32_t cfg = 0;
    if (gpu_config.dc_enable) cfg |= 0x1;
    if (gpu_config.dpm_enable) cfg |= 0x2;
    if (gpu_config.cik_support) cfg |= 0x4;
    
    pci_write_config32(dev, 0x70, cfg);
    
    // Enable VGA
    pci_write_config16(dev, 0x04, 0x3);  // I/O and memory enable
    
    printk(BIOS_INFO, "Belize: GPU initialized (mem at 0x%llx)\n", 
           gpu_config.mem_base);
}

/* Read resources */
static void belize_read_resources(struct device *dev)
{
    struct resource *resource;
    
    // MMIO region
    resource = new_resource(dev, 0x10);
    resource->base = 0xE0000000;
    resource->size = 0x20000000;
    resource->flags = IORESOURCE_MEM | IORESOURCE_RESERVE | 
                      IORESOURCE_FIXED | IORESOURCE_STORED;
    
    // I/O region
    resource = new_resource(dev, 0x14);
    resource->base = 0x3000;
    resource->size = 0x1000;
    resource->flags = IORESOURCE_IO | IORESOURCE_RESERVE | 
                      IORESOURCE_FIXED | IORESOURCE_STORED;
}

/* Set resources */
static void belize_set_resources(struct device *dev)
{
    struct resource *resource;
    
    for (resource = dev->resource_list; resource; resource = resource->next) {
        if (!(resource->flags & IORESOURCE_ASSIGNED))
            continue;
            
        pci_write_config32(dev, resource->index, resource->base);
    }
}

/* Enable device */
static void belize_enable(struct device *dev)
{
    uint16_t cmd = pci_read_config16(dev, PCI_COMMAND);
    cmd |= PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;
    pci_write_config16(dev, PCI_COMMAND, cmd);
}

/* Device operations */
static struct device_operations belize_host_ops = {
    .read_resources = belize_read_resources,
    .set_resources = belize_set_resources,
    .enable = belize_enable,
    .init = belize_host_init,
};

static struct device_operations belize_gpu_ops = {
    .read_resources = belize_read_resources,
    .set_resources = belize_set_resources,
    .enable = belize_enable,
    .init = belize_gpu_init,
};

static struct device_operations belize_hdmi_ops = {
    .read_resources = belize_read_resources,
    .set_resources = belize_set_resources,
    .enable = belize_enable,
};

/* PCI device IDs */
static const struct pci_driver belize_host_driver __pci_driver = {
    .ops = &belize_host_ops,
    .vendor = BELIZE_VID,
    .device = BELIZE_DID_HOST,
};

static const struct pci_driver belize_gpu_driver __pci_driver = {
    .ops = &belize_gpu_ops,
    .vendor = BELIZE_VID,
    .device = BELIZE_DID_GPU,
};

static const struct pci_driver belize_hdmi_driver __pci_driver = {
    .ops = &belize_hdmi_ops,
    .vendor = BELIZE_VID,
    .device = BELIZE_DID_HDMI,
};