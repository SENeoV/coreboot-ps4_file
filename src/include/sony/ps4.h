/*
 * Sony PS4 Hardware Definitions
 */

#ifndef SONY_PS4_H
#define SONY_PS4_H

#include <stdint.h>
#include <arch/io.h>

/* PS4 Model Types */
enum ps4_model {
    PS4_MODEL_FAT,      // CUH-1000 series
    PS4_MODEL_SLIM,     // CUH-2000 series
    PS4_MODEL_PRO,      // CUH-7000 series
};

/* Southbridge Types */
enum ps4_southbridge {
    SOUTHBRIDGE_AEOLIA,     // Fat models
    SOUTHBRIDGE_BAIKAL,     // Slim models  
    SOUTHBRIDGE_PROSPERO,   // Pro models
};

/* Memory Map */
#define PS4_DRAM_BASE       0x80000000ULL
#define PS4_DRAM_SIZE       0x200000000ULL  // 8GB

/* Aeolia Southbridge Registers (Fat model) */
#define AEOLIA_PCIE_BASE     0xe0000000
#define AEOLIA_XHCI_BASE     0xe8000000
#define AEOLIA_AHCI_BASE     0xe9000000
#define AEOLIA_UART_BASE     0xd0340000
#define AEOLIA_GPIO_BASE     0xef000000
#define AEOLIA_SMBUS_BASE    0xef100000

/* UART Registers (16550-compatible) */
#define AEOLIA_UART_RBR      (AEOLIA_UART_BASE + 0x00)  // RX buffer
#define AEOLIA_UART_THR      (AEOLIA_UART_BASE + 0x00)  // TX holding
#define AEOLIA_UART_DLL      (AEOLIA_UART_BASE + 0x00)  // Divisor LSB
#define AEOLIA_UART_IER      (AEOLIA_UART_BASE + 0x04)  // Interrupt enable
#define AEOLIA_UART_DLM      (AEOLIA_UART_BASE + 0x04)  // Divisor MSB
#define AEOLIA_UART_FCR      (AEOLIA_UART_BASE + 0x08)  // FIFO control
#define AEOLIA_UART_LCR      (AEOLIA_UART_BASE + 0x0C)  // Line control
#define AEOLIA_UART_MCR      (AEOLIA_UART_BASE + 0x10)  // Modem control
#define AEOLIA_UART_LSR      (AEOLIA_UART_BASE + 0x14)  // Line status
#define AEOLIA_UART_MSR      (AEOLIA_UART_BASE + 0x18)  // Modem status
#define AEOLIA_UART_SCR      (AEOLIA_UART_BASE + 0x1C)  // Scratch

/* GPIO Bits */
#define AEOLIA_GPIO_LED_POWER    (1 << 0)  // Blue power LED
#define AEOLIA_GPIO_LED_EJECT    (1 << 1)  // White eject LED
#define AEOLIA_GPIO_LED_WIFI     (1 << 2)  // WiFi activity
#define AEOLIA_GPIO_BTN_POWER    (1 << 8)  // Power button
#define AEOLIA_GPIO_BTN_EJECT    (1 << 9)  // Eject button
#define AEOLIA_GPIO_BTN_RESET    (1 << 10) // Reset button

/* PS4 Boot Protocol */
struct ps4_boot_params {
    uint64_t signature;      // "PS4BOOT0"
    uint32_t version;        // Boot protocol version
    uint32_t model;          // PS4 model (fat/slim/pro)
    uint32_t southbridge;    // Southbridge type
    uint32_t reserved;
    uint64_t memory_base;    // Physical memory base
    uint64_t memory_size;    // Total memory size
    uint64_t kernel_addr;    // Kernel load address
    uint64_t initrd_addr;    // Initrd load address
    uint64_t initrd_size;    // Initrd size
    uint64_t dtb_addr;       // Device tree address
    uint64_t cmdline_addr;   // Command line address
    uint32_t boot_flags;     // Boot flags
    uint32_t boot_dev;       // Boot device (0=USB, 1=SATA)
} __attribute__((packed));

/* PS4-specific ACPI tables */
struct ps4_acpi_header {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed));

struct ps4_acpi_southbridge {
    struct ps4_acpi_header header;
    uint32_t southbridge_type;
    uint64_t mmio_base;
    uint32_t irq_line;
    uint32_t usb_ports;
    uint32_t sata_ports;
    uint64_t xhci_base;
    uint64_t ahci_base;
    uint64_t uart_base;
} __attribute__((packed));

/* Function prototypes */
void ps4_uart_init(void);
void ps4_uart_putc(char c);
void ps4_uart_puts(const char *str);
void ps4_led_set(uint32_t mask);
void ps4_led_clear(uint32_t mask);
int ps4_button_pressed(uint32_t mask);
void ps4_southbridge_init(void);
void ps4_memory_init(void);
int ps4_boot_linux(void *kernel, void *initrd, size_t initrd_size);

#endif /* SONY_PS4_H */