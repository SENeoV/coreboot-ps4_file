/*
 * Aeolia Southbridge Core Implementation
 */

#include <arch/io.h>
#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <delay.h>
#include <string.h>
#include "aeolia.h"
#include <sony/ps4.h>

/* Static configuration */
static struct aeolia_config aeolia_cfg = {
    .usb = {
        .base = AEOLIA_XHCI_BASE,
        .num_ports = 4,
        .speed = 1,
    },
    .sata = {
        .base = AEOLIA_AHCI_BASE,
        .num_ports = 1,
        .gen = 3,
        .hotplug = 0,
    },
    .uart = {
        .base = AEOLIA_UART_BASE,
        .baud = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = 0,
    },
    .gpio = {
        .base = AEOLIA_GPIO_BASE,
        .led_polarity = 1,
        .btn_polarity = 1,
    },
    .pm = {
        .wake_on_usb = 1,
        .wake_on_sata = 0,
        .power_saving = 1,
    },
};

/* UART functions */
void ps4_uart_init(void)
{
    uint32_t base = aeolia_cfg.uart.base;
    uint32_t baud = aeolia_cfg.uart.baud;
    uint32_t divisor;
    
    // Calculate divisor for 1.8432 MHz clock
    divisor = 1843200 / (16 * baud);
    
    // Set DLAB
    outb(0x80, base + AEOLIA_UART_LCR);
    
    // Set baud rate
    outb(divisor & 0xFF, base + AEOLIA_UART_DLL);
    outb((divisor >> 8) & 0xFF, base + AEOLIA_UART_DLM);
    
    // 8N1
    outb(0x03, base + AEOLIA_UART_LCR);
    
    // Enable FIFOs
    outb(0x07, base + AEOLIA_UART_FCR);
    
    // Disable interrupts
    outb(0x00, base + AEOLIA_UART_IER);
    
    printk(BIOS_INFO, "Aeolia: UART initialized at 0x%x, %d baud\n", 
           base, baud);
}

void ps4_uart_putc(char c)
{
    uint32_t base = aeolia_cfg.uart.base;
    
    // Wait for THR empty
    while (!(inb(base + AEOLIA_UART_LSR) & 0x20));
    
    // Send character
    outb(c, base + AEOLIA_UART_THR);
    
    // Handle newline
    if (c == '\n') {
        while (!(inb(base + AEOLIA_UART_LSR) & 0x20));
        outb('\r', base + AEOLIA_UART_THR);
    }
}

void ps4_uart_puts(const char *str)
{
    while (*str) {
        ps4_uart_putc(*str++);
    }
}

/* GPIO functions */
void ps4_led_set(uint32_t mask)
{
    uint32_t base = aeolia_cfg.gpio.base;
    uint32_t val = inl(base);
    
    if (aeolia_cfg.gpio.led_polarity) {
        val |= mask;
    } else {
        val &= ~mask;
    }
    
    outl(val, base);
}

void ps4_led_clear(uint32_t mask)
{
    uint32_t base = aeolia_cfg.gpio.base;
    uint32_t val = inl(base);
    
    if (aeolia_cfg.gpio.led_polarity) {
        val &= ~mask;
    } else {
        val |= mask;
    }
    
    outl(val, base);
}

int ps4_button_pressed(uint32_t mask)
{
    uint32_t base = aeolia_cfg.gpio.base;
    uint32_t val = inl(base);
    
    if (aeolia_cfg.gpio.btn_polarity) {
        return (val & mask) != 0;
    } else {
        return (val & mask) == 0;
    }
}

/* USB initialization */
void aeolia_usb_init(uint32_t base, int num_ports)
{
    volatile uint32_t *xhci = (volatile uint32_t *)base;
    
    printk(BIOS_DEBUG, "Aeolia: Initializing USB 3.0 controller\n");
    
    // Reset controller
    xhci[0x00] = 0x2;  // USBCMD = reset
    udelay(1000);
    
    // Wait for reset to complete
    int timeout = 1000;
    while ((xhci[0x00] & 0x2) && timeout--) {
        udelay(100);
    }
    
    // Set page size (4KB)
    xhci[0x08 / 4] = 0x1;  // PAGE_SIZE = 4KB support
    
    // Enable controller
    xhci[0x00 / 4] = 0x1;  // USBCMD = run/stop
    xhci[0x38 / 4] = 0x1;  // CONFIG = enable slots
    
    // Initialize ports
    for (int port = 0; port < num_ports; port++) {
        volatile uint32_t *portsc = &xhci[0x400 / 4 + port * 4];
        printk(BIOS_SPEW, "Aeolia: USB port %d status 0x%x\n", 
               port, *portsc);
    }
    
    printk(BIOS_INFO, "Aeolia: USB initialized, %d ports\n", num_ports);
}

/* SATA initialization */
void aeolia_sata_init(uint32_t base, int num_ports)
{
    volatile uint32_t *ahci = (volatile uint32_t *)base;
    
    printk(BIOS_DEBUG, "Aeolia: Initializing SATA controller\n");
    
    // Enable AHCI
    ahci[0x04 / 4] |= 0x80000000;  // GHC.AE = 1
    
    // Reset controller
    ahci[0x04 / 4] |= 0x1;  // GHC.HR = 1
    udelay(1000);
    
    // Wait for reset
    int timeout = 1000;
    while ((ahci[0x04 / 4] & 0x1) && timeout--) {
        udelay(100);
    }
    
    // Check implemented ports
    uint32_t ports_impl = ahci[0x0C / 4];  // PI
    printk(BIOS_INFO, "Aeolia: SATA ports implemented: 0x%x\n", 
           ports_impl);
    
    // Initialize each port
    for (int port = 0; port < 32; port++) {
        if (ports_impl & (1 << port)) {
            volatile uint32_t *port_base = &ahci[0x100 / 4 + port * 0x20];
            uint32_t ssts = port_base[0];  // Port status
            
            if ((ssts & 0xF) == 0x3) {
                printk(BIOS_INFO, "Aeolia: SATA device found on port %d\n", 
                       port);
            }
        }
    }
    
    printk(BIOS_INFO, "Aeolia: SATA initialized\n");
}

