export declare interface EthernetInterface {
  /** Send raw packet to the ethernet interface. */
  send(data: Uint8Array): void;
  /** Callback to receive raw packets from the interface. */
  onpacket?: (data: Uint8Array) => void;
}

export declare interface StartOptions {
  /** Hardware (MAC) address of the ethernet interface. */
  hardwareAddress?: string;

  /** IP address, default to 192.168.1.1 */
  ip?: string;

  /** Netmask, default to 255.255.255.0 */
  netmask?: string;

  /** Gateway address, default to 192.168.1.1 */
  gateway?: string;
}

export declare class EventEmitter {}

export declare class TCPSocket extends EventEmitter {
  constructor();

  bind(address: string, port: number): void;
  listen(): void;

  connect(address: string, port: number): Promise<void>;

  write(data: Uint8Array): Promise<void>;

  close(): void;

  remoteIP(): string;
  remotePort(): number;

  on(type: "accept", callback: (socket: TCPSocket) => void): void;
  on(type: "error", callback: (error: Error) => void): void;
  on(type: "data", callback: (Uint8Array) => void): void;
  on(type: "closed", callback: () => void): void;

  off(type: "accept", callback: (socket: TCPSocket) => void): void;
  off(type: "error", callback: (error: Error) => void): void;
  off(type: "data", callback: (Uint8Array) => void): void;
  off(type: "closed", callback: () => void): void;
}

export declare class UDPSocket extends EventEmitter {
  constructor();

  bind(address: string, port: number): void;
  connect(address: string, port: number): void;
  disconnect(): void;
  send(msg: Uint8Array): void;
  send(msg: Uint8Array, address: string, port: number): void;
  close(): void;

  on(
    type: "message",
    callback: (message: { data: Uint8Array; address: string; port: number }) => void
  ): void;
  off(
    type: "message",
    callback: (message: { data: Uint8Array; address: string; port: number }) => void
  ): void;
}

export declare interface Module {
  ethernet: EthernetInterface;

  start(options: StartOptions): void;
  stop(): void;

  TCPSocket: typeof TCPSocket;
  UDPSocket: typeof UDPSocket;
}

export default function (): Promise<Module>;
