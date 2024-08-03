[![Node.js CI](https://github.com/melopepo/lwip-wasm/actions/workflows/test.yml/badge.svg)](https://github.com/melopepo/lwip-wasm/actions/workflows/test.yml)

# lwIP networking stack in WebAssembly

This is a port of the [lwIP](https://savannah.nongnu.org/projects/lwip/) networking stack to WebAssembly, for use inside browsers.

**Why port a TCP/IP stack into a browser???** I was trying to use [v86](https://github.com/copy/v86), a browser-based x86 machine emulator, to revive my old Window projects. I wanted to be able to share files between the guest virtual machine and the hosting web page. However, existing solutions either require me to put those files in a disk image and then mount the image as a CD-ROM or Floppy disk, or require uploading those files to the server and have a WebSocket-based network proxy that tunnels Ethernet packets. The former is pretty tedious and only supports one direction of file transfer; the latter sounds like a huge security risk, plus a special script to run on the server. I was wondering, can we just run a HTTP server inside the browser and let the VM access it?

It turned out to be pretty hard, because the emulator only exposes raw Ethernet frames from/to the emulated Ethernet controller. When the VM wants to make a HTTP request, we get Ethernet frames from the JavaScript code. To serve files in HTTP, we'd need to parse those Ethernet frames into IP packets, then TCP stream, and finally HTTP requests. It's a huge amount of work. TCP along is really hard to implement correctly. This is where lwIP comes into play. It implements these complex protocols for us. On one side it talks in Ethernet frames, and on the other side it exposes a nice API for TCP/UDP. With this we can then build a simple HTTP server that serves our files.

## API

The API is documented in [api.d.ts](api.d.ts).

See the `test` folder for usage examples.

## Build

Make sure to clone with Git submodules.

Install [Emscripten](https://emscripten.org/) and have `emcc` is in your `PATH`.

Run `make` or `npm run build` to build the project.

## Limitations

- Only support one and only one Ethernet interface.
- Only expose API for TCP and UDP.
- Only support IPv4. No IPv6 support.
- `TCPSocket` and `UDPSocket` API needs improvement. I'm hoping to make them more like Node.js's socket API.
- We are using lwIP's `NO_SYS` mode, and the network stack runs in the main thread. If we use Emscripten's multi-thread support, we might be able to switch to the [OS mode](https://www.nongnu.org/lwip/2_0_x/group__lwip__os.html) and maybe get better performance.
- Low throughput (about 200KB/s file download speed in the VM), need performance tuning.
