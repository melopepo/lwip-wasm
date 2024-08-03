declare function cwrap(name: string, ret?: string, args?: string[]): any;
declare function addFunction(fn: any, args: string): number;
declare namespace Module {
  export let HEAPU8: Uint8Array;
}
