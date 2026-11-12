// keycodes.js - Macintosh virtual key codes (as sent by ADB keyboards; the Plus
// keyboard module converts them to its own wire format).

// KeyboardEvent.code -> Mac key code
export const KEY_CODES = {
  KeyA: 0x00, KeyS: 0x01, KeyD: 0x02, KeyF: 0x03, KeyH: 0x04, KeyG: 0x05, KeyZ: 0x06, KeyX: 0x07, KeyC: 0x08, KeyV: 0x09,
  KeyB: 0x0B, KeyQ: 0x0C, KeyW: 0x0D, KeyE: 0x0E, KeyR: 0x0F, KeyY: 0x10, KeyT: 0x11,
  Digit1: 0x12, Digit2: 0x13, Digit3: 0x14, Digit4: 0x15, Digit6: 0x16, Digit5: 0x17, Equal: 0x18, Digit9: 0x19, Digit7: 0x1A,
  Minus: 0x1B, Digit8: 0x1C, Digit0: 0x1D, BracketRight: 0x1E, KeyO: 0x1F, KeyU: 0x20, BracketLeft: 0x21, KeyI: 0x22, KeyP: 0x23,
  Enter: 0x24, KeyL: 0x25, KeyJ: 0x26, Quote: 0x27, KeyK: 0x28, Semicolon: 0x29, Backslash: 0x2A, Comma: 0x2B, Slash: 0x2C,
  KeyN: 0x2D, KeyM: 0x2E, Period: 0x2F, Tab: 0x30, Space: 0x31, Backquote: 0x32, Backspace: 0x33, Escape: 0x35,
  ControlLeft: 0x36, ControlRight: 0x36, MetaLeft: 0x37, MetaRight: 0x37, OSLeft: 0x37, OSRight: 0x37,
  ShiftLeft: 0x38, ShiftRight: 0x38, CapsLock: 0x39, AltLeft: 0x3A, AltRight: 0x3A,
  ArrowLeft: 0x3B, ArrowRight: 0x3C, ArrowDown: 0x3D, ArrowUp: 0x3E,
  NumpadDecimal: 0x41, NumpadMultiply: 0x43, NumpadAdd: 0x45, NumLock: 0x47, NumpadDivide: 0x4B, NumpadEnter: 0x4C,
  NumpadSubtract: 0x4E, NumpadEqual: 0x51, Numpad0: 0x52, Numpad1: 0x53, Numpad2: 0x54, Numpad3: 0x55, Numpad4: 0x56,
  Numpad5: 0x57, Numpad6: 0x58, Numpad7: 0x59, Numpad8: 0x5B, Numpad9: 0x5C,
  F1: 0x7A, F2: 0x78, F3: 0x63, F4: 0x76, F5: 0x60, F6: 0x61, F7: 0x62, F8: 0x64, F9: 0x65, F10: 0x6D, F11: 0x67, F12: 0x6F,
  Insert: 0x72, Home: 0x73, PageUp: 0x74, Delete: 0x75, End: 0x77, PageDown: 0x79,
};

export const KEY_SHIFT = 0x38, KEY_COMMAND = 0x37, KEY_OPTION = 0x3A, KEY_CONTROL = 0x36, KEY_RETURN = 0x24;

// US layout: character -> [key code, needs shift]
const PLAIN = "asdfhgzxcv\u0000bqweryt123465=97-80]ou[ip\rlj'k;\\,/nm.\t `\b";
const SHIFTED = 'ASDFHGZXCV\u0000BQWERYT!@#$^%+(&_*)}OU{IP\rLJ"K:|<?NM>\t ~\b';
const CHAR_MAP = new Map();
for (let i = 0; i < PLAIN.length; i++) {
  if (i === 0x0A) continue;
  if (!CHAR_MAP.has(PLAIN[i])) CHAR_MAP.set(PLAIN[i], [i, false]);
  if (SHIFTED[i] !== PLAIN[i] && !CHAR_MAP.has(SHIFTED[i])) CHAR_MAP.set(SHIFTED[i], [i, true]);
}
CHAR_MAP.set('\n', [0x24, false]);
CHAR_MAP.set('\u001b', [0x35, false]);

export function charToKey(ch) { return CHAR_MAP.get(ch) || null; }

// Queue of timed key events for typing a string: returns [{ code, down }] in order.
export function typeString(text) {
  const out = [];
  for (const ch of text) {
    const k = charToKey(ch);
    if (!k) continue;
    if (k[1]) out.push({ code: KEY_SHIFT, down: true });
    out.push({ code: k[0], down: true }, { code: k[0], down: false });
    if (k[1]) out.push({ code: KEY_SHIFT, down: false });
  }
  return out;
}
