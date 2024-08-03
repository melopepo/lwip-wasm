#include <emscripten/emscripten.h>
#include <lwip/udp.h>

#include "binding.h"

// Mirror functions in https://www.nongnu.org/lwip/2_1_x/group__udp__raw.html

static lw_udp_recv_callback_fn _udp_recv_fn = NULL;

static void lw_udp_recv_fn(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port) {
    LWIP_UNUSED_ARG(arg);
    if (_udp_recv_fn == NULL) {
        return;
    }
    if (lw_buffer_load_pbuf(p) != ERR_OK) {
        return;
    }
    pbuf_free(p);
    _udp_recv_fn(pcb, addr->addr, port);
}

void lw_set_udp_recv_callback(lw_udp_recv_callback_fn fn) { _udp_recv_fn = fn; }

lw_udp_t lw_udp_new(void) {
    struct udp_pcb *pcb = udp_new();
    if (pcb != NULL) {
        udp_recv(pcb, lw_udp_recv_fn, NULL);
    }
    return pcb;
}

void lw_udp_remove(lw_udp_t udp) {
    struct udp_pcb *pcb = (struct udp_pcb *)udp;
    udp_remove(pcb);
}

int lw_udp_sendto(lw_udp_t udp, ip_address_t dst_ip, int dst_port) {
    int r;
    struct udp_pcb *pcb = (struct udp_pcb *)udp;
    struct pbuf *p = lw_buffer_make_pbuf();
    if (p == NULL) {
        return ERR_MEM;
    }
    ip_addr_t dst_addr = IPADDR4_INIT(dst_ip);
    r = udp_sendto(pcb, p, &dst_addr, dst_port);
    pbuf_free(p);
    return r;
}

int lw_udp_send(lw_udp_t udp) {
    int r;
    struct udp_pcb *pcb = (struct udp_pcb *)udp;
    struct pbuf *p = lw_buffer_make_pbuf();
    if (p == NULL) {
        return ERR_MEM;
    }
    r = udp_send(pcb, p);
    pbuf_free(p);
    return r;
}

int lw_udp_bind(lw_udp_t udp, ip_address_t ip, int port) {
    struct udp_pcb *pcb = (struct udp_pcb *)udp;
    ip_addr_t addr = IPADDR4_INIT(ip);
    return udp_bind(pcb, &addr, port);
}

int lw_udp_connect(lw_udp_t udp, ip_address_t ip, int port) {
    struct udp_pcb *pcb = (struct udp_pcb *)udp;
    ip_addr_t addr = IPADDR4_INIT(ip);
    return udp_connect(pcb, &addr, port);
}

void lw_udp_disconnect(lw_udp_t udp) {
    struct udp_pcb *pcb = (struct udp_pcb *)udp;
    udp_disconnect(pcb);
}
