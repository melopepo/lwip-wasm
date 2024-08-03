// @ts-check

import factory from "../dist/lwip-wasm.mjs";

/** @typedef {import("../api").Module} Module */

async function make_interface_pair() {
  /** @type {Module} */
  let M = await factory();
  /** @type {Module} */
  let N = await factory();

  M.start({ ip: "192.168.1.1", hardwareAddress: "11:22:33:44:55:66" });
  N.start({ ip: "192.168.1.2", hardwareAddress: "66:55:44:33:22:11" });

  M.ethernet.onpacket = (p) => {
    setTimeout(() => {
      N.ethernet.send(p);
    }, 10);
  };
  N.ethernet.onpacket = (p) => {
    setTimeout(() => {
      M.ethernet.send(p);
    }, 10);
  };
  return [M, N];
}

function repeat(callback, count, interval) {
  return new Promise((resolve, _) => {
    let n = 0;
    let timer = setInterval(() => {
      callback();
      n += 1;
      if (n >= count) {
        clearInterval(timer);
        resolve(undefined);
      }
    }, interval);
  });
}

function delay(interval) {
  return new Promise((resolve, _) => {
    setTimeout(resolve, interval);
  });
}

async function test_udp() {
  console.log("[test_udp] start");
  let [M, N] = await make_interface_pair();

  let total_received = 0;
  let total_sent = 0;

  let udp1 = new M.UDPSocket();
  udp1.bind("0.0.0.0", 100);
  udp1.on("message", (e) => {
    total_received += e.data.length;
  });

  let udp2 = new N.UDPSocket();
  udp2.connect("192.168.1.1", 100);
  await repeat(
    () => {
      let data = new Uint8Array([1, 2, 3, 4, 5, 6, 7, 8, 9, 10]);
      udp2.send(data);
      total_sent += data.length;
    },
    10,
    10
  );
  let udp3 = new N.UDPSocket();
  await repeat(
    () => {
      let data = new Uint8Array([1, 2, 3, 4, 5, 6, 7, 8, 9, 10]);
      udp3.send(data, "192.168.1.1", 100);
      total_sent += data.length;
    },
    10,
    10
  );

  await delay(200);

  console.log(`sent: ${total_sent}, received: ${total_received}`);

  M.stop();
  N.stop();

  console.log("[test_udp] done");
}

async function test_tcp() {
  console.log("[test_tcp] start");
  let [M, N] = await make_interface_pair();

  const total_to_send = 100000;
  let total_data_errors = 0;

  let server = new M.TCPSocket();
  let sockets = new Set();
  server.bind("0.0.0.0", 100);
  server.on("accept", (socket) => {
    let total_received = 0;
    let t0 = new Date().getTime();
    let ii = 0;
    let num = 0;
    console.log(`client connected ${socket.remoteIP()}:${socket.remotePort()}`);
    socket.on("data", (data) => {
      if (total_received == 0) {
        num = data[0];
      }
      total_received += data.length;
      for (let i = 0; i < data.length; i++) {
        if (data[i] != (num + ii * 7) % 255) {
          total_data_errors += 1;
        }
        ii++;
      }
      if (total_received == total_to_send) {
        let total_time = (new Date().getTime() - t0) / 1000;
        let speed = total_received / total_time;
        console.log(`client ${num} completed transfer, speed: ${(speed / 1024).toFixed(2)}KB/s`);
        socket.close();
        sockets.delete(socket);
      }
    });
    sockets.add(socket);
  });
  server.listen();

  let addClient = async (num) => {
    let client = new N.TCPSocket();
    await client.connect("192.168.1.1", 100);
    let data = new Uint8Array(total_to_send);
    for (let i = 0; i < data.length; i++) {
      data[i] = (num + i * 7) % 255;
    }
    let t0 = new Date().getTime();
    await client.write(data);
    let t1 = new Date().getTime();
    console.log(`client ${num}: sent in ${((t1 - t0) / 1000).toFixed(3)}s`);
  };

  await Promise.all([addClient(1), addClient(2), addClient(3)]);
  await Promise.all([addClient(4), addClient(5), addClient(6)]);
  await delay(1000);

  M.stop();
  N.stop();
  console.log("[test_tcp] done");
}

async function test() {
  await test_udp();
  await delay(1000);
  await test_tcp();
}

test();
