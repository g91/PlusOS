/* config.c - table of built-in (statically linked) modules for PlusOS. Generated shape of
 * Modules/config.c.in; edit PlusOS/modules.txt and this list together. */
#include "Python.h"
extern PyObject* PyMarshal_Init(void);
extern PyObject* PyInit__imp(void);
extern PyObject* PyInit_gc(void);
extern PyObject* PyInit__ast(void);
extern PyObject* PyInit__tokenize(void);
extern PyObject* PyInit__contextvars(void);
extern PyObject* _PyWarnings_Init(void);
extern PyObject* PyInit__string(void);
extern PyObject* PyInit_atexit(void);
extern PyObject* PyInit_posix(void);
extern PyObject* PyInit__signal(void);
extern PyObject* PyInit__suggestions(void);
extern PyObject* PyInit__datetime(void);
extern PyObject* PyInit__codecs(void);
extern PyObject* PyInit__collections(void);
extern PyObject* PyInit_errno(void);
extern PyObject* PyInit_itertools(void);
extern PyObject* PyInit__sre(void);
extern PyObject* PyInit__sysconfig(void);
extern PyObject* PyInit__thread(void);
extern PyObject* PyInit_time(void);
extern PyObject* PyInit__types(void);
extern PyObject* PyInit__typing(void);
extern PyObject* PyInit__weakref(void);
extern PyObject* PyInit__abc(void);
extern PyObject* PyInit__functools(void);
extern PyObject* PyInit__locale(void);
extern PyObject* PyInit__opcode(void);
extern PyObject* PyInit__operator(void);
extern PyObject* PyInit__stat(void);
extern PyObject* PyInit__symtable(void);
extern PyObject* PyInit_math(void);
extern PyObject* PyInit__math_integer(void);
extern PyObject* PyInit_cmath(void);
extern PyObject* PyInit__struct(void);
extern PyObject* PyInit__random(void);
extern PyObject* PyInit__bisect(void);
extern PyObject* PyInit__heapq(void);
extern PyObject* PyInit_array(void);
extern PyObject* PyInit_binascii(void);
extern PyObject* PyInit__json(void);
extern PyObject* PyInit__pickle(void);
extern PyObject* PyInit_unicodedata(void);
extern PyObject* PyInit__statistics(void);
extern PyObject* PyInit__zoneinfo(void);
extern PyObject* PyInit__csv(void);
extern PyObject* PyInit__io(void);

struct _inittab _PyImport_Inittab[] = {
    {"atexit", PyInit_atexit},
    {"posix", PyInit_posix},
    {"_signal", PyInit__signal},
    {"_suggestions", PyInit__suggestions},
    {"_datetime", PyInit__datetime},
    {"_codecs", PyInit__codecs},
    {"_collections", PyInit__collections},
    {"errno", PyInit_errno},
    {"itertools", PyInit_itertools},
    {"_sre", PyInit__sre},
    {"_sysconfig", PyInit__sysconfig},
    {"_thread", PyInit__thread},
    {"time", PyInit_time},
    {"_types", PyInit__types},
    {"_typing", PyInit__typing},
    {"_weakref", PyInit__weakref},
    {"_abc", PyInit__abc},
    {"_functools", PyInit__functools},
    {"_locale", PyInit__locale},
    {"_opcode", PyInit__opcode},
    {"_operator", PyInit__operator},
    {"_stat", PyInit__stat},
    {"_symtable", PyInit__symtable},
    {"math", PyInit_math},
    {"_math_integer", PyInit__math_integer},
    {"cmath", PyInit_cmath},
    {"_struct", PyInit__struct},
    {"_random", PyInit__random},
    {"_bisect", PyInit__bisect},
    {"_heapq", PyInit__heapq},
    {"array", PyInit_array},
    {"binascii", PyInit_binascii},
    {"_json", PyInit__json},
    {"_pickle", PyInit__pickle},
    {"unicodedata", PyInit_unicodedata},
    {"_statistics", PyInit__statistics},
    {"_zoneinfo", PyInit__zoneinfo},
    {"_csv", PyInit__csv},
    {"_io", PyInit__io},
    {"marshal", PyMarshal_Init},
    {"_imp", PyInit__imp},
    {"_ast", PyInit__ast},
    {"_tokenize", PyInit__tokenize},
    {"builtins", NULL},
    {"sys", NULL},
    {"gc", PyInit_gc},
    {"_contextvars", PyInit__contextvars},
    {"_warnings", _PyWarnings_Init},
    {"_string", PyInit__string},
    {0, 0}
};
