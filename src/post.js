/** Return a Error object.
 * @param err {number} the error code.
 */
function lw_jserror(err) {
  return new Error(`lwIP error ${err} (${lw_strerror(err)})`);
}

/** Check error code and throw exception if not ERR_OK.
 * @param err {number} the error code.
 */
function CHECK_ERR(err) {
  if (err != ERR_OK) {
    throw lw_jserror(err);
  }
}

// ----------------------------------------------------------------------------
// Buffering.

/** Get data from buffer.
 * @returns {Uint8Array} a copy of the data
 */
function buffer_get() {
  let ptr = lw_buffer_ptr();
  let len = lw_buffer_get_length();
  return Module.HEAPU8.subarray(ptr, ptr + len).slice();
}

/** Set buffer data.
 * @param {Uint8Array} data the data.
 */
function buffer_set(data) {
  if (!(data instanceof Uint8Array)) {
    throw new Error("buffer must be Uint8Array");
  }
  if (data.length > LW_BUFFER_CAPACITY) {
    throw new Error(`buffer larger than capacity (${LW_BUFFER_CAPACITY})`);
  }
  let ptr = lw_buffer_ptr();
  Module.HEAPU8.subarray(ptr, ptr + data.length).set(data);
  lw_buffer_set_length(data.length);
}

// ----------------------------------------------------------------------------
// Address resolution and helpers.

/** Resolve ipv4 address.
 * @param {string} address string representation of the ipv4 address
 */
function resolve_ip4(address) {
  // TODO: support hostname.
  let [a, b, c, d] = address.split(".").map((x) => +x);
  return (d << 24) | (c << 16) | (b << 8) | a;
}

/** Convert ipv4 address from uint32_t to string.
 * @param {number} address_uint uint32_t representation of the address.
 */
function ip4_ntoa(address_uint) {
  let a = address_uint & 0xff;
  let b = (address_uint >> 8) & 0xff;
  let c = (address_uint >> 16) & 0xff;
  let d = (address_uint >> 24) & 0xff;
  return `${a}.${b}.${c}.${d}`;
}

// ----------------------------------------------------------------------------
// Timer to run lwIP stack's main loop.

Module.ethernet = {
  send: (packet) => {
    if (packet.length <= LW_BUFFER_CAPACITY) {
      buffer_set(packet);
      CHECK_ERR(lw_ethernet_send());
    }
  },
  onpacket: null,
};

let timer = null;

function start(options) {
  let hw_address = options.hardwareAddress ?? "11:22:33:44:55:66";
  let ip = options.ip ?? "192.168.1.1";
  let netmask = options.netmask ?? "255.255.255.0";
  let gateway = options.gateway ?? "192.168.1.1";
  lw_init(hw_address, ip, netmask, gateway);
  if (timer != null) {
    clearInterval(timer);
  }

  lw_set_udp_recv_callback(addFunction(udp_recv_callback, "viii"));
  lw_set_tcp_event_callback(addFunction(tcp_event_callback, "iiiii"));

  timer = setInterval(() => {
    lw_loop();
    while (lw_ethernet_recv() == ERR_OK) {
      if (Module.ethernet.onpacket) {
        Module.ethernet.onpacket(buffer_get());
      }
    }
  }, options.interval ?? 10);
}

function stop() {
  if (timer != null) {
    clearInterval(timer);
    timer = null;
  }
}

Module.start = start;
Module.stop = stop;

// ----------------------------------------------------------------------------
// GC hooks.
let gc_registry = new FinalizationRegistry(([type, ptr]) => {
  switch (type) {
    case "udp":
      {
        lw_udp_remove(ptr);
        udp_socket_map.delete(ptr);
      }
      break;
    case "tcp":
      {
        console.log("closing tcp on GC", ptr);
        if (lw_tcp_close(ptr) != ERR_OK) {
          lw_tcp_abort(ptr);
        }
        tcp_socket_map.delete(ptr);
      }
      break;
  }
});

// ----------------------------------------------------------------------------
// UDP async socket-style API.

let udp_socket_map = new Map();

function udp_recv_callback(udp, address_uint, port) {
  let socket = udp_socket_map.get(udp)?.deref();
  if (socket) {
    let data = buffer_get();
    let address = ip4_ntoa(address_uint);
    return socket._on_recv(address, port, data);
  } else {
    return ERR_OK;
  }
}

class UDPSocket extends EventEmitter {
  constructor() {
    super();
    let udp = lw_udp_new();
    if (udp == 0) {
      throw new Error("unable to create udp");
    }
    this._udp = udp;
    udp_socket_map.set(udp, new WeakRef(this));
    gc_registry.register(this, ["udp", udp], this);
  }

  close() {
    if (this._udp != null) {
      lw_udp_remove(this._udp);
      udp_socket_map.delete(this._udp);
      gc_registry.unregister(this);
      this._udp = null;
    }
  }

  send(msg, address, port) {
    buffer_set(msg);
    if (address != null && port != null) {
      lw_udp_sendto(this._udp, resolve_ip4(address), port);
    } else {
      lw_udp_send(this._udp);
    }
  }

  bind(address, port) {
    lw_udp_bind(this._udp, resolve_ip4(address), port);
  }

  connect(address, port) {
    lw_udp_connect(this._udp, resolve_ip4(address), port);
  }

  disconnect() {
    lw_udp_disconnect(this._udp);
  }

  _on_recv(address, port, data) {
    this._emitNoExc("message", { data, address, port });
  }
}

Module.UDPSocket = UDPSocket;

// ----------------------------------------------------------------------------
// TCP async socket-style API.

let tcp_socket_map = new Map();

function tcp_event_callback(tcp, type, arg, new_tcp) {
  let socket = tcp_socket_map.get(tcp)?.deref();
  if (socket) {
    return socket._on_event(type, arg, new_tcp);
  } else {
    return ERR_OK;
  }
}

class TCPSocket extends EventEmitter {
  constructor(_existing_tcp) {
    super();
    let tcp = _existing_tcp ?? lw_tcp_new();
    if (tcp == 0) {
      throw new Error("unable to create tcp");
    }
    this._tcp = tcp;
    tcp_socket_map.set(tcp, new WeakRef(this));
    gc_registry.register(this, ["tcp", tcp], this);
  }

  _assert_tcp() {
    if (this._tcp == null) {
      throw new Error("socket is already closed");
    }
  }

  bind(address, port) {
    this._assert_tcp();
    CHECK_ERR(lw_tcp_bind(this._tcp, resolve_ip4(address), port));
  }

  listen() {
    this._assert_tcp();
    // tcp_listen returns a new PCB and deallocates the current one,
    // so here we need to replace the PCB pointer with the new one.
    let new_tcp = lw_tcp_listen(this._tcp);
    if (new_tcp == 0) {
      throw new Error("unable to listen");
    }
    tcp_socket_map.delete(this._tcp);
    gc_registry.unregister(this);
    gc_registry.register(this, ["tcp", new_tcp], this);
    tcp_socket_map.set(new_tcp, new WeakRef(this));
    this._tcp = new_tcp;
  }

  connect(address, port) {
    this._assert_tcp();
    return new Promise((resolve, reject) => {
      let onConnected = () => {
        this.off("_connected", onConnected);
        this.off("error", onError);
        resolve();
      };
      let onError = (error) => {
        this.off("_connected", onConnected);
        this.off("error", onError);
        reject(error);
      };
      this.on("_connected", onConnected);
      this.on("error", onError);
      CHECK_ERR(lw_tcp_connect(this._tcp, resolve_ip4(address), port));
    });
  }

  write(data) {
    return new Promise((resolve, reject) => {
      let offset = 0;
      let iteration = () => {
        if (this._tcp == null) {
          reject(lw_jserror(ERR_CLSD));
          return;
        }
        const max_write = 8192;
        let n_written = 0;
        while (1) {
          let count = data.length - offset;
          count = Math.min(
            count,
            lw_tcp_sndbuf(this._tcp),
            LW_BUFFER_CAPACITY,
            max_write - n_written
          );
          if (count == 0) {
            break;
          }
          buffer_set(data.subarray(offset, offset + count));
          let more = offset + count < data.length ? 1 : 0;
          if (lw_tcp_write(this._tcp, more) != ERR_OK) {
            break;
          }
          n_written += count;
          offset += count;
        }
        lw_tcp_output(this._tcp);

        if (offset == data.length) {
          resolve();
        } else {
          setTimeout(iteration, n_written == 0 ? 20 : 1);
        }
      };
      iteration();
    });
  }

  close() {
    if (this._tcp == null) {
      return;
    }
    if (lw_tcp_close(this._tcp) != ERR_OK) {
      lw_tcp_abort(this._tcp);
      this._did_abort = true;
    }
    tcp_socket_map.delete(this._tcp);
    gc_registry.unregister(this);
    this._tcp = null;
  }

  remoteIP() {
    this._assert_tcp();
    let ip = lw_tcp_remote_ip(this._tcp);
    return ip4_ntoa(ip);
  }

  remotePort() {
    this._assert_tcp();
    return lw_tcp_remote_port(this._tcp);
  }

  _on_event(type, arg, new_tcp) {
    this._did_abort = false;
    switch (type) {
      case 1: // EVENT_TYPE_ERR
        {
          this._emitNoExc("error", lw_jserror(arg));
        }
        break;
      case 2: // EVENT_TYPE_CONNECTED
        {
          this._emitNoExc("_connected");
        }
        break;
      case 3: // EVENT_TYPE_RECV
        {
          if (arg == ERR_CLSD) {
            this.close();
            this._emitNoExc("closed");
          } else if (arg == ERR_OK) {
            let data = buffer_get();
            if (this._tcp != null) {
              lw_tcp_recved(this._tcp, data.length);
            }
            this._emitNoExc("data", data);
          } else {
            console.error("recv with unknown err", arg);
          }
        }
        break;
      case 4: // EVENT_TYPE_SENT
        {
          // no-op
        }
        break;
      case 5: // EVENT_TYPE_ACCEPT
        {
          if (arg != ERR_OK) {
            this._emitNoExc("error", lw_jserror(arg));
          } else {
            let socket = new TCPSocket(new_tcp);
            this._emitNoExc("accept", socket);
          }
        }
        break;
    }
    if (this._did_abort) {
      return ERR_ABRT;
    }
    return ERR_OK;
  }
}

Module.TCPSocket = TCPSocket;
