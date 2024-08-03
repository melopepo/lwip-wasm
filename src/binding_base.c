#include <lwip/pbuf.h>
#include <stdint.h>
#include <string.h>

#include "binding.h"

static uint8_t buffer[LW_BUFFER_CAPACITY];
static int buffer_length = 0;

uint8_t *lw_buffer_ptr(void) { return buffer; }

void lw_buffer_set_length(int length) { buffer_length = length; }

int lw_buffer_get_length(void) { return buffer_length; }

struct pbuf *lw_buffer_make_pbuf(void) {
    if (buffer_length <= 0) {
        return NULL;
    }
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, buffer_length, PBUF_POOL);
    if (p == NULL) {
        return NULL;
    }
    int r = pbuf_take(p, buffer, buffer_length);
    if (r != ERR_OK) {
        pbuf_free(p);
        return NULL;
    }
    return p;
}

int lw_buffer_load_pbuf(struct pbuf *pbuf) {
    struct pbuf *current = pbuf;
    int offset = 0;
    while (current != NULL) {
        if (offset + current->len > LW_BUFFER_CAPACITY) {
            return ERR_BUF;
        }
        memcpy(buffer + offset, current->payload, current->len);
        offset += current->len;
        current = current->next;
    }
    buffer_length = offset;
    return ERR_OK;
}
