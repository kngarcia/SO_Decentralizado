#include "e1000.h"
#include "../net/netif.h"
#include "../drivers/serial.h"
#include "../mm/physical_memory.h"
#include <stddef.h>

static e1000_dev_t e1000_device;

// Memory-mapped I/O helpers
static inline void e1000_write32(uint32_t reg, uint32_t val) {
    *((volatile uint32_t*)(e1000_device.mem_base + reg)) = val;
}

static inline uint32_t e1000_read32(uint32_t reg) {
    return *((volatile uint32_t*)(e1000_device.mem_base + reg));
}

// Read EEPROM
static uint16_t e1000_eeprom_read(uint8_t addr) {
    uint32_t tmp = 0;
    e1000_write32(E1000_REG_EERD, ((uint32_t)addr << 8) | 1);
    
    // Wait for read to complete
    while (!((tmp = e1000_read32(E1000_REG_EERD)) & (1 << 4)));
    
    return (uint16_t)((tmp >> 16) & 0xFFFF);
}

// Read MAC address from EEPROM
static void e1000_read_mac(mac_addr_t *mac) {
    uint16_t tmp;
    
    tmp = e1000_eeprom_read(0);
    mac->addr[0] = tmp & 0xFF;
    mac->addr[1] = (tmp >> 8) & 0xFF;
    
    tmp = e1000_eeprom_read(1);
    mac->addr[2] = tmp & 0xFF;
    mac->addr[3] = (tmp >> 8) & 0xFF;
    
    tmp = e1000_eeprom_read(2);
    mac->addr[4] = tmp & 0xFF;
    mac->addr[5] = (tmp >> 8) & 0xFF;
}

// Initialize receive descriptors
static int e1000_init_rx(void) {
    // Allocate descriptor ring
    e1000_device.rx_descs = (e1000_rx_desc_t*)(uint64_t)alloc_frame();
    if (!e1000_device.rx_descs) return -1;
    
    // Allocate and setup RX buffers
    for (int i = 0; i < E1000_NUM_RX_DESC; i++) {
        e1000_device.rx_buffers[i] = (uint8_t*)(uint64_t)alloc_frame();
        if (!e1000_device.rx_buffers[i]) return -1;
        
        e1000_device.rx_descs[i].addr = (uint64_t)e1000_device.rx_buffers[i];
        e1000_device.rx_descs[i].status = 0;
    }
    
    e1000_device.rx_cur = 0;
    
    // Setup RX descriptor registers
    e1000_write32(E1000_REG_RDBAL, (uint32_t)((uint64_t)e1000_device.rx_descs & 0xFFFFFFFF));
    e1000_write32(E1000_REG_RDBAH, (uint32_t)((uint64_t)e1000_device.rx_descs >> 32));
    e1000_write32(E1000_REG_RDLEN, E1000_NUM_RX_DESC * sizeof(e1000_rx_desc_t));
    e1000_write32(E1000_REG_RDH, 0);
    e1000_write32(E1000_REG_RDT, E1000_NUM_RX_DESC - 1);
    
    // Enable receiver
    e1000_write32(E1000_REG_RCTL, 
        E1000_RCTL_EN | E1000_RCTL_UPE | E1000_RCTL_MPE | 
        E1000_RCTL_BAM | E1000_RCTL_BSIZE_2048 | E1000_RCTL_SECRC);
    
    return 0;
}

// Initialize transmit descriptors
static int e1000_init_tx(void) {
    // Allocate descriptor ring
    e1000_device.tx_descs = (e1000_tx_desc_t*)(uint64_t)alloc_frame();
    if (!e1000_device.tx_descs) return -1;
    
    // Allocate and setup TX buffers
    for (int i = 0; i < E1000_NUM_TX_DESC; i++) {
        e1000_device.tx_buffers[i] = (uint8_t*)(uint64_t)alloc_frame();
        if (!e1000_device.tx_buffers[i]) return -1;
        
        e1000_device.tx_descs[i].addr = (uint64_t)e1000_device.tx_buffers[i];
        e1000_device.tx_descs[i].status = 1; // Mark as done
        e1000_device.tx_descs[i].cmd = 0;
    }
    
    e1000_device.tx_cur = 0;
    
    // Setup TX descriptor registers
    e1000_write32(E1000_REG_TDBAL, (uint32_t)((uint64_t)e1000_device.tx_descs & 0xFFFFFFFF));
    e1000_write32(E1000_REG_TDBAH, (uint32_t)((uint64_t)e1000_device.tx_descs >> 32));
    e1000_write32(E1000_REG_TDLEN, E1000_NUM_TX_DESC * sizeof(e1000_tx_desc_t));
    e1000_write32(E1000_REG_TDH, 0);
    e1000_write32(E1000_REG_TDT, 0);
    
    // Enable transmitter
    e1000_write32(E1000_REG_TCTL, E1000_TCTL_EN | E1000_TCTL_PSP);
    
    return 0;
}