/* GPIO initialization */
void aeolia_gpio_init(uint32_t base)
{
    printk(BIOS_DEBUG, "Aeolia: Initializing GPIO\n");
    
    // Set all LEDs off initially
    outl(0, base);
    
    // Configure direction (output for LEDs, input for buttons)
    outl(0x07, base + 0x04);  // Direction register
    
    printk(BIOS_INFO, "Aeolia: GPIO initialized\n");
}

/* PCIe initialization */
void aeolia_pci_init(void)
{
    printk(BIOS_DEBUG, "Aeolia: Initializing PCIe bridge\n");
    
    // Check vendor/device ID
    uint32_t vendor = pci_read_config32(PCI_DEV(0, 2, 0), PCI_VENDOR_ID);
    
    if ((vendor & 0xFFFF) != AEOLIA_VID) {
        printk(BIOS_ERR, "Aeolia: Invalid vendor ID 0x%x\n", vendor);
        return;
    }
    
    // Enable PCIe bridge
    pci_write_config16(PCI_DEV(0, 2, 0), PCI_COMMAND,
                       PCI_COMMAND_IO | PCI_COMMAND_MEMORY | 
                       PCI_COMMAND_MASTER);
    
    printk(BIOS_INFO, "Aeolia: PCIe bridge enabled\n");
}

/* Main Aeolia initialization */
void aeolia_init(struct aeolia_config *cfg)
{
    printk(BIOS_INFO, "\n========================================\n");
    printk(BIOS_INFO, "Aeolia Southbridge Initialization\n");
    printk(BIOS_INFO, "Model: PS4 Fat (CUH-1004A)\n");
    printk(BIOS_INFO, "========================================\n");
    
    // Copy configuration if provided
    if (cfg) {
        memcpy(&aeolia_cfg, cfg, sizeof(aeolia_cfg));
    }
    
    // Initialize PCIe
    aeolia_pci_init();
    
    // Initialize subsystems
    aeolia_usb_init(aeolia_cfg.usb.base, aeolia_cfg.usb.num_ports);
    aeolia_sata_init(aeolia_cfg.sata.base, aeolia_cfg.sata.num_ports);
    aeolia_uart_init(aeolia_cfg.uart.base, aeolia_cfg.uart.baud);
    aeolia_gpio_init(aeolia_cfg.gpio.base);
    
    // Turn on power LED
    ps4_led_set(AEOLIA_GPIO_LED_POWER);
    
    printk(BIOS_INFO, "Aeolia: Initialization complete\n\n");
}

/* Detect boot device */
int aeolia_boot_device_detect(void)
{
    printk(BIOS_INFO, "Aeolia: Detecting boot devices...\n");
    
    // Check USB first
    volatile uint32_t *xhci = (volatile uint32_t *)aeolia_cfg.usb.base;
    
    for (int port = 0; port < aeolia_cfg.usb.num_ports; port++) {
        volatile uint32_t *portsc = &xhci[0x400 / 4 + port * 4];
        
        if (*portsc & 0x1) {  // Device connected
            printk(BIOS_INFO, "Aeolia: USB device detected on port %d\n", 
                   port);
            return 0;  // USB boot device
        }
    }
    
    // Check SATA
    volatile uint32_t *ahci = (volatile uint32_t *)aeolia_cfg.sata.base;
    uint32_t ports_impl = ahci[0x0C / 4];
    
    for (int port = 0; port < 32; port++) {
        if (ports_impl & (1 << port)) {
            volatile uint32_t *port_base = &ahci[0x100 / 4 + port * 0x20];
            uint32_t ssts = port_base[0];
            
            if ((ssts & 0xF) == 0x3) {
                printk(BIOS_INFO, "Aeolia: SATA device found on port %d\n", 
                       port);
                return 1;  // SATA boot device
            }
        }
    }
    
    printk(BIOS_WARNING, "Aeolia: No boot device found\n");
    return -1;
}

/* Reset PS4 */
void aeolia_reset(void)
{
    printk(BIOS_INFO, "Aeolia: System reset requested\n");
    
    // Try GPIO reset first
    uint32_t base = aeolia_cfg.gpio.base;
    outl(inl(base) | (1 << 31), base);  // Assume reset bit
    
    // Fallback to PCI reset
    outb(0x06, 0xCF9);  // System reset
    
    // Should never reach here
    while (1) {
        asm volatile ("hlt");
    }
}

/* Power off PS4 */
void aeolia_poweroff(void)
{
    printk(BIOS_INFO, "Aeolia: System power off requested\n");
    
    // Try GPIO power off
    uint32_t base = aeolia_cfg.gpio.base;
    outl(inl(base) & ~AEOLIA_GPIO_LED_POWER, base);
    
    // ACPI power off would go here
    outw(0x2000, 0x604);  // ACPI power off
    
    while (1) {
        asm volatile ("hlt");
    }
}