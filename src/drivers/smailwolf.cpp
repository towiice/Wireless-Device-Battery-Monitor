// SMAILWOLF RS7 mouse over USB cable (VID A8A4) or its 2.4 GHz receiver (VID A8A5), PID 2255.
// The mouse has no battery information over Bluetooth, so this only covers cable and receiver.
// Vendor collection: usage page FF01, usage 0x10, USB interface 2, 65-byte input and output reports
// (report id 0). Commands start with 55; replies start with AA and echo the command byte:
//   battery: [0][55][30][A5][0B][2E][01][01]  -> [AA][30]...[8] = percent, [9] = charging (1)
//   online:  [0][55][ED][00][01][2E]          -> [AA][ED]...[8] = 2 when the mouse reached the receiver
// Protocol read from the vendor's web configurator (smailwolf.com/qdxz/wyqd, RS7 entry) and
// confirmed on a real RS7 over USB cable. The receiver path follows the same web configurator and
// has not been tested.
#include "drivers.h"

namespace {

// Sends `cmd` (bytes after the report id) and waits for the reply that echoes `echo`.
bool Query(HidDev& d, std::initializer_list<BYTE> cmd, BYTE echo, std::vector<BYTE>& r) {
    std::vector<BYTE> req(65, 0);
    size_t i = 1;
    for (BYTE b : cmd) req[i++] = b;
    d.Flush();
    if (!d.Write(req)) return false;
    ULONGLONG end = GetTickCount64() + 600;
    for (;;) {
        int left = (int)((LONGLONG)end - (LONGLONG)GetTickCount64());
        if (left <= 0 || d.Read(r, left) < 10) return false;
        if (r[0] == 0xAA && r[1] == echo) return true;  // other 0xAA reports are unsolicited notifications
    }
}

}  // namespace

void PollSmailwolf(PollContext& ctx) {
    for (USHORT vendor : { (USHORT)0xA8A4, (USHORT)0xA8A5 }) {
        const bool cable = vendor == 0xA8A4;
        for (auto [vid, pid] : ctx.Products(vendor)) {
            if (pid != 0x2255) continue;
            for (const HidColl* c : ctx.Find(vid, pid, [](const HidColl& c) {
                     return c.page() == 0xFF01 && c.outLen() == 65 && c.inLen() == 65;
                 })) {
                HidDev d;
                if (!d.Open(*c)) continue;
                std::vector<BYTE> r;
                DeviceStatus s;
                s.key = L"smailwolf:" + Hex4(vid) + L":" + Hex4(pid);
                s.name = L"Smailwolf RS7";
                s.kind = DeviceKind::Mouse;
                s.via = cable ? L"USB cable" : L"2.4 GHz receiver";
                if (!cable) {
                    // The receiver is always present; ask whether the mouse is actually linked to it.
                    if (!Query(d, { 0x55, 0xED, 0x00, 0x01, 0x2E }, 0xED, r)) continue;
                    if (r[8] != 2) {
                        s.connected = false;
                        ctx.Add(s, vid, pid);
                        break;
                    }
                }
                if (!Query(d, { 0x55, 0x30, 0xA5, 0x0B, 0x2E, 0x01, 0x01 }, 0x30, r) || r[8] > 100) continue;
                s.percent = r[8];
                s.charging = r[9] != 0;
                ctx.Add(s, vid, pid);
                break;
            }
        }
    }
}
