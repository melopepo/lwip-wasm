#ifndef LWIPOPTS_H
#define LWIPOPTS_H

#define NO_SYS 1

#define LWIP_IPV4 1
#define LWIP_IPV6 0
#define LWIP_ICMP 1

// Disable forward.
#define IP_FORWARD 0

// Disable APIs that are not compatible with NO_SYS.
#define LWIP_SOCKET 0
#define LWIP_NETCONN 0

// MIB2 stats are not currently used.
#define MIB2_STATS 0

// Larger memory size than an embedded device,
// because we target modern browsers.
#define MEM_SIZE 524288

// Larger limits on buffers and PCBs.
#define MEMP_NUM_PBUF 128
#define PBUF_POOL_SIZE 128
#define MEMP_NUM_RAW_PCB 16

// UDP parameters.
#define MEMP_NUM_UDP_PCB 16

// TCP parameters.
#define MEMP_NUM_TCP_PCB 16
#define MEMP_NUM_TCP_PCB_LISTEN 16
#define MEMP_NUM_TCP_SEG 1024
#define TCP_MSS 1460
#define TCP_WND 0xffff
#define TCP_SND_BUF (TCP_MSS * 16)
#define TCP_SND_QUEUELEN 64
#define TCP_LISTEN_BACKLOG 1
#define TCP_DEFAULT_LISTEN_BACKLOG 0xff

#include "arch/sys_arch.h"

#endif
