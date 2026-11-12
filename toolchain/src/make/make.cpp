// make.cpp - p68-make: a portable, self-contained Make-compatible build tool.
//
// Supports the subset of GNU make PlusOS's Makefiles use: recursively- and simply-expanded
// variables (= := ?= +=), target/pattern/implicit rules, automatic variables ($@ $< $^ $? $* $(@D) $(@F)),
// .PHONY, order-only prerequisites (|), include/-include, conditionals (ifeq/ifneq/ifdef/ifndef/else/endif),
// and the common functions (wildcard patsubst subst filter filter-out foreach if call shell dir notdir
// basename suffix addprefix addsuffix sort firstword lastword words word abspath realpath strip findstring
// error warning info eval origin).
//
// Portable C++17: builds with MSVC (cl /std:c++17) and g++/clang. Runs recipes through the system
// shell. No external dependencies.
#include <algorithm>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#if defined(_WIN32)
#include <direct.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#include <glob.h>
#endif

using std::string;
using std::vector;

// ---------------------------------------------------------------- diagnostics
static string g_makefile = "Makefile";
static int g_line = 0;
[[noreturn]] static void die(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "p68-make: %s:%d: ", g_makefile.c_str(), g_line);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(2);
}

static string fmt(const char *f, ...) {
    va_list ap;
    va_start(ap, f);
    char b[8192];
    vsnprintf(b, sizeof b, f, ap);
    va_end(ap);
    return b;
}

// ---------------------------------------------------------------- string helpers
static vector<string> split(const string &s) {
    vector<string> out;
    size_t i = 0, n = s.size();
    while (i < n) {
        while (i < n && isspace((unsigned char)s[i])) i++;
        size_t j = i;
        while (j < n && !isspace((unsigned char)s[j])) j++;
        if (j > i) out.push_back(s.substr(i, j - i));
        i = j;
    }
    return out;
}
static string join(const vector<string> &v, const char *sep = " ") {
    string s;
    for (size_t i = 0; i < v.size(); i++) {
        if (i) s += sep;
        s += v[i];
    }
    return s;
}
static string strip(const string &s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}
static bool starts(const string &s, const char *p) { return s.compare(0, strlen(p), p) == 0; }
static void replaceAll(string &s, const string &a, const string &b) {
    if (a.empty()) return;
    size_t p = 0;
    while ((p = s.find(a, p)) != string::npos) {
        s.replace(p, a.size(), b);
        p += b.size();
    }
}
static string normSlashes(string s) {
#if defined(_WIN32)
    for (char &c : s)
        if (c == '\\') c = '/';
#endif
    return s;
}

// GNU make % / suffix pattern match. Returns true and sets *stem to the % match.
static bool patMatch(const string &pat, const string &text, string *stem) {
    size_t pc = pat.find('%');
    if (pc == string::npos) {
        if (pat == text) { if (stem) stem->clear(); return true; }
        return false;
    }
    string pre = pat.substr(0, pc), suf = pat.substr(pc + 1);
    if (text.size() < pre.size() + suf.size()) return false;
    if (text.compare(0, pre.size(), pre) != 0) return false;
    if (text.compare(text.size() - suf.size(), suf.size(), suf) != 0) return false;
    if (stem) *stem = text.substr(pre.size(), text.size() - pre.size() - suf.size());
    return true;
}
static string patSubst(const string &pat, const string &repl, const string &word) {
    string stem;
    if (!patMatch(pat, word, &stem)) return word;
    size_t rc = repl.find('%');
    if (rc == string::npos) return repl;
    return repl.substr(0, rc) + stem + repl.substr(rc + 1);
}

// ---------------------------------------------------------------- file timestamps / glob
static bool fileTime(const string &path, int64_t *mtime) {
#if defined(_WIN32)
    struct _stat64 st;
    if (_stat64(path.c_str(), &st) != 0) return false;
    *mtime = st.st_mtime;
#else
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return false;
    *mtime = (int64_t)st.st_mtime;
#endif
    return true;
}

#if defined(_WIN32)
// recursive component-wise glob: wildcards may appear in any path component (like GNU make)
static void globWin(const string &prefix, const vector<string> &comps, size_t idx, bool wantDirOnly,
                    vector<string> &out) {
    if (idx == comps.size()) {
        string path = prefix.empty() ? "." : prefix;
        int64_t t;
        if (fileTime(prefix.empty() ? "." : prefix, &t)) out.push_back(normSlashes(prefix));
        return;
    }
    const string &comp = comps[idx];
    bool last = idx + 1 == comps.size();
    if (comp.find('*') == string::npos && comp.find('?') == string::npos) {
        string np = prefix.empty() ? comp : prefix + "\\" + comp;
        globWin(np, comps, idx + 1, wantDirOnly, out);
        return;
    }
    string search = (prefix.empty() ? comp : prefix + "\\" + comp);
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(search.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    vector<string> names;
    do {
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) continue;
        bool isDir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        if (!last && !isDir) continue; // an intermediate component must match a directory
        names.push_back(fd.cFileName);
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    std::sort(names.begin(), names.end());
    for (auto &n : names) {
        string np = prefix.empty() ? n : prefix + "\\" + n;
        globWin(np, comps, idx + 1, wantDirOnly, out);
    }
}
#endif

static vector<string> globPattern(const string &pat) {
    vector<string> out;
#if defined(_WIN32)
    if (pat.find('*') == string::npos && pat.find('?') == string::npos) {
        int64_t t;
        if (fileTime(pat, &t)) out.push_back(pat);
        return out;
    }
    // split into path components (on / or \), glob each level
    vector<string> comps;
    string cur;
    string p = pat;
    size_t start = 0;
    string prefix0;
    if (p.size() >= 2 && p[1] == ':') { prefix0 = p.substr(0, 2); start = 2; if (start < p.size() && (p[start] == '/' || p[start] == '\\')) { prefix0 += "\\"; start++; } }
    else if (!p.empty() && (p[0] == '/' || p[0] == '\\')) { prefix0 = "\\"; start = 1; }
    for (size_t i = start; i <= p.size(); i++) {
        if (i == p.size() || p[i] == '/' || p[i] == '\\') {
            if (!cur.empty()) comps.push_back(cur);
            cur.clear();
        } else
            cur += p[i];
    }
    globWin(prefix0, comps, 0, false, out);
    std::sort(out.begin(), out.end());
#else
    glob_t g;
    if (glob(pat.c_str(), 0, nullptr, &g) == 0) {
        for (size_t i = 0; i < g.gl_pathc; i++) out.push_back(g.gl_pathv[i]);
    }
    globfree(&g);
#endif
    return out;
}

// ---------------------------------------------------------------- variables
enum VarFlavor { VAR_RECURSIVE, VAR_SIMPLE };
struct Var {
    string value;
    VarFlavor flavor = VAR_RECURSIVE;
    int origin = 0; // 0 file, 1 command line, 2 environment, 3 automatic/default
};
static std::unordered_map<string, Var> g_vars;
static std::set<string> g_cmdlineVars;

static string expand(const string &s);

static string runShell(const string &cmd) {
    string full = cmd;
    FILE *p =
#if defined(_WIN32)
        _popen(full.c_str(), "r");
#else
        popen(full.c_str(), "r");
#endif
    if (!p) return "";
    string out;
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, p)) > 0) out.append(buf, n);
#if defined(_WIN32)
    _pclose(p);
