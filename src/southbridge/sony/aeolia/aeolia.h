/*
 * Aeolia Southbridge (PS4 Fat)
 */

#ifndef SOUTHBRIDGE_SONY_AEOLIA_H
#define SOUTHBRIDGE_SONY_AEOLIA_H

#include <sony/ps4.h>

/* Aeolia PCI device IDs */
#define AEOLIA_VID            0x1002  // AMD
#define AEOLIA_DID            0xAA00  // Aeolia

/* Aeolia PCI function numbers */
#define AEOLIA_FUNC_XHCI      0
#define AEOLIA_FUNC_AHCI      1
#define AEOLIA_FUNC_UART      2
#define AEOLIA_FUNC_GPIO      3
#define AEOLIA_FUNC_SMBUS     4

/* Aeolia MMIO regions */
#define AEOLIA_MMIO_SIZE      0x1000

/* Aeolia interrupt assignments */
#define AEOLIA_IRQ_XHCI       16
#define AEOLIA_IRQ_AHCI       17
#define AEOLIA_IRQ_UART       18
#define AEOLIA_IRQ_GPIO       19

/* Aeolia power management */
#define AEOLIA_PM_STATUS      0x80
#define AEOLIA_PM_CMD         0x84
#define AEOLIA_PM_WAKE        0x88

/* Aeolia configuration structure */
struct aeolia_config {
    /* USB configuration */
    struct {
        uint32_t base;
        uint8_t num_ports;
        uint8_t speed;        // 0=USB2, 1=USB3
        uint8_t reserved[2];
    } usb;
    
    /* SATA configuration */
    struct {
        uint32_t base;
        uint8_t num_ports;
        uint8_t gen;           // 1=SATA1, 2=SATA2, 3=SATA3
        uint8_t hotplug;
    } sata;
    
    /* UART configuration */
    struct {
        uint32_t base;
        uint32_t baud;
        uint8_t data_bits;
        uint8_t stop_bits;
        uint8_t parity;
    } uart;
    
    /* GPIO configuration */
    struct {
        uint32_t base;
        uint32_t led_polarity;
        uint32_t btn_polarity;
    } gpio;
    
    /* Power management */
    struct {
        uint8_t wake_on_usb;
        uint8_t wake_on_sata;
        uint8_t power_saving;
    } pm;
};

/* Function prototypes */
void aeolia_init(struct aeolia_config *cfg);
void aeolia_pci_init(void);
void aeolia_usb_init(uint32_t base, int num_ports);
void aeolia_sata_init(uint32_t base, int num_ports);
void aeolia_uart_init(uint32_t base, uint32_t baud);
void aeolia_gpio_init(uint32_t base);
void aeolia_pm_init(void);
int aeolia_boot_device_detect(void);
void aeolia_reset(void);
void aeolia_poweroff(void);

#endif /* SOUTHBRIDGE_SONY_AEOLIA_H */