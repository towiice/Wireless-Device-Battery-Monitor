// Optional bridge to HeadsetControl (github.com/Sapd/HeadsetControl), which supports many more
// headsets than the built-in drivers. If headsetcontrol.exe sits next to WirelessBattery.exe or is
// on PATH, it is run with `-o json` and any headset not already handled here is added.
#include "drivers.h"

#include <atomic>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>

namespace {

std::atomic<bool> g_cancel{ false };

// ---- minimal JSON reader (objects, arrays, strings, numbers, literals) -----------------------

struct Json {
    enum Type { Null, Bool, Num, Str, Arr, Obj } type = Null;
    double num = 0;
    std::string str;
    std::vector<Json> arr;
    std::map<std::string, Json> obj;
    const Json* get(const char* k) const {
        auto it = obj.find(k);
        return it == obj.end() ? nullptr : &it->second;
    }
};

struct Parser {
    const char* p;
    const char* e;
    void ws() { while (p < e && (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t')) p++; }
    bool parse(Json& v, int depth = 0) {
        if (depth > 32) return false;
        ws();
        if (p >= e) return false;
        if (*p == '{') {
            v.type = Json::Obj;
            p++;
            ws();
            if (p < e && *p == '}') { p++; return true; }
            for (;;) {
                Json key;
                ws();
                if (p >= e || *p != '"' || !parseString(key.str)) return false;
                ws();
                if (p >= e || *p++ != ':') return false;
                if (!parse(v.obj[key.str], depth + 1)) return false;
                ws();
                if (p < e && *p == ',') { p++; continue; }
                if (p < e && *p == '}') { p++; return true; }
                return false;
            }
        }
        if (*p == '[') {
            v.type = Json::Arr;
            p++;
            ws();
            if (p < e && *p == ']') { p++; return true; }
            for (;;) {
                v.arr.emplace_back();
                if (!parse(v.arr.back(), depth + 1)) return false;
                ws();
                if (p < e && *p == ',') { p++; continue; }
                if (p < e && *p == ']') { p++; return true; }
                return false;
            }
        }
        if (*p == '"') { v.type = Json::Str; return parseString(v.str); }
        if (*p == 't' || *p == 'f' || *p == 'n') {
            v.type = *p == 'n' ? Json::Null : Json::Bool;
            v.num = *p == 't';
            while (p < e && isalpha((unsigned char)*p)) p++;
            return true;
        }
        char* end = nullptr;
        v.num = strtod(p, &end);
        if (end == p) return false;
        v.type = Json::Num;
        p = end;
        return true;
    }
    bool parseString(std::string& out) {
        p++;  // opening quote
        while (p < e && *p != '"') {
            if (*p == '\\' && p + 1 < e) {
                p++;
                char c = *p++;
                switch (c) {
                    case 'n': out += '\n'; break;
                    case 't': out += '\t'; break;
                    case 'u': p = p + 4 <= e ? p + 4 : e; out += '?'; break;
                    default: out += c;
                }
            } else {
                out += *p++;
            }
        }
        if (p >= e) return false;
        p++;
        return true;
    }
};

std::wstring Widen(const std::string& s) {
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(n, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), w.data(), n);
    return w;
}

std::wstring FindExe() {
    wchar_t self[MAX_PATH];
    GetModuleFileNameW(nullptr, self, MAX_PATH);
    std::wstring dir = self;
    dir = dir.substr(0, dir.find_last_of(L"\\/") + 1);
    std::wstring local = dir + L"headsetcontrol.exe";
    if (GetFileAttributesW(local.c_str()) != INVALID_FILE_ATTRIBUTES) return local;
    wchar_t found[MAX_PATH];
    if (SearchPathW(nullptr, L"headsetcontrol.exe", nullptr, MAX_PATH, found, nullptr)) return found;
    return L"";
}

// Runs the command line and returns its stdout (empty on failure, on cancel or after `timeoutMs`).
std::string Run(std::wstring cmd, int timeoutMs) {
    SECURITY_ATTRIBUTES sa{ sizeof(sa), nullptr, TRUE };
    HANDLE rd, wr;
    if (!CreatePipe(&rd, &wr, &sa, 0)) return "";
    SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0);
    STARTUPINFOW si{ sizeof(si) };
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = wr;
    si.hStdError = wr;
    si.hStdInput = nullptr;
    PROCESS_INFORMATION pi{};
    BOOL ok = CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    CloseHandle(wr);
    std::string out;
    if (ok) {
        ULONGLONG end = GetTickCount64() + timeoutMs;
        char buf[4096];
        for (;;) {
            DWORD avail = 0;
            if (!PeekNamedPipe(rd, nullptr, 0, nullptr, &avail, nullptr)) break;  // pipe closed: process done
            if (avail) {
                DWORD n = 0;
                if (!ReadFile(rd, buf, sizeof(buf), &n, nullptr) || !n) break;
                out.append(buf, n);
                if (out.size() > (1 << 20)) break;
                continue;
            }
            if (g_cancel || (LONGLONG)end - (LONGLONG)GetTickCount64() <= 0) { TerminateProcess(pi.hProcess, 1); out.clear(); break; }
            Sleep(20);
        }
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    CloseHandle(rd);
    return out;
}

}  // namespace

void CancelPolling() { g_cancel = true; }

void PollHeadsetControl(PollContext& ctx) {
    static std::wstring exe = FindExe();
    if (exe.empty()) return;
    std::string text = Run(L"\"" + exe + L"\" -o json", 8000);
    size_t start = text.find('{');
    if (start == std::string::npos) return;
    Json root;
    Parser ps{ text.c_str() + start, text.c_str() + text.size() };
    if (!ps.parse(root) || root.type != Json::Obj) return;
    const Json* devices = root.get("devices");
    if (!devices || devices->type != Json::Arr) return;

    for (const Json& d : devices->arr) {
        const Json* vidJ = d.get("id_vendor");
        const Json* pidJ = d.get("id_product");
        const Json* bat = d.get("battery");
        if (!vidJ || !pidJ || !bat || bat->type != Json::Obj) continue;
        USHORT vid = (USHORT)strtoul(vidJ->str.c_str(), nullptr, 16);
        USHORT pid = (USHORT)strtoul(pidJ->str.c_str(), nullptr, 16);
        if (ctx.IsClaimed(vid, pid)) continue;
        const Json* status = bat->get("status");
        const Json* level = bat->get("level");
        std::string st = status ? status->str : "";
        DeviceStatus s;
        s.key = L"hsc:" + Hex4(vid) + L":" + Hex4(pid);
        const Json* name = d.get("device");
        s.name = name && !name->str.empty() ? Widen(name->str) : L"Headset";
        s.kind = DeviceKind::Headset;
        s.via = L"HeadsetControl";
        s.charging = st == "BATTERY_CHARGING";
        s.connected = st == "BATTERY_AVAILABLE" || st == "BATTERY_CHARGING";
        if (s.connected && level && level->type == Json::Num && level->num >= 0) s.percent = Clamp100((int)level->num);
        ctx.Add(s, vid, pid);
    }
}
