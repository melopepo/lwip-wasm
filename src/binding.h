#ifndef BINDING_H
#define BINDING_H

#include <emscripten/emscripten.h>
#include <stdint.h>

#define LW_BUFFER_CAPACITY 32768

// Binding functions exposed to JavaScript.
// Mainly mirror lwIP "raw" API, with:
// - Buffer access now go though lw_buffer_*
// - Integers are always int.
// - Pointers are always void*.

EMSCRIPTEN_KEEPALIVE uint8_t *lw_buffer_ptr(void);
EMSCRIPTEN_KEEPALIVE void lw_buffer_set_length(int length);
EMSCRIPTEN_KEEPALIVE int lw_buffer_get_length(void);

struct pbuf *lw_buffer_make_pbuf(void);
int lw_buffer_load_pbuf(struct pbuf *pbuf);

EMSCRIPTEN_KEEPALIVE void lw_init(const char *mac_address, const char *ip, const char *netmask, const char *gateway);
EMSCRIPTEN_KEEPALIVE void lw_loop(void);

EMSCRIPTEN_KEEPALIVE int lw_ethernet_send(void);
EMSCRIPTEN_KEEPALIVE int lw_ethernet_recv(void);

typedef uint32_t ip_address_t;

typedef void *lw_udp_t;
EMSCRIPTEN_KEEPALIVE lw_udp_t lw_udp_new(void);
EMSCRIPTEN_KEEPALIVE void lw_udp_remove(lw_udp_t udp);
EMSCRIPTEN_KEEPALIVE int lw_udp_sendto(lw_udp_t udp, ip_address_t dst_ip, int dst_port);
EMSCRIPTEN_KEEPALIVE int lw_udp_send(lw_udp_t udp);
EMSCRIPTEN_KEEPALIVE int lw_udp_bind(lw_udp_t udp, ip_address_t ip, int port);
EMSCRIPTEN_KEEPALIVE int lw_udp_connect(lw_udp_t udp, ip_address_t ip, int port);
EMSCRIPTEN_KEEPALIVE void lw_udp_disconnect(lw_udp_t udp);
EMSCRIPTEN_KEEPALIVE void lw_udp_recv(lw_udp_t udp, void *arg);

typedef void (*lw_udp_recv_callback_fn)(void *, ip_address_t, int);
EMSCRIPTEN_KEEPALIVE void lw_set_udp_recv_callback(lw_udp_recv_callback_fn fn);

typedef void *lw_tcp_t;
EMSCRIPTEN_KEEPALIVE lw_tcp_t lw_tcp_new(void);
EMSCRIPTEN_KEEPALIVE int lw_tcp_close(lw_tcp_t tcp);
EMSCRIPTEN_KEEPALIVE void lw_tcp_abort(lw_tcp_t tcp);
EMSCRIPTEN_KEEPALIVE int lw_tcp_bind(lw_tcp_t tcp, ip_address_t address, int port);
EMSCRIPTEN_KEEPALIVE lw_tcp_t lw_tcp_listen(lw_tcp_t tcp);
EMSCRIPTEN_KEEPALIVE void lw_tcp_recved(lw_tcp_t tcp, u16_t len);
EMSCRIPTEN_KEEPALIVE int lw_tcp_connect(lw_tcp_t tcp, ip_address_t address, int port);
EMSCRIPTEN_KEEPALIVE int lw_tcp_write(lw_tcp_t tcp, int more);
EMSCRIPTEN_KEEPALIVE int lw_tcp_sndbuf(lw_tcp_t tcp);
EMSCRIPTEN_KEEPALIVE int lw_tcp_output(lw_tcp_t tcp);
EMSCRIPTEN_KEEPALIVE uint32_t lw_tcp_remote_ip(lw_tcp_t tcp);
EMSCRIPTEN_KEEPALIVE int lw_tcp_remote_port(lw_tcp_t tcp);

typedef int (*lw_tcp_callback_fn)(void *, int, int, lw_tcp_t);
EMSCRIPTEN_KEEPALIVE void lw_set_tcp_event_callback(lw_tcp_callback_fn fn);

#endif
