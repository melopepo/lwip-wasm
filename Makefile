SRCS = src/arch.c src/binding_base.c src/binding_ethernet.c src/binding_udp.c src/binding_tcp.c

LWIPDIR = lwip/src
LWIPINCLUDEDIR = $(LWIPDIR)/include
include $(LWIPDIR)/Filelists.mk

# CFLAGS = -pedantic -Wall -Wextra -g -gsource-map
CFLAGS = -pedantic -Wall -Wextra -O2 -flto

EMCCFLAGS = -sEXPORTED_RUNTIME_METHODS=cwrap,addFunction \
	-sALLOW_TABLE_GROWTH \
	-sALLOW_MEMORY_GROWTH \
	-sFILESYSTEM=0 \
	--post-js src/binding.js --post-js src/event_emitter.js --post-js src/post.js

.PHONY: all
all:
	node scripts/generate_binding.js
	mkdir -p dist
	emcc $(COREFILES) $(CORE4FILES) $(NETIFFILES) $(SRCS) \
		-I $(LWIPINCLUDEDIR) -I src/include \
		$(CFLAGS) $(EMCCFLAGS) \
		-o dist/lwip-wasm.mjs

.PHONY: test
test: all
	node test/test.mjs

.PHONY: clean
clean:
	rm -rf dist