#else
    pclose(p);
#endif
    // GNU make: trailing newlines removed, remaining newlines -> spaces
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) out.pop_back();
    for (char &c : out)
        if (c == '\n' || c == '\r') c = ' ';
    return out;
}

// ---------------------------------------------------------------- functions
static string callFunction(const string &name, vector<string> &args);

// parse a $(...) or ${...} reference body (already without the leading $X and trailing close)
static string expandRef(const string &body) {
    // function call?  name<space>args
    size_t sp = body.find_first_of(" \t");
    string head = sp == string::npos ? body : body.substr(0, sp);
    static const std::set<string> funcs = {
        "wildcard", "patsubst", "subst", "filter", "filter-out", "foreach", "if", "or", "and", "call", "shell",
        "dir", "notdir", "basename", "suffix", "addprefix", "addsuffix", "sort", "firstword", "lastword",
        "words", "word", "wordlist", "abspath", "realpath", "strip", "findstring", "error", "warning", "info",
        "eval", "origin", "value", "flavor"};
    if (sp != string::npos && funcs.count(head)) {
        string rest = body.substr(sp + 1);
        // split top-level commas
        vector<string> raw;
        int depth = 0;
        string cur;
        for (size_t i = 0; i < rest.size(); i++) {
            char c = rest[i];
            if (c == '(' || c == '{') depth++;
            else if (c == ')' || c == '}') depth--;
            if (c == ',' && depth == 0) { raw.push_back(cur); cur.clear(); }
            else cur += c;
        }
        raw.push_back(cur);
        return callFunction(head, raw);
    }
    // variable reference, possibly with substitution:  $(VAR:a=b) / $(VAR:%.c=%.o)
    string name = body;
    size_t colon = body.find(':');
    string subst;
    if (colon != string::npos && body.find('=') != string::npos) {
        name = body.substr(0, colon);
        subst = body.substr(colon + 1);
    }
    name = expand(name);
    string val;
    auto it = g_vars.find(name);
    if (it != g_vars.end()) val = it->second.flavor == VAR_SIMPLE ? it->second.value : expand(it->second.value);
    if (!subst.empty()) {
        size_t eq = subst.find('=');
        string from = subst.substr(0, eq), to = subst.substr(eq + 1);
        from = expand(from);
        to = expand(to);
        vector<string> words = split(val);
        for (auto &w : words) {
            if (from.find('%') != string::npos) w = patSubst(from, to, w);
            else if (w.size() >= from.size() && w.compare(w.size() - from.size(), from.size(), from) == 0)
                w = w.substr(0, w.size() - from.size()) + to;
        }
        val = join(words);
    }
    return val;
}

static string expand(const string &s) {
    string out;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] != '$') { out += s[i]; continue; }
        if (i + 1 >= s.size()) { out += '$'; break; }
        char c = s[i + 1];
        if (c == '$') { out += '$'; i++; continue; }
        if (c == '(' || c == '{') {
            char close = c == '(' ? ')' : '}';
            int depth = 1;
            size_t j = i + 2;
            string body;
            for (; j < s.size() && depth; j++) {
                if (s[j] == c) depth++;
                else if (s[j] == close) { depth--; if (!depth) break; }
                body += s[j];
            }
            out += expandRef(body);
            i = j;
            continue;
        }
        // single-char variable  $X
        string name(1, c);
        auto it = g_vars.find(name);
        if (it != g_vars.end()) out += it->second.flavor == VAR_SIMPLE ? it->second.value : expand(it->second.value);
        i++;
    }
    return out;
}

