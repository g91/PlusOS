// worker.js - Web Worker entry: the emulator runs here so the page stays responsive.
import { Engine } from './engine.js';

const engine = new Engine((msg, transfer) => self.postMessage(msg, transfer || []));
self.onmessage = (e) => engine.handle(e.data);
self.postMessage({ type: 'ready' });
