#include <emscripten/emscripten.h>
#include <lwip/tcp.h>

#include "binding.h"

// Mirror functions in https://www.nongnu.org/lwip/2_1_x/group__tcp__raw.html

#define EVENT_TYPE_ERR 1
#define EVENT_TYPE_CONNECTED 2
#define EVENT_TYPE_RECV 3
#define EVENT_TYPE_SENT 4
#define EVENT_TYPE_ACCEPT 5

static lw_tcp_callback_fn _tcp_callback_fn = NULL;

static err_t lw_tcp_connected_fn(void *arg, struct tcp_pcb *tpcb, err_t err) {
    LWIP_UNUSED_ARG(tpcb);
    if (_tcp_callback_fn == NULL) {
        return ERR_OK;
    }
    return _tcp_callback_fn(arg, EVENT_TYPE_CONNECTED, err, NULL);
}

static void lw_tcp_err_fn(void *arg, err_t err) {
    if (_tcp_callback_fn == NULL) {
        return;
    }
    _tcp_callback_fn(arg, EVENT_TYPE_ERR, err, NULL);
}

static err_t lw_tcp_recv_fn(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    LWIP_UNUSED_ARG(tpcb);
    if (p != NULL) {
        if (_tcp_callback_fn != NULL) {
            lw_buffer_load_pbuf(p);
        }
        pbuf_free(p);
    } else {
        // Connection has been closed.
        err = ERR_CLSD;
    }
    if (_tcp_callback_fn != NULL) {
        return _tcp_callback_fn(arg, EVENT_TYPE_RECV, err, NULL);
    }
    return ERR_OK;
}

static err_t lw_tcp_sent_fn(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    LWIP_UNUSED_ARG(tpcb);
    if (_tcp_callback_fn == NULL) {
        return ERR_OK;
    }
    return _tcp_callback_fn(arg, EVENT_TYPE_SENT, len, NULL);
}

static err_t lw_tcp_accept_fn(void *arg, struct tcp_pcb *newpcb, err_t err) {
    if (_tcp_callback_fn == NULL) {
        return ERR_OK;
    }
    tcp_arg(newpcb, newpcb);
    tcp_err(newpcb, lw_tcp_err_fn);
    tcp_recv(newpcb, lw_tcp_recv_fn);
    tcp_sent(newpcb, lw_tcp_sent_fn);
    if (_tcp_callback_fn != NULL) {
        return _tcp_callback_fn(arg, EVENT_TYPE_ACCEPT, err, newpcb);
    } else {
        // Nothing to handle the accepted connection, close or abort it.
        if (tcp_close(newpcb) != ERR_OK) {
            tcp_abort(newpcb);
            return ERR_ABRT;
        }
        return ERR_OK;
    }
}

void lw_set_tcp_event_callback(lw_tcp_callback_fn fn) { _tcp_callback_fn = fn; }

lw_tcp_t lw_tcp_new(void) {
    struct tcp_pcb *pcb = tcp_new();
    if (pcb != NULL) {
        tcp_arg(pcb, pcb);
        tcp_err(pcb, lw_tcp_err_fn);
        tcp_recv(pcb, lw_tcp_recv_fn);
        tcp_sent(pcb, lw_tcp_sent_fn);
    }
    return pcb;
}

int lw_tcp_close(lw_tcp_t tcp) {
    struct tcp_pcb *pcb = (struct tcp_pcb *)tcp;
    return tcp_close(pcb);
}

void lw_tcp_abort(lw_tcp_t tcp) {
    struct tcp_pcb *pcb = (struct tcp_pcb *)tcp;
    tcp_abort(pcb);
}

int lw_tcp_bind(lw_tcp_t tcp, ip_address_t address, int port) {
    struct tcp_pcb *pcb = (struct tcp_pcb *)tcp;
    ip_addr_t addr = IPADDR4_INIT(address);
    return tcp_bind(pcb, &addr, port);
}

lw_tcp_t lw_tcp_listen(lw_tcp_t tcp) {
    struct tcp_pcb *pcb = (struct tcp_pcb *)tcp;
    pcb = tcp_listen(pcb);
    if (pcb != NULL) {
        tcp_arg(pcb, pcb);
        tcp_accept(pcb, lw_tcp_accept_fn);
    }
    return pcb;
}

void lw_tcp_recved(lw_tcp_t tcp, u16_t len) {
    struct tcp_pcb *pcb = (struct tcp_pcb *)tcp;
    tcp_recved(pcb, len);
}

int lw_tcp_connect(lw_tcp_t tcp, ip_address_t address, int port) {
    struct tcp_pcb *pcb = (struct tcp_pcb *)tcp;
    ip_addr_t addr = IPADDR4_INIT(address);
    return tcp_connect(pcb, &addr, port, lw_tcp_connected_fn);
}

int lw_tcp_write(lw_tcp_t tcp, int more) {
    struct tcp_pcb *pcb = (struct tcp_pcb *)tcp;
    uint8_t flags = TCP_WRITE_FLAG_COPY;
    if (more) {
        flags |= TCP_WRITE_FLAG_MORE;
    }
    return tcp_write(pcb, lw_buffer_ptr(), lw_buffer_get_length(), flags);
}

int lw_tcp_sndbuf(lw_tcp_t tcp) {
    struct tcp_pcb *pcb = (struct tcp_pcb *)tcp;
    return tcp_sndbuf(pcb);
}

int lw_tcp_output(lw_tcp_t tcp) {
    struct tcp_pcb *pcb = (struct tcp_pcb *)tcp;
    return tcp_output(pcb);
}

uint32_t lw_tcp_remote_ip(lw_tcp_t tcp) {
    struct tcp_pcb *pcb = (struct tcp_pcb *)tcp;
    return pcb->remote_ip.addr;
}

int lw_tcp_remote_port(lw_tcp_t tcp) {
    struct tcp_pcb *pcb = (struct tcp_pcb *)tcp;
    return pcb->remote_port;
}