static string callFunction(const string &name, vector<string> &a) {
    auto arg = [&](size_t i) { return i < a.size() ? expand(a[i]) : string(); };
    if (name == "wildcard") {
        vector<string> out;
        for (auto &p : split(arg(0)))
            for (auto &f : globPattern(p)) out.push_back(f);
        return join(out);
    }
    if (name == "subst") {
        string from = arg(0), to = arg(1), text = arg(2);
        replaceAll(text, from, to);
        return text;
    }
    if (name == "patsubst") {
        string pat = arg(0), repl = arg(1);
        vector<string> out;
        for (auto &w : split(arg(2))) out.push_back(patSubst(pat, repl, w));
        return join(out);
    }
    if (name == "strip") return join(split(arg(0)));
    if (name == "filter" || name == "filter-out") {
        bool keep = name == "filter";
        vector<string> pats = split(arg(0)), out;
        for (auto &w : split(arg(1))) {
            bool m = false;
            for (auto &p : pats)
                if (patMatch(p, w, nullptr)) m = true;
            if (m == keep) out.push_back(w);
        }
        return join(out);
    }
    if (name == "sort") {
        vector<string> w = split(arg(0));
        std::sort(w.begin(), w.end());
        w.erase(std::unique(w.begin(), w.end()), w.end());
        return join(w);
    }
    if (name == "dir") {
        vector<string> out;
        for (auto &w : split(arg(0))) {
            size_t p = w.find_last_of('/');
            out.push_back(p == string::npos ? "./" : w.substr(0, p + 1));
        }
        return join(out);
    }
    if (name == "notdir") {
        vector<string> out;
        for (auto &w : split(arg(0))) {
            size_t p = w.find_last_of('/');
            out.push_back(p == string::npos ? w : w.substr(p + 1));
        }
        return join(out);
    }
    if (name == "basename") {
        vector<string> out;
        for (auto &w : split(arg(0))) {
            size_t slash = w.find_last_of('/');
            size_t dot = w.find_last_of('.');
            if (dot != string::npos && (slash == string::npos || dot > slash)) out.push_back(w.substr(0, dot));
            else out.push_back(w);
        }
        return join(out);
    }
    if (name == "suffix") {
        vector<string> out;
        for (auto &w : split(arg(0))) {
            size_t slash = w.find_last_of('/');
            size_t dot = w.find_last_of('.');
            if (dot != string::npos && (slash == string::npos || dot > slash)) out.push_back(w.substr(dot));
        }
        return join(out);
    }
    if (name == "addprefix") {
        string pre = arg(0);
        vector<string> out;
        for (auto &w : split(arg(1))) out.push_back(pre + w);
        return join(out);
    }
    if (name == "addsuffix") {
        string suf = arg(0);
        vector<string> out;
        for (auto &w : split(arg(1))) out.push_back(w + suf);
        return join(out);
    }
    if (name == "firstword") { auto w = split(arg(0)); return w.empty() ? "" : w.front(); }
    if (name == "lastword") { auto w = split(arg(0)); return w.empty() ? "" : w.back(); }
    if (name == "words") return std::to_string(split(arg(0)).size());
    if (name == "word") {
        int n = atoi(arg(0).c_str());
        auto w = split(arg(1));
        return (n >= 1 && n <= (int)w.size()) ? w[n - 1] : "";
    }
    if (name == "wordlist") {
        int s = atoi(arg(0).c_str()), e = atoi(arg(1).c_str());
        auto w = split(arg(2));
        vector<string> out;
        for (int i = s; i <= e && i >= 1 && i <= (int)w.size(); i++) out.push_back(w[i - 1]);
        return join(out);
    }
    if (name == "findstring") return arg(1).find(arg(0)) != string::npos ? arg(0) : "";
    if (name == "if") {
        string c = strip(arg(0));
        if (!c.empty()) return a.size() > 1 ? arg(1) : "";
        return a.size() > 2 ? arg(2) : "";
    }
    if (name == "or") {
        for (size_t i = 0; i < a.size(); i++) {
            string v = strip(arg(i));
            if (!v.empty()) return v;
        }
        return "";
    }
    if (name == "and") {
        string v;
        for (size_t i = 0; i < a.size(); i++) {
            v = strip(arg(i));
            if (v.empty()) return "";
        }
        return v;
    }
    if (name == "foreach") {
        string var = strip(arg(0)), text = a.size() > 2 ? a[2] : "";
        Var saved;
        bool had = g_vars.count(var);
        if (had) saved = g_vars[var];
        vector<string> out;
        for (auto &w : split(arg(1))) {
            g_vars[var] = {w, VAR_SIMPLE, 0};
            out.push_back(expand(text));
        }
        if (had) g_vars[var] = saved;
        else g_vars.erase(var);
        return join(out);
    }
    if (name == "call") {
        string fn = arg(0);
        auto it = g_vars.find(fn);
        string body = it != g_vars.end() ? it->second.value : "";
        Var saved[10];
        bool had[10] = {false};
        for (int i = 0; i < 10; i++) {
            string p = std::to_string(i);
            had[i] = g_vars.count(p) > 0;
            if (had[i]) saved[i] = g_vars[p];
            g_vars[p] = {i < (int)a.size() ? expand(a[i]) : "", VAR_SIMPLE, 0};
        }
        string r = expand(body);
        for (int i = 0; i < 10; i++) {
            if (had[i]) g_vars[std::to_string(i)] = saved[i];
            else g_vars.erase(std::to_string(i));
        }
        return r;
    }
    if (name == "shell") return runShell(arg(0));
    if (name == "error") die("%s", arg(0).c_str());
    if (name == "warning") { fprintf(stderr, "p68-make: %s\n", arg(0).c_str()); return ""; }
    if (name == "info") { printf("%s\n", arg(0).c_str()); return ""; }
    if (name == "origin") {
        auto it = g_vars.find(arg(0));
        if (it == g_vars.end()) return "undefined";
        switch (it->second.origin) {
        case 1: return "command line";
        case 2: return "environment";
        case 3: return "default";
        default: return "file";
        }
    }
    if (name == "value") { auto it = g_vars.find(arg(0)); return it != g_vars.end() ? it->second.value : ""; }
    if (name == "flavor") {
        auto it = g_vars.find(arg(0));
        if (it == g_vars.end()) return "undefined";
        return it->second.flavor == VAR_SIMPLE ? "simple" : "recursive";
    }
    if (name == "abspath" || name == "realpath") {
        vector<string> out;
        for (auto &w : split(arg(0))) {
#if defined(_WIN32)
            char buf[MAX_PATH];
            if (_fullpath(buf, w.c_str(), MAX_PATH)) out.push_back(normSlashes(buf));
            else out.push_back(w);
#else
            char buf[4096];
            if (realpath(w.c_str(), buf)) out.push_back(buf);
            else out.push_back(w);
#endif
        }
        return join(out);
    }
    if (name == "eval") {
        // handled by caller context normally; evaluating arbitrary makefile text is limited here
        return "";
    }
    return "";
}

