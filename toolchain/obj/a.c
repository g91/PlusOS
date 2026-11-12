static pascal void pvoid(short *out, short v) { *out = (short)(v + 1); }
short f(void){ short out = 0; pvoid(&out, 41); return out; }
pascal void pv2(short *out, short v) { *out = (short)(v + 1); }
