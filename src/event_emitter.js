class EventEmitter {
  constructor() {
    this._listeners = new Map();
  }

  on(type, listener) {
    let s = this._listeners.get(type);
    if (s == null) {
      s = new Set();
      this._listeners.set(type, s);
    }
    s.add(listener);
  }

  off(type, listener) {
    if (this._listeners.has(type)) {
      this._listeners.get(type).delete(listener);
    }
  }

  _emit(type, event) {
    let listeners = this._listeners.get(type);
    if (listeners) {
      for (let listener of listeners) {
        listener(event);
      }
    }
  }

  _emitNoExc(type, event) {
    try {
      this._emit(type, event);
    } catch (e) {
      console.error(e);
    }
  }
}
