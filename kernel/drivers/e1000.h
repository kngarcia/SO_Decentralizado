#ifndef E1000_H
#define E1000_H

#include <stdint.h>
#include "../net/netif.h"

// E1000 PCI Vendor/Device IDs
#define E1000_VENDOR_ID    0x8086
#define E1000_DEVICE_ID    0x100E  // 82540EM

// E1000 Register offsets
#define E1000_REG_CTRL     0x00000
#define E1000_REG_STATUS   0x00008
#define E1000_REG_EECD     0x00010
#define E1000_REG_EERD     0x00014
#define E1000_REG_ICR      0x000C0
#define E1000_REG_IMS      0x000D0
#define E1000_REG_RCTL     0x00100
#define E1000_REG_TCTL     0x00400
#define E1000_REG_RDBAL    0x02800
#define E1000_REG_RDBAH    0x02804
#define E1000_REG_RDLEN    0x02808
#define E1000_REG_RDH      0x02810
#define E1000_REG_RDT      0x02818
#define E1000_REG_TDBAL    0x03800
#define E1000_REG_TDBAH    0x03804
#define E1000_REG_TDLEN    0x03808
#define E1000_REG_TDH      0x03810
#define E1000_REG_TDT      0x03818
#define E1000_REG_RAL      0x05400
#define E1000_REG_RAH      0x05404

// Control Register bits
#define E1000_CTRL_RST     (1 << 26)
#define E1000_CTRL_SLU     (1 << 6)
#define E1000_CTRL_ASDE    (1 << 5)

// Receive Control bits
#define E1000_RCTL_EN      (1 << 1)
#define E1000_RCTL_UPE     (1 << 3)
#define E1000_RCTL_MPE     (1 << 4)
#define E1000_RCTL_BAM     (1 << 15)
#define E1000_RCTL_BSIZE_2048  (0 << 16)
#define E1000_RCTL_SECRC   (1 << 26)

// Transmit Control bits
#define E1000_TCTL_EN      (1 << 1)
#define E1000_TCTL_PSP     (1 << 3)

// Descriptor counts
#define E1000_NUM_RX_DESC  32
#define E1000_NUM_TX_DESC  32

// Receive Descriptor
typedef struct {
    volatile uint64_t addr;
    volatile uint16_t length;
    volatile uint16_t checksum;
    volatile uint8_t status;
    volatile uint8_t errors;
    volatile uint16_t special;
} __attribute__((packed)) e1000_rx_desc_t;

// Transmit Descriptor
typedef struct {
    volatile uint64_t addr;
    volatile uint16_t length;
    volatile uint8_t cso;
    volatile uint8_t cmd;
    volatile uint8_t status;
    volatile uint8_t css;
    volatile uint16_t special;
} __attribute__((packed)) e1000_tx_desc_t;

// E1000 device structure
typedef struct {
    uint32_t mem_base;
    mac_addr_t mac;
    
    e1000_rx_desc_t *rx_descs;
    uint8_t *rx_buffers[E1000_NUM_RX_DESC];
    uint16_t rx_cur;
    
    e1000_tx_desc_t *tx_descs;
    uint8_t *tx_buffers[E1000_NUM_TX_DESC];
    uint16_t tx_cur;
    
    netif_t netif;
} e1000_dev_t;

// Driver functions
int e1000_init(void);
int e1000_send_packet(netif_t *netif, const void *data, uint32_t len);
int e1000_recv_packet(netif_t *netif, void *buf, uint32_t max_len);

#endif // E1000_H