int e1000_send_packet(netif_t *netif, const void *data, uint32_t len) {
    if (!data || len == 0 || len > 4096) return -1;
    
    e1000_tx_desc_t *desc = &e1000_device.tx_descs[e1000_device.tx_cur];
    
    // Wait if descriptor is busy
    while (!(desc->status & 1));
    
    // Copy data to TX buffer
    uint8_t *buf = e1000_device.tx_buffers[e1000_device.tx_cur];
    for (uint32_t i = 0; i < len; i++) {
        buf[i] = ((uint8_t*)data)[i];
    }
    
    // Setup descriptor
    desc->length = len;
    desc->cmd = (1 << 0) | (1 << 1) | (1 << 3); // EOP | IFCS | RS
    desc->status = 0;
    
    // Update tail pointer
    uint16_t old_cur = e1000_device.tx_cur;
    e1000_device.tx_cur = (e1000_device.tx_cur + 1) % E1000_NUM_TX_DESC;
    e1000_write32(E1000_REG_TDT, e1000_device.tx_cur);
    
    return len;
}

int e1000_recv_packet(netif_t *netif, void *buf, uint32_t max_len) {
    e1000_rx_desc_t *desc = &e1000_device.rx_descs[e1000_device.rx_cur];
    
    // Check if packet is available
    if (!(desc->status & 1)) return 0;
    
    // Copy packet data
    uint16_t len = desc->length;
    if (len > max_len) len = max_len;
    
    uint8_t *rx_buf = e1000_device.rx_buffers[e1000_device.rx_cur];
    for (uint16_t i = 0; i < len; i++) {
        ((uint8_t*)buf)[i] = rx_buf[i];
    }
    
    // Reset descriptor
    desc->status = 0;
    
    // Update tail pointer
    uint16_t old_cur = e1000_device.rx_cur;
    e1000_device.rx_cur = (e1000_device.rx_cur + 1) % E1000_NUM_RX_DESC;
    e1000_write32(E1000_REG_RDT, old_cur);
    
    return len;
}

int e1000_init(void) {
    extern void *mmio_map(uint64_t phys_addr, size_t size);
    
    serial_puts("[e1000] Initializing Intel E1000 NIC\n");
    
    /* Map E1000 MMIO region (hardcoded address, should detect via PCI) */
    void *mmio_base = mmio_map(0xFEBC0000, 0x20000);  /* 128KB */
    if (!mmio_base) {
        serial_puts("[e1000] ERROR: Failed to map MMIO region\n");
        return -1;
    }
    e1000_device.mem_base = (uint64_t)mmio_base;
    serial_puts("[e1000] MMIO mapped successfully\\n");
    
    // Quick device detection - read status register
    // If all bits are 0xFF, device is not present
    uint32_t status = e1000_read32(E1000_REG_STATUS);
    if (status == 0xFFFFFFFF || status == 0) {
        serial_puts("[e1000] WARNING: Device not detected (no hardware or QEMU missing -device e1000)\\n");
        return -1;
    }
    serial_puts("[e1000] Device detected\\n");
    
    // Reset device (with timeout)
    uint32_t ctrl = e1000_read32(E1000_REG_CTRL);
    e1000_write32(E1000_REG_CTRL, ctrl | E1000_CTRL_RST);
    
    // Wait for reset with timeout
    int timeout = 10000;
    while (timeout-- > 0) {
        ctrl = e1000_read32(E1000_REG_CTRL);
        if (!(ctrl & E1000_CTRL_RST)) break;
        for (volatile int i = 0; i < 100; i++);
    }
    
    if (timeout <= 0) {
        serial_puts("[e1000] WARNING: Reset timeout\\n");
        return -1;
    }
    serial_puts("[e1000] Reset complete\\n");
    
    // Link up
    ctrl = e1000_read32(E1000_REG_CTRL);
    e1000_write32(E1000_REG_CTRL, ctrl | E1000_CTRL_SLU | E1000_CTRL_ASDE);
    
    // Read MAC address
    e1000_read_mac(&e1000_device.mac);
    serial_puts("[e1000] MAC: ");
    for (int i = 0; i < 6; i++) {
        // Simple hex print (would use proper formatting in real code)
        serial_putc(e1000_device.mac.addr[i]);
        if (i < 5) serial_putc(':');
    }
    serial_putc('\n');
    
    // Initialize RX and TX
    if (e1000_init_rx() < 0) {
        serial_puts("[e1000] Failed to init RX\n");
        return -1;
    }
    
    if (e1000_init_tx() < 0) {
        serial_puts("[e1000] Failed to init TX\n");
        return -1;
    }
    
    // Setup network interface
    e1000_device.netif.send = e1000_send_packet;
    e1000_device.netif.recv = e1000_recv_packet;
    e1000_device.netif.driver_data = &e1000_device;
    mac_addr_copy(&e1000_device.netif.mac, &e1000_device.mac);
    
    // Register with network interface layer
    netif_register(&e1000_device.netif);
    
    serial_puts("[e1000] Initialization complete\n");
    return 0;
}
