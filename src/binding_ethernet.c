#include <emscripten/emscripten.h>
#include <lwip/inet.h>
#include <lwip/init.h>
#include <lwip/ip4.h>
#include <lwip/ip4_addr.h>
#include <lwip/ip4_frag.h>
#include <lwip/netif.h>
#include <lwip/tcp.h>
#include <lwip/timeouts.h>
#include <lwip/udp.h>
#include <netif/etharp.h>
#include <string.h>

#include "binding.h"

#define ETHERNET_MTU 1500

#define QUEUE_SIZE 256
static struct pbuf *output_queue[QUEUE_SIZE];
int output_queue_count = 0;

static struct netif netif;
uint8_t mac_address[6];

static err_t netif_output(struct netif *netif, struct pbuf *p) {
    LWIP_UNUSED_ARG(netif);
    if (output_queue_count < QUEUE_SIZE) {
        pbuf_ref(p);
        output_queue[output_queue_count++] = p;
        return ERR_OK;
    } else {
        return ERR_BUF;
    }
}

static err_t port_netif_init(struct netif *netif) {
    netif->linkoutput = netif_output;
    netif->output = etharp_output;
    netif->mtu = ETHERNET_MTU;
    netif->flags = NETIF_FLAG_UP | NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET | NETIF_FLAG_IGMP;

    memcpy(netif->hwaddr, mac_address, sizeof(netif->hwaddr));
    netif->hwaddr_len = sizeof(netif->hwaddr);
    return ERR_OK;
}

void lw_init(const char *mac, const char *ip, const char *netmask, const char *gateway) {
    lwip_init();

    ip4_addr_t ip_addr, gw_addr, mask_addr;
    ip4addr_aton(ip, &ip_addr);
    ip4addr_aton(gateway, &gw_addr);
    ip4addr_aton(netmask, &mask_addr);
    int m[6];
    sscanf(mac, "%x:%x:%x:%x:%x:%x", &m[0], &m[1], &m[2], &m[3], &m[4], &m[5]);
    for (int i = 0; i < 6; i++) {
        mac_address[i] = m[i];
    }

    netif_add(&netif, &ip_addr, &mask_addr, &gw_addr, NULL, port_netif_init, netif_input);
    netif_set_default(&netif);
    netif_set_up(&netif);
    netif_set_link_up(&netif);
}

void lw_loop(void) { sys_check_timeouts(); }

int lw_ethernet_recv(void) {
    uint8_t *buffer = lw_buffer_ptr();
    if (output_queue_count == 0) {
        return ERR_WOULDBLOCK;
    }

    int r = ERR_WOULDBLOCK;

    // Dequeue the first packet.
    struct pbuf *p = output_queue[0];
    for (int i = 0; i < output_queue_count - 1; i++) {
        output_queue[i] = output_queue[i + 1];
    }
    output_queue_count -= 1;

    int len = p->len;
    if (len <= LW_BUFFER_CAPACITY) {
        memcpy(buffer, p->payload, len);
        lw_buffer_set_length(len);
        r = ERR_OK;
    } else {
        r = ERR_BUF;
    }
    pbuf_free(p);

    return r;
}

int lw_ethernet_send(void) {
    struct pbuf *p = lw_buffer_make_pbuf();
    if (p == NULL) {
        return ERR_MEM;
    }
    int r = netif.input(p, &netif);
    if (r != ERR_OK) {
        pbuf_free(p);
    }
    return r;
}