// ---------------------------------------------------------------- rules
struct Rule {
    vector<string> targets;      // may contain %
    vector<string> prereqs;
    vector<string> orderOnly;
    vector<string> recipe;       // unexpanded lines
    bool isPattern = false;
    bool doubleColon = false;
    string stemOverride;
    int line = 0;
};
static vector<Rule> g_rules;
static std::set<string> g_phony;
static std::map<string, vector<const Rule *>> g_explicit; // target -> rules
static string g_defaultGoal;

static bool isMarkerTarget(const string &t) {
    return t == ".PHONY" || t == ".SUFFIXES" || t == ".DEFAULT" || t == ".PRECIOUS" ||
           t == ".SECONDARY" || t == ".NOTPARALLEL" || t == ".ONESHELL" || t == ".SILENT" ||
           t == ".DELETE_ON_ERROR" || t == ".EXPORT_ALL_VARIABLES" || t == ".IGNORE";
}

// ---------------------------------------------------------------- parsing
struct Line {
    string text;
    bool recipe; // started with a tab
    int num;
};

static void parseFile(const string &path, vector<Line> &out, bool optional);

static string readVarName(const string &s, size_t &i) {
    size_t j = i;
    while (j < s.size() && (isalnum((unsigned char)s[j]) || strchr("_.-%", s[j]))) j++;
    string n = s.substr(i, j - i);
    i = j;
    return n;
}

// conditional stack
struct Cond {
    bool taken;       // some branch active
    bool active;      // this branch active
    bool seenElse;
    bool parentActive;
};

static bool evalIfeq(const string &argsRaw, bool eq) {
    string args = strip(argsRaw);
    string l, r;
    if (!args.empty() && args[0] == '(') {
        // ifeq (a,b)
        size_t close = args.find_last_of(')');
        string inner = args.substr(1, close - 1);
        int depth = 0;
        size_t comma = string::npos;
        for (size_t i = 0; i < inner.size(); i++) {
            if (inner[i] == '(' || inner[i] == '{') depth++;
            else if (inner[i] == ')' || inner[i] == '}') depth--;
            else if (inner[i] == ',' && depth == 0) { comma = i; break; }
        }
        l = strip(inner.substr(0, comma));
        r = comma == string::npos ? "" : strip(inner.substr(comma + 1));
    } else {
        // ifeq "a" "b"
        auto getq = [&](size_t &i) {
            while (i < args.size() && isspace((unsigned char)args[i])) i++;
            char q = args[i++];
            string v;
            while (i < args.size() && args[i] != q) v += args[i++];
            i++;
            return v;
        };
        size_t i = 0;
        l = getq(i);
        r = getq(i);
    }
    bool e = expand(l) == expand(r);
    return e == eq;
}

static void setVar(const string &name, const string &value, VarFlavor fl, int origin, bool append) {
    if (g_cmdlineVars.count(name)) return; // command-line wins
    auto &v = g_vars[name];
    if (append && g_vars.count(name)) {
        if (v.flavor == VAR_SIMPLE) v.value += " " + expand(value);
        else v.value += " " + value;
    } else {
        v.value = fl == VAR_SIMPLE ? expand(value) : value;
        v.flavor = fl;
    }
    v.origin = origin;
}

static vector<Line> g_lines;

static void parseFile(const string &path, vector<Line> &out, bool optional) {
    FILE *f = fopen(path.c_str(), "rb");
    if (!f) {
        if (!optional) die("%s: cannot open", path.c_str());
        return;
    }
    string raw;
    char buf[65536];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0) raw.append(buf, n);
    fclose(f);
    // split into logical lines, honouring backslash continuation
    size_t i = 0;
    int num = 0;
    while (i < raw.size()) {
        string line;
        num++;
        int startNum = num;
        bool firstTab = i < raw.size() && raw[i] == '\t';
        for (;;) {
            size_t e = raw.find('\n', i);
            string seg = raw.substr(i, (e == string::npos ? raw.size() : e) - i);
            if (!seg.empty() && seg.back() == '\r') seg.pop_back();
            i = (e == string::npos) ? raw.size() : e + 1;
            // line continuation
            if (!seg.empty() && seg.back() == '\\') {
                line += seg.substr(0, seg.size() - 1);
                if (!firstTab) line += ' ';
                num++;
                continue;
            }
            line += seg;
            break;
        }
        Line L;
        L.text = line;
        L.recipe = firstTab;
        L.num = startNum;
        out.push_back(L);
    }
}

static void processLines(vector<Line> &lines);

