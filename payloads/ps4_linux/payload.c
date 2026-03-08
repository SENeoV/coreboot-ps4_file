/*
 * PS4 Linux Payload for Coreboot
 */

#include <stdint.h>
#include <stddef.h>

#define CB_HEADER_MAGIC 0x4F4F4243  // "CBOO" in hex

struct cbfs_header {
    uint32_t magic;
    uint32_t version;
    uint32_t rom_size;
    uint32_t boot_block_size;
    uint32_t align;
    uint32_t offset;
    uint32_t architecture;
} __attribute__((packed));

struct cbfs_file {
    uint8_t magic[8];
    uint32_t len;
    uint32_t type;
    uint32_t checksum;
    uint32_t offset;
} __attribute__((packed));

void payload_entry(void *arg)
{
    // This is where the Linux kernel would be
    // In practice, Coreboot will load this payload
    // and jump to this entry point
    
    while (1);
}