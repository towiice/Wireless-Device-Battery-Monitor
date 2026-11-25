// 2.4 GHz dongles of the "qmk.top" web-configurator family (VID 3151 and a dozen others), found in
// 8K wireless mice such as the Kysona Jupiter Pro (3151:402D). The dongle answers on a vendor
// collection (usage page FFFF, usage 2, USB interface 2) with one 64-byte feature report (id 0):
//   request:  [0][F7][0 x 63]
//   reply:    [0][canRead][keyboard %][mouse %][keyboard offline][mouse offline][canSend] ...
// "offline" bytes are 0 when the paired device is connected. The dongle can host a keyboard and a
// mouse at once, so both are reported when present. A slot that has never been online since the app
// started is left out: most dongles only have a mouse paired.
// Protocol read from the vendor's own web configurator (https://qmk.top/, class "___rawGetStatus"
// and the dongle table in its bundle); verified against a 3151:402D dongle with a Kysona Jupiter Pro.
#include "drivers.h"

#include <set>
#include <string>

namespace {

struct Family {
    USHORT vid;
    std::vector<USHORT> pids;
};

// Dongle ids listed by the vendor's configurator (usage 2, page FFFF, interface 2).
const Family kFamilies[] = {
    { 0x3151, { 0x4011, 0x4014, 0x4017, 0x4021, 0x4027, 0x402D, 0x4038, 0x5006, 0x5007, 0x5008, 0x5020, 0x5026, 0x502B, 0x5031,
                0x5037, 0x5038, 0x503D, 0x503F, 0x5040, 0x5042, 0x5045, 0x504B, 0x504E, 0x5050, 0x5052, 0x5055, 0x505A } },
    { 0x39AB, { 0x7003, 0x7006, 0x7007, 0x7008, 0x7009 } },
    { 0x25AA, { 0x2008, 0x4008, 0x4018 } },
    { 0x342D, { 0xE3E0, 0xE3E1, 0xE3E2, 0xE3E6, 0xE3E7, 0xE3E8, 0xE3F5, 0xE3F6, 0xE3F7, 0xE3FB, 0xE3FC, 0xE3FD } },
    { 0x374A, { 0xA205, 0xA217, 0xA220, 0xA223, 0xA226, 0xA229, 0xA234, 0xA237, 0xA251, 0xA263, 0xA268, 0xA311, 0xA314, 0xA319,
                0xA322, 0xA329, 0xA355 } },
    { 0x347A, { 0xA302, 0xA305 } },
    { 0x0DB0, { 0x197E, 0x8E9A } },
    { 0x379A, { 0x1804, 0x1807 } },
    { 0x38A9, { 0x0002 } },
    { 0x145F, { 0x0376, 0x038D } },
    { 0x38EE, { 0x0000, 0x0002, 0x0039 } },
    { 0x391D, { 0x1A04, 0x1A08, 0x1A0E, 0x1A13, 0x1A25, 0x1A2B, 0x1A34, 0x2017, 0x2A05 } },
    { 0x14A5, { 0xC045 } },
};

// Mouse model for dongles whose product string only says "2.4G Wireless Mouse".
const wchar_t* KnownMouseName(USHORT vid, USHORT pid) {
    if (vid == 0x3151 && pid == 0x402D) return L"Kysona Jupiter Pro";
    return nullptr;
}

bool KnownDongle(USHORT vid, USHORT pid) {
    for (const Family& f : kFamilies) {
        if (f.vid != vid) continue;
        for (USHORT p : f.pids)
            if (p == pid) return true;
    }
    return false;
}

// Sends the status request and returns the reply (r[0] = report id).
bool QueryStatus(HidDev& d, std::vector<BYTE>& r) {
    for (int attempt = 0; attempt < 3; attempt++) {
        std::vector<BYTE> w(65, 0);
        w[1] = 0xF7;
        if (!d.SetFeature(w)) return false;
        Sleep(20 + attempt * 30);
        if (d.GetFeature(0, r) && r.size() >= 7) {
            // canRead and canSend are 0/1 flags and the levels are percentages; anything else is
            // not this protocol.
            if (r[1] <= 1 && r[6] <= 1 && r[2] <= 100 && r[3] <= 100) return true;
        }
    }
    return false;
}

}  // namespace

void PollRongyuan(PollContext& ctx) {
    static std::set<std::wstring> seenOnline;  // slot keys; only the polling thread touches it
    for (const Family& f : kFamilies) {
        for (auto [vid, pid] : ctx.Products(f.vid)) {
            if (!KnownDongle(vid, pid)) continue;
            for (const HidColl* c : ctx.Find(vid, pid, [](const HidColl& c) { return c.page() == 0xFFFF && c.featLen() == 65; })) {
                HidDev d;
                if (!d.OpenFeatureOnly(*c)) continue;
                std::vector<BYTE> r;
                if (!QueryStatus(d, r)) continue;
                // A dongle that has been idle can report its devices offline on the first request
                // while it wakes up the link; ask again before treating the mouse as switched off.
                for (int again = 0; again < 6 && (r[4] != 0 && r[5] != 0); again++) {
                    Sleep(300);
                    std::vector<BYTE> r2;
                    if (QueryStatus(d, r2)) r = r2;
                }

                // The dongle's product string names the receiver, not the mouse, so keep it generic.
                std::wstring base = HidProduct(*c);
                if (base.empty() || base.find(L"Receiver") != std::wstring::npos || base.find(L"Dongle") != std::wstring::npos)
                    base.clear();

                struct Slot { const wchar_t* suffix; const wchar_t* fallback; DeviceKind kind; int level; bool online; };
                const Slot slots[] = {
                    { L":mouse", L"Wireless mouse", DeviceKind::Mouse, r[3], r[5] == 0 },
                    { L":keyboard", L"Wireless keyboard", DeviceKind::Keyboard, r[2], r[4] == 0 },
                };
                bool first = true;
                for (const Slot& sl : slots) {
                    DeviceStatus s;
                    s.key = L"rongyuan:" + Hex4(vid) + L":" + Hex4(pid) + sl.suffix;
                    const wchar_t* known = first ? KnownMouseName(vid, pid) : nullptr;
                    s.name = known ? known : (first && !base.empty()) ? base : sl.fallback;
                    s.kind = sl.kind;
                    s.via = L"2.4 GHz receiver";
                    s.connected = sl.online;
                    if (sl.online) s.percent = sl.level;
                    first = false;
                    if (sl.online) seenOnline.insert(s.key);
                    if (seenOnline.count(s.key)) ctx.out.push_back(s);
                }
                ctx.Claim(vid, pid);
                break;
            }
        }
    }
}