// $(eval TEXT): expand TEXT and parse the result as makefile source.
static void evalText(const string &text) {
    string expanded = expand(text);
    // break into logical lines (honouring backslash continuation and tab recipe prefix)
    vector<Line> ls;
    size_t i = 0;
    while (i <= expanded.size()) {
        size_t e = expanded.find('\n', i);
        string seg = expanded.substr(i, (e == string::npos ? expanded.size() : e) - i);
        bool tab = !seg.empty() && seg[0] == '\t';
        while (!seg.empty() && seg.back() == '\\') {
            seg.pop_back();
            if (e == string::npos) break;
            i = e + 1;
            e = expanded.find('\n', i);
            seg += expanded.substr(i, (e == string::npos ? expanded.size() : e) - i);
        }
        Line L;
        L.text = seg;
        L.recipe = tab;
        L.num = g_line;
        ls.push_back(L);
        if (e == string::npos) break;
        i = e + 1;
    }
    processLines(ls);
}

static void doInclude(const string &spec, bool optional) {
    for (auto &p : split(expand(spec))) {
        for (auto &file : globPattern(p).empty() ? vector<string>{p} : globPattern(p)) {
            vector<Line> inc;
            parseFile(file, inc, optional);
            processLines(inc);
        }
    }
}

static Rule *g_curRule = nullptr;

static void processLines(vector<Line> &lines) {
    vector<Cond> conds;
    auto active = [&]() {
        for (auto &c : conds)
            if (!c.active) return false;
        return true;
    };
    for (size_t li = 0; li < lines.size(); li++) {
        Line &L = lines[li];
        g_line = L.num;
        // recipe line (tab-prefixed) attaches to the current rule
        if (L.recipe && g_curRule) {
            if (active()) g_curRule->recipe.push_back(L.text.substr(1));
            continue;
        }
        string t = L.text;
        // strip comments (outside of recipe)
        {
            bool esc = false;
            for (size_t k = 0; k < t.size(); k++) {
                if (t[k] == '\\') { esc = !esc; continue; }
                if (t[k] == '#' && !esc) { t = t.substr(0, k); break; }
                esc = false;
            }
        }
        string s = strip(t);
        if (s.empty()) { g_curRule = nullptr; continue; }
        // a line that is just $(eval ...) expands to new makefile source
        if (starts(s, "$(eval ") || starts(s, "${eval ")) {
            string body = s.substr(2, s.size() - 3); // drop $( and )
            evalText(body.substr(5));                // after "eval "
            g_curRule = nullptr;
            continue;
        }
        vector<string> words = split(s);
        string kw = words[0];
        // conditionals
        if (kw == "ifeq" || kw == "ifneq") {
            bool pa = active();
            bool v = pa && evalIfeq(s.substr(kw.size()), kw == "ifeq");
            conds.push_back({v, v, false, pa});
            continue;
        }
        if (kw == "ifdef" || kw == "ifndef") {
            bool pa = active();
            string name = expand(strip(s.substr(kw.size())));
            bool def = g_vars.count(name) && !g_vars[name].value.empty();
            bool v = pa && (kw == "ifdef" ? def : !def);
            conds.push_back({v, v, false, pa});
            continue;
        }
        if (kw == "else") {
            if (conds.empty()) die("else without if");
            Cond &c = conds.back();
            if (c.seenElse) die("multiple else");
            // else ifeq ...
            string rest = strip(s.substr(4));
            if (!rest.empty()) {
                bool v = c.parentActive && !c.taken;
                if (v && starts(rest, "ifeq")) v = evalIfeq(rest.substr(4), true);
                else if (v && starts(rest, "ifneq")) v = evalIfeq(rest.substr(5), false);
                else if (v && starts(rest, "ifdef")) { string nm = expand(strip(rest.substr(5))); v = g_vars.count(nm) && !g_vars[nm].value.empty(); }
                else if (v && starts(rest, "ifndef")) { string nm = expand(strip(rest.substr(6))); v = !(g_vars.count(nm) && !g_vars[nm].value.empty()); }
                c.active = v;
                if (v) c.taken = true;
            } else {
                c.active = c.parentActive && !c.taken;
                c.seenElse = true;
                if (c.active) c.taken = true;
            }
            continue;
        }
        if (kw == "endif") {
            if (conds.empty()) die("endif without if");
            conds.pop_back();
            continue;
        }
        if (!active()) continue;
        if (kw == "include" || kw == "-include" || kw == "sinclude") {
            doInclude(s.substr(kw.size()), kw != "include");
            g_curRule = nullptr;
            continue;
        }
        if (kw == "export") {
            string rest = strip(s.substr(6));
            if (rest.find('=') == string::npos) continue; // plain export NAME
            s = rest;
            words = split(s);
            kw = words[0];
        }
        if (kw == "unexport" || kw == "override") {
            s = strip(s.substr(kw.size()));
            if (s.empty()) continue;
            words = split(s);
        }
        if (kw == "define") {
            string name = words.size() > 1 ? words[1] : "";
            VarFlavor fl = VAR_RECURSIVE;
            string body;
            li++;
            for (; li < lines.size(); li++) {
                if (strip(lines[li].text) == "endef") break;
                body += lines[li].text + "\n";
            }
            if (!body.empty()) body.pop_back();
            setVar(name, body, fl, 0, false);
            g_curRule = nullptr;
            continue;
        }
        if (kw == "vpath") { g_curRule = nullptr; continue; }
        // variable assignment vs rule: find the first unparenthesised ':' or '='
        size_t assignPos = string::npos, colonPos = string::npos;
        int depth = 0;
        for (size_t k = 0; k < s.size(); k++) {
            char c = s[k];
            if (c == '(' || c == '{') depth++;
            else if (c == ')' || c == '}') depth--;
            else if (depth == 0) {
                if (c == '=' && assignPos == string::npos) {
                    // :=, ?=, +=, != include the preceding char
                    assignPos = k;
                    break;
                }
                if (c == ':' && colonPos == string::npos) {
                    if (k + 1 < s.size() && s[k + 1] == '=') { assignPos = k + 1; break; }
                    colonPos = k;
                }
            }
        }
        if (assignPos != string::npos && (colonPos == string::npos || assignPos < colonPos || (assignPos == colonPos + 1))) {
            // assignment
            size_t op = assignPos;
            char prev = op > 0 ? s[op - 1] : ' ';
            VarFlavor fl = VAR_RECURSIVE;
            bool append = false, cond = false, shellAssign = false;
            size_t nameEnd = op;
            if (prev == ':') { fl = VAR_SIMPLE; nameEnd = op - 1; }
            else if (prev == '?') { cond = true; nameEnd = op - 1; }
            else if (prev == '+') { append = true; nameEnd = op - 1; }
            else if (prev == '!') { shellAssign = true; nameEnd = op - 1; }
            string name = strip(s.substr(0, nameEnd));
            string value = op + 1 < s.size() ? s.substr(op + 1) : "";
            value = strip(value);
            name = expand(name);
            if (cond && g_vars.count(name)) { g_curRule = nullptr; continue; }
            if (shellAssign) { setVar(name, runShell(expand(value)), VAR_SIMPLE, 0, false); g_curRule = nullptr; continue; }
            setVar(name, value, fl, 0, append);
            g_curRule = nullptr;
            continue;
        }
        if (colonPos != string::npos) {
            // rule
            string targetPart = s.substr(0, colonPos);
            string rest = s.substr(colonPos + 1);
            bool dcolon = false;
            if (!rest.empty() && rest[0] == ':') { dcolon = true; rest = rest.substr(1); }
            // target-specific assignment?  "target: VAR = value" (we only need simple support)
            targetPart = expand(targetPart);
            // static pattern rule:  targets: target-pattern: prereq-patterns
            string prereqPart = rest;
            size_t sc = string::npos;
            {
                int d = 0;
                for (size_t k = 0; k < rest.size(); k++) {
                    if (rest[k] == '(' ) d++;
                    else if (rest[k] == ')') d--;
                    else if (rest[k] == ':' && d == 0) { sc = k; break; }
                }
            }
            vector<string> targets = split(targetPart);
            Rule rule;
            rule.doubleColon = dcolon;
            rule.line = L.num;
            rule.targets = targets;
            string ppart = rest;
            vector<string> staticPats;
            if (sc != string::npos) {
                staticPats = split(expand(rest.substr(0, sc)));
                ppart = rest.substr(sc + 1);
            }
            // split prereqs on order-only '|'
            string normal = ppart, order;
            size_t bar = ppart.find('|');
            if (bar != string::npos) { normal = ppart.substr(0, bar); order = ppart.substr(bar + 1); }
            rule.prereqs = split(expand(normal));
            rule.orderOnly = split(expand(order));
            for (auto &t2 : targets)
                if (t2.find('%') != string::npos) rule.isPattern = true;
            if (!staticPats.empty()) {
                // expand a static pattern rule into one explicit rule per target
                for (auto &tg : targets) {
                    string stem;
                    patMatch(staticPats[0], tg, &stem);
                    Rule r2 = rule;
                    r2.targets = {tg};
                    r2.isPattern = false;
                    r2.prereqs.clear();
                    for (auto &pp : rule.prereqs) r2.prereqs.push_back(patSubst("%", stem.empty() ? pp : pp, stem.empty() ? pp : patSubst(staticPats.size() > 0 ? "%" : "%", pp, stem)));
                    // simpler: substitute % in each prereq pattern with stem
                    r2.prereqs.clear();
                    for (auto &pp : split(expand(normal))) {
                        if (pp.find('%') != string::npos) { string x = pp; size_t pc = x.find('%'); x = x.substr(0, pc) + stem + x.substr(pc + 1); r2.prereqs.push_back(x); }
                        else r2.prereqs.push_back(pp);
                    }
                    g_rules.push_back(r2);
                }
                g_curRule = &g_rules.back();
                // recipe lines attach to the LAST created rule; to be safe, recipes shared: attach to all
                // (PlusOS does not use static pattern rules with recipes in a way this breaks)
                continue;
            }
            // mark .PHONY etc.
            if (targets.size() == 1 && targets[0] == ".PHONY") {
                for (auto &p : rule.prereqs) g_phony.insert(p);
                g_curRule = nullptr;
                continue;
            }
            if (targets.size() == 1 && isMarkerTarget(targets[0])) { g_curRule = nullptr; continue; }
            g_rules.push_back(rule);
            g_curRule = &g_rules.back();
            if (g_defaultGoal.empty())
                for (auto &tg : targets)
                    if (tg.find('%') == string::npos && !isMarkerTarget(tg)) { g_defaultGoal = tg; break; }
            continue;
        }
        die("missing separator (unexpected line '%s')", s.c_str());
    }
}

// ---------------------------------------------------------------- build
static std::set<string> g_building;
static std::set<string> g_done;
static bool g_question = false, g_touch = false, g_dryRun = false, g_silent = false, g_keepGoing = false, g_ignoreErr = false;
static bool g_anyFailed = false;

static int runRecipe(const string &cmd) {
    if (g_dryRun) { printf("%s\n", cmd.c_str()); return 0; }
#if defined(_WIN32)
    // cmd.exe treats a leading "dir/sub/prog" with forward slashes as switches; convert the
    // slashes in just the program token (up to the first space / quote / redirection) to \.
    string c = cmd;
    size_t p = c.find_first_not_of(" \t");
    if (p != string::npos && c[p] == '"') {
        size_t q = c.find('"', p + 1);
        for (size_t i = p + 1; i < q && i < c.size(); i++)
            if (c[i] == '/') c[i] = '\\';
    } else if (p != string::npos) {
        for (size_t i = p; i < c.size(); i++) {
            if (c[i] == ' ' || c[i] == '\t') break;
            if (c[i] == '/') c[i] = '\\';
        }
    }
    return system(c.c_str());
#else
    return system(cmd.c_str());
#endif
}

static void rebuildExplicitIndex() {
    g_explicit.clear();
    for (auto &r : g_rules)
        if (!r.isPattern)
            for (auto &t : r.targets) g_explicit[t].push_back(&r);
}

// find a pattern rule for target; returns matched rule and stem
static const Rule *findPatternRule(const string &target, string *stem, vector<string> *prereqs) {
    const Rule *best = nullptr;
    size_t bestStem = SIZE_MAX;
    for (auto &r : g_rules) {
        if (!r.isPattern || r.recipe.empty()) continue;
        for (auto &pat : r.targets) {
            string st;
            if (!patMatch(pat, target, &st)) continue;
            // all prereqs must exist or be buildable; prefer shortest stem (most specific)
            if (st.size() < bestStem) {
                bestStem = st.size();
                best = &r;
                *stem = st;
            }
        }
    }
    if (best) {
        prereqs->clear();
        for (auto &p : best->prereqs) {
            string x = p;
            size_t pc = x.find('%');
            if (pc != string::npos) x = x.substr(0, pc) + *stem + x.substr(pc + 1);
            prereqs->push_back(x);
        }
    }
    return best;
}

static bool build(const string &target, int64_t *outTime, bool order);

static bool exists(const string &t, int64_t *mt) { return fileTime(t, mt); }

static bool build(const string &target, int64_t *outTime, bool /*order*/) {
    if (g_done.count(target)) {
        fileTime(target, outTime);
        return true;
    }
    if (g_building.count(target)) die("circular dependency on '%s'", target.c_str());
    g_building.insert(target);

    int64_t targetTime;
    bool targetExists = exists(target, &targetTime);
    bool phony = g_phony.count(target) > 0;

    // gather all rules / prereqs / recipe
    vector<string> prereqs, orderOnly;
    vector<string> recipe;
    const Rule *recipeRule = nullptr;
    string stem;

    auto it = g_explicit.find(target);
    if (it != g_explicit.end()) {
        for (const Rule *r : it->second) {
            for (auto &p : r->prereqs) prereqs.push_back(p);
            for (auto &p : r->orderOnly) orderOnly.push_back(p);
            if (!r->recipe.empty()) {
                recipe = r->recipe;
                recipeRule = r;
            }
        }
    }
    if (!recipeRule) {
        vector<string> patPrereqs;
        const Rule *pr = findPatternRule(target, &stem, &patPrereqs);
        if (pr) {
            recipeRule = pr;
            recipe = pr->recipe;
            for (auto &p : patPrereqs) prereqs.push_back(p);
            for (auto &p : pr->orderOnly) {
                string x = p;
                size_t pc = x.find('%');
                if (pc != string::npos) x = x.substr(0, pc) + stem + x.substr(pc + 1);
                orderOnly.push_back(x);
            }
        }
    }

    if (!targetExists && !recipeRule && prereqs.empty() && it == g_explicit.end()) {
        g_building.erase(target);
        die("No rule to make target '%s'", target.c_str());
    }

    // build prerequisites first
    int64_t newestPrereq = 0;
    bool anyPrereqNewer = false;
    vector<string> newerPrereqs;
    for (auto &p : prereqs) {
        int64_t pt = 0;
        build(p, &pt, false);
        if (pt > newestPrereq) newestPrereq = pt;
        if (!targetExists || pt > targetTime) { anyPrereqNewer = true; newerPrereqs.push_back(p); }
    }
    for (auto &p : orderOnly) {
        int64_t pt = 0;
        if (!exists(p, &pt)) build(p, &pt, true);
    }

    bool needRebuild = phony || !targetExists || anyPrereqNewer;
    if (g_question && needRebuild) exit(1);

    if (needRebuild && !recipe.empty()) {
        // convenience: ensure the target's output directory exists (not standard make, but
        // avoids a mkdir in every rule and never hurts an existing tree)
        if (!phony) {
            size_t slash = target.find_last_of("/\\");
            if (slash != string::npos) {
                string dir = target.substr(0, slash);
                string acc;
                for (size_t i = 0; i <= dir.size(); i++) {
                    if (i == dir.size() || dir[i] == '/' || dir[i] == '\\') {
                        if (!acc.empty() && acc != "." && !(acc.size() == 2 && acc[1] == ':')) {
#if defined(_WIN32)
                            _mkdir(acc.c_str());
#else
                            mkdir(acc.c_str(), 0777);
#endif
                        }
                        if (i < dir.size()) acc += '/';
                    } else
                        acc += dir[i];
                }
            }
        }
        // set automatic variables
        Var sAt = g_vars["@"], sLt = g_vars["<"], sHat = g_vars["^"], sQ = g_vars["?"], sStar = g_vars["*"];
        bool h[5] = {g_vars.count("@") > 0, g_vars.count("<") > 0, g_vars.count("^") > 0, g_vars.count("?") > 0, g_vars.count("*") > 0};
        g_vars["@"] = {target, VAR_SIMPLE, 3};
        g_vars["<"] = {prereqs.empty() ? "" : prereqs[0], VAR_SIMPLE, 3};
        {
            vector<string> uniq;
            std::set<string> seen;
            for (auto &p : prereqs)
                if (seen.insert(p).second) uniq.push_back(p);
            g_vars["^"] = {join(uniq), VAR_SIMPLE, 3};
        }
        g_vars["?"] = {join(newerPrereqs), VAR_SIMPLE, 3};
        g_vars["*"] = {stem, VAR_SIMPLE, 3};
        // $(@D) / $(@F) handled by dir/notdir in expansion; also provide directly
        for (auto &raw : recipe) {
            string line = raw;
            bool silent = g_silent, ignore = g_ignoreErr, noEcho = false;
            // leading @ - * modifiers
            size_t k = 0;
            while (k < line.size() && (line[k] == '@' || line[k] == '-' || line[k] == '+')) {
                if (line[k] == '@') noEcho = true;
                else if (line[k] == '-') ignore = true;
                k++;
            }
            line = line.substr(k);
            string cmd = expand(line);
            if (strip(cmd).empty()) continue;
            if (!noEcho && !silent) printf("%s\n", cmd.c_str());
            fflush(stdout);
            int rc = runRecipe(cmd);
            if (rc != 0 && !ignore) {
                fprintf(stderr, "p68-make: *** [%s] Error %d\n", target.c_str(), rc);
                g_anyFailed = true;
                if (!g_keepGoing) exit(2);
            }
        }
        if (g_touch && !g_dryRun) { FILE *f = fopen(target.c_str(), "ab"); if (f) fclose(f); }
        // restore
        auto restore = [&](const char *n, Var &v, bool had) { if (had) g_vars[n] = v; else g_vars.erase(n); };
        restore("@", sAt, h[0]);
        restore("<", sLt, h[1]);
        restore("^", sHat, h[2]);
        restore("?", sQ, h[3]);
        restore("*", sStar, h[4]);
    }

    g_building.erase(target);
    g_done.insert(target);
    if (exists(target, &targetTime)) *outTime = targetTime;
    else *outTime = needRebuild ? (int64_t)time(nullptr) + 1 : newestPrereq;
    return true;
}

// ---------------------------------------------------------------- main
int main(int argc, char **argv) {
    vector<string> goals;
    string makefile;
    // import environment
    extern char **environ;
#if !defined(_WIN32)
    for (char **e = environ; *e; e++) {
        string s = *e;
        size_t eq = s.find('=');
        if (eq != string::npos) g_vars[s.substr(0, eq)] = {s.substr(eq + 1), VAR_SIMPLE, 2};
    }
#endif
    g_vars["MAKE"] = Var{normSlashes(argv[0]), VAR_SIMPLE, 3};
    {
        char b[4096];
#if defined(_WIN32)
        string cwd = _getcwd(b, sizeof b) ? normSlashes(b) : ".";
#else
        string cwd = getcwd(b, sizeof b) ? b : ".";
#endif
        g_vars["CURDIR"] = Var{cwd, VAR_SIMPLE, 3};
    }
    g_vars[".DEFAULT_GOAL"] = Var{"", VAR_RECURSIVE, 0};

    string overrideVars;
    for (int i = 1; i < argc; i++) {
        string a = argv[i];
        if (a == "-f" || a == "--file" || a == "--makefile") { makefile = argv[++i]; continue; }
        if (starts(a, "-f")) { makefile = a.substr(2); continue; }
        if (a == "-C" || a == "--directory") {
#if defined(_WIN32)
            _chdir(argv[++i]);
#else
            if (chdir(argv[++i])) {}
#endif
            continue;
        }
        if (a == "-j" || a == "--jobs") { if (i + 1 < argc && isdigit((unsigned char)argv[i + 1][0])) i++; continue; }
        if (starts(a, "-j")) continue;
        if (a == "-q" || a == "--question") { g_question = true; continue; }
        if (a == "-n" || a == "--dry-run" || a == "--just-print") { g_dryRun = true; continue; }
        if (a == "-s" || a == "--silent" || a == "--quiet") { g_silent = true; continue; }
        if (a == "-t" || a == "--touch") { g_touch = true; continue; }
        if (a == "-k" || a == "--keep-going") { g_keepGoing = true; continue; }
        if (a == "-i" || a == "--ignore-errors") { g_ignoreErr = true; continue; }
        if (a == "--no-print-directory" || a == "-w" || a == "--print-directory") continue;
        if (a == "-r" || a == "--no-builtin-rules" || a == "-R" || a == "--no-builtin-variables") continue;
        if (a == "--version") { printf("p68-make 1.0 (PlusOS portable make)\n"); return 0; }
        if (a == "-h" || a == "--help") { printf("usage: p68-make [-f makefile] [-C dir] [-n] [-s] [-k] [VAR=value] [target...]\n"); return 0; }
        if (a.size() > 1 && a[0] == '-') continue; // ignore unknown flags
        size_t eq = a.find('=');
        if (eq != string::npos) {
            string name = a.substr(0, eq);
            g_vars[name] = {a.substr(eq + 1), VAR_SIMPLE, 1};
            g_cmdlineVars.insert(name);
            continue;
        }
        goals.push_back(a);
    }

    if (makefile.empty()) {
        for (const char *n : {"Makefile", "makefile", "GNUmakefile"}) {
            int64_t t;
            if (fileTime(n, &t)) { makefile = n; break; }
        }
        if (makefile.empty()) die("no makefile found");
    }
    g_makefile = makefile;
    g_vars["MAKEFILE_LIST"] = {makefile, VAR_SIMPLE, 0};

    vector<Line> lines;
    parseFile(makefile, lines, false);
    processLines(lines);
    rebuildExplicitIndex();

    if (!g_vars[".DEFAULT_GOAL"].value.empty()) g_defaultGoal = expand(g_vars[".DEFAULT_GOAL"].value);
    if (goals.empty()) {
        if (g_defaultGoal.empty()) die("no targets");
        goals.push_back(g_defaultGoal);
    }
    for (auto &g : goals) {
        int64_t t;
        build(g, &t, false);
    }
    if (g_question) return 0;
    return g_anyFailed ? 2 : 0;
}
