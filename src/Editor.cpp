#include "Plugin.h"
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>
#include "public.sdk/source/common/pluginview.h"
#include <cstdio>
#include <cstring>

namespace atomiccluster {
using namespace Steinberg;
class PedalEditor final : public CPluginView {
    Controller* controller;
    HWND window = nullptr;
    HINSTANCE instance = nullptr;
    int drag = -1, startY = 0;
    double startValue = 0;
    ULONGLONG modeDown = 0, lastTap = 0;
    bool modeHeld = false;
    void tap() {
        if (value(TapEnabled) < .5) return;
        auto now = GetTickCount64();
        if (lastTap && now - lastTap >= 60 && now - lastTap <= 2000) {
            const double seconds = (now - lastTap) / 1000.;
            change(Speed, std::log(seconds / 2.) / std::log(1. / 750.)); change(Sync, 0);
        }
        lastTap = now;
    }
    static constexpr int width = 420, height = 760;
    double value(int id) { return controller->getParamNormalized(id); }
    void change(int id, double v, bool gesture = true) {
        if (gesture) controller->beginEdit(id);
        controller->setParamNormalized(id, clean(v));
        controller->performEdit(id, clean(v));
        if (gesture) controller->endEdit(id);
        InvalidateRect(window, nullptr, FALSE);
    }
    static void fill(HDC dc, int x, int y, int w, int h, COLORREF color, int radius = 0) {
        auto brush = CreateSolidBrush(color);
        auto oldB = SelectObject(dc, brush); auto oldP = SelectObject(dc, GetStockObject(NULL_PEN));
        if (radius) RoundRect(dc, x, y, x + w, y + h, radius, radius); else Rectangle(dc, x, y, x + w, y + h);
        SelectObject(dc, oldP); SelectObject(dc, oldB); DeleteObject(brush);
    }
    static void ellipse(HDC dc, int x, int y, int r, COLORREF color) {
        auto b = CreateSolidBrush(color); auto ob = SelectObject(dc, b); auto op = SelectObject(dc, GetStockObject(NULL_PEN));
        Ellipse(dc, x - r, y - r, x + r, y + r); SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(b);
    }
    static void text(HDC dc, int y, const char* s, int size, COLORREF color = RGB(246, 240, 225), int x = 0, int w = width) {
        auto f = CreateFontA(size, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
        auto old = SelectObject(dc, f); SetTextColor(dc, color); SetBkMode(dc, TRANSPARENT);
        RECT r{x, y, x + w, y + size + 8}; DrawTextA(dc, s, -1, &r, DT_CENTER | DT_SINGLELINE);
        SelectObject(dc, old); DeleteObject(f);
    }
    void knob(HDC dc, int id, int x, int y, const char* label) {
        ellipse(dc, x + 2, y + 5, 55, RGB(9, 8, 20));
        ellipse(dc, x, y, 54, RGB(129, 130, 139));
        ellipse(dc, x, y - 1, 49, RGB(214, 213, 210));
        ellipse(dc, x - 1, y - 3, 44, RGB(250, 248, 240));
        double angle = (-225 + value(id) * 270) * pi / 180;
        auto pen = CreatePen(PS_SOLID, 4, RGB(45, 27, 28)); auto old = SelectObject(dc, pen);
        MoveToEx(dc, x + int(std::cos(angle) * 23), y + int(std::sin(angle) * 23), nullptr);
        LineTo(dc, x + int(std::cos(angle) * 37), y + int(std::sin(angle) * 37));
        SelectObject(dc, old); DeleteObject(pen);
        text(dc, y + 59, label, 20, RGB(250, 246, 235), x - 70, 140);
        Vst::String128 display{}; controller->getParamStringByValue(id, value(id), display);
        char ascii[128]{}; for (int i = 0; i < 127 && display[i]; ++i) ascii[i] = char(display[i]);
        text(dc, y + 84, ascii, 13, RGB(192, 181, 206), x - 75, 150);
    }
    void paint() {
        PAINTSTRUCT ps; HDC target = BeginPaint(window, &ps);
        HDC dc = CreateCompatibleDC(target); HBITMAP bmp = CreateCompatibleBitmap(target, width, height);
        auto old = SelectObject(dc, bmp);
        fill(dc, 0, 0, width, height, RGB(14, 13, 22));
        fill(dc, 22, 10, 376, 732, RGB(76, 67, 100), 38);
        fill(dc, 25, 12, 370, 726, RGB(32, 18, 70), 34);
        // Deterministic metallic flecks, painted at control rate rather than in audio.
        uint32_t random = 71;
        for (int i = 0; i < 10500; ++i) {
            random = random * 1664525u + 1013904223u; int x = 35 + (random % 350);
            random = random * 1664525u + 1013904223u; int y = 24 + (random % 696);
            SetPixelV(dc, x, y, RGB(57 + (random & 31), 37 + (random & 23), 96 + (random & 47)));
        }
        ellipse(dc, 210, 55, 24, RGB(132, 131, 143));
        ellipse(dc, 210, 54, 19, RGB(250, 245, 232));
        text(dc, 82, "MODE", 18);
        knob(dc, Volume, 112, 167, "VOL"); knob(dc, Blend, 308, 167, "BLEND");
        knob(dc, Speed, 112, 361, "SPEED"); knob(dc, Atoms, 308, 361, "ATOMS");
        const bool on = value(Bypass) < .5, smooth = value(Smooth) >= .5;
        ellipse(dc, 210, 272, 22, on ? (smooth ? RGB(27, 95, 55) : RGB(122, 19, 26)) : RGB(30, 24, 37));
        ellipse(dc, 210, 272, 13, on ? (smooth ? RGB(131, 255, 159) : RGB(255, 224, 185)) : RGB(58, 52, 63));
        text(dc, 297, on ? (smooth ? "SMOOTH" : "SHARP") : "BYPASS", 13);
        char spatial[100]{};
        std::snprintf(spatial,sizeof(spatial),"STEREO %.0f%%   |   %s DYNAMICS",value(Stereo)*100,value(Flat)>=.5 ? "FLAT" : "FULL");
        fill(dc,65,464,290,22,RGB(47,32,79),6);
        text(dc,466,spatial,11,RGB(219,208,234));
        // Original Jerzy branding with the pedal's purple/yellow visual vocabulary.
        auto pen = CreatePen(PS_SOLID, 3, RGB(249, 187, 36)); auto op = SelectObject(dc, pen);
        auto ob = SelectObject(dc, GetStockObject(NULL_BRUSH));
        RoundRect(dc, 55, 488, 365, 714, 18, 18);
        Ellipse(dc, 87, 526, 333, 604); Ellipse(dc, 146, 523, 273, 633);
        SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(pen);
        fill(dc, 88, 521, 244, 76, RGB(31, 18, 67), 12);
        text(dc, 520, "ATOMIC", 35); text(dc, 556, "CLUSTER", 32);
        text(dc, 600, "JERZY  /  SPECTRAL DECOMPOSER", 12, RGB(255, 199, 54));
        ellipse(dc, 210, 665, 49, RGB(217, 173, 65));
        ellipse(dc, 210, 665, 43, RGB(80, 80, 83));
        ellipse(dc, 210, 665, 37, RGB(219, 219, 214));
        ellipse(dc, 210, 664, 29, RGB(110, 112, 111));
        ellipse(dc, 210, 662, 24, RGB(169, 171, 167));
        const char* patterns[]{"CLUSTER", "ARP", "RANDOM"};
        for(int i=0;i<3;++i) {
            const bool selected=int(std::round(value(Motion)*2))==i;
            fill(dc,65+i*98,490,94,27,selected ? RGB(210,152,30) : RGB(47,32,79),6);
            text(dc,495,patterns[i],12,selected ? RGB(24,15,45) : RGB(240,228,248),65+i*98,94);
        }
        text(dc, 719, "CLICK: BYPASS   /   RIGHT CLICK: OPTIONS", 11, RGB(215, 205, 228));
        BitBlt(target, 0, 0, width, height, dc, 0, 0, SRCCOPY);
        SelectObject(dc, old); DeleteObject(bmp); DeleteDC(dc); EndPaint(window, &ps);
    }
    void menu(int x, int y) {
        HMENU m = CreatePopupMenu();
        AppendMenuA(m, MF_STRING | (value(Flat) >= .5 ? MF_CHECKED : 0), 1, "Flat dynamics");
        AppendMenuA(m, MF_STRING | (value(Sync) >= .5 ? MF_CHECKED : 0), 2, "Sync to host tempo");
        AppendMenuA(m, MF_STRING | (value(TapEnabled) >= .5 ? MF_CHECKED : 0), 3, "Tap tempo enabled");
        AppendMenuA(m, MF_SEPARATOR, 0, nullptr);
        const char* names[]{"1/16 beat", "1/8 beat", "1/4 beat", "1/2 beat", "1 bar"};
        for (int i = 0; i < 5; ++i) AppendMenuA(m, MF_STRING | (int(std::round(value(Division) * 4)) == i ? MF_CHECKED : 0), 10 + i, names[i]);
        AppendMenuA(m,MF_SEPARATOR,0,nullptr);
        const char* patterns[]{"Pattern: Cluster", "Pattern: Arpeggio", "Pattern: Random harmonic"};
        for(int i=0;i<3;++i) AppendMenuA(m,MF_STRING | (int(std::round(value(Motion)*2))==i ? MF_CHECKED:0),20+i,patterns[i]);
        const char* widths[]{"Stereo: mono", "Stereo: 50%", "Stereo: 100%"};
        for(int i=0;i<3;++i) AppendMenuA(m,MF_STRING | (std::abs(value(Stereo)-i*.5)<.01 ? MF_CHECKED:0),30+i,widths[i]);
        POINT p{x, y}; ClientToScreen(window, &p);
        int result = TrackPopupMenu(m, TPM_RETURNCMD | TPM_RIGHTBUTTON, p.x, p.y, 0, window, nullptr);
        DestroyMenu(m);
        if(result>=20 && result<=22) change(Motion,(result-20)*.5);
        if(result>=30 && result<=32) change(Stereo,(result-30)*.5);
        if (result == 1) change(Flat, value(Flat) < .5 ? 1 : 0);
        if (result == 2) change(Sync, value(Sync) < .5 ? 1 : 0);
        if (result == 3) change(TapEnabled, value(TapEnabled) < .5 ? 1 : 0);
        if (result >= 10 && result <= 14) change(Division, (result - 10) / 4.);
    }
    LRESULT message(UINT msg, WPARAM wp, LPARAM lp) {
        const int x = GET_X_LPARAM(lp), y = GET_Y_LPARAM(lp);
        switch (msg) {
        case WM_PAINT: paint(); return 0;
        case WM_ERASEBKGND: return 1;
        case WM_TIMER:
            if (modeDown && !modeHeld && GetTickCount64() - modeDown >= 850) { modeHeld = true; change(Flat, value(Flat) < .5 ? 1 : 0); }
            InvalidateRect(window, nullptr, FALSE); return 0;
        case WM_LBUTTONDOWN: {
            SetFocus(window);
            if(y>=490 && y<=517 && x>=65 && x<359) { change(Motion,std::clamp((x-65)/98,0,2)*.5); return 0; }
            if(y>=462 && y<486) { change(Stereo,value(Stereo)<.25 ? .5 : value(Stereo)<.75 ? 1 : 0); return 0; }
            const int xs[]{112, 308, 112, 308}, ys[]{167, 167, 361, 361};
            for (int i = 0; i < 4; ++i) if (std::hypot(double(x - xs[i]), double(y - ys[i])) < 55) {
                drag = i; startY = y; startValue = value(i); controller->beginEdit(i); SetCapture(window); return 0;
            }
            if (std::hypot(double(x - 210), double(y - 55)) < 27) { modeDown = GetTickCount64(); modeHeld = false; SetCapture(window); }
            if (std::hypot(double(x - 210), double(y - 665)) < 50) { change(Bypass, value(Bypass) < .5 ? 1 : 0); tap(); }
            return 0;
        }
        case WM_MOUSEMOVE:
            if (drag >= 0) {
                double scale = (wp & MK_SHIFT) ? .0005 : .005;
                change(drag, startValue + (startY - y) * scale, false);
                if (drag == Speed) change(Sync, 0);
            }
            return 0;
        case WM_LBUTTONUP:
            if (drag >= 0) { controller->endEdit(drag); drag = -1; }
            if (modeDown) { if (!modeHeld) change(Smooth, value(Smooth) < .5 ? 1 : 0); modeDown = 0; }
            ReleaseCapture(); return 0;
        case WM_CAPTURECHANGED:
            if (drag >= 0) { controller->endEdit(drag); drag = -1; }
            modeDown = 0; return 0;
        case WM_RBUTTONUP: menu(x, y); return 0;
        case WM_MBUTTONUP: tap(); return 0;
        }
        return DefWindowProcA(window, msg, wp, lp);
    }
    static LRESULT CALLBACK procedure(HWND h, UINT m, WPARAM w, LPARAM l) {
        auto* self = reinterpret_cast<PedalEditor*>(GetWindowLongPtrA(h, GWLP_USERDATA));
        if (m == WM_NCCREATE) {
            self = static_cast<PedalEditor*>(reinterpret_cast<CREATESTRUCTA*>(l)->lpCreateParams);
            self->window = h; SetWindowLongPtrA(h, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        return self ? self->message(m, w, l) : DefWindowProcA(h, m, w, l);
    }
public:
    explicit PedalEditor(Controller* c) : controller(c) { controller->addRef(); ViewRect r(0, 0, width, height); setRect(r); }
    ~PedalEditor() override { removed(); controller->release(); }
    tresult PLUGIN_API isPlatformTypeSupported(FIDString type) override { return type && std::strcmp(type, kPlatformTypeHWND) == 0 ? kResultTrue : kResultFalse; }
    tresult PLUGIN_API attached(void* parent, FIDString type) override {
        if (!parent || isPlatformTypeSupported(type) != kResultTrue || window) return kResultFalse;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCSTR>(&procedure), &instance);
        WNDCLASSA wc{}; wc.lpfnWndProc = procedure; wc.hInstance = instance; wc.lpszClassName = "JerzyAtomicClusterEditor010"; wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        if (!RegisterClassA(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return kResultFalse;
        window = CreateWindowExA(0, wc.lpszClassName, "Jerzy Atomic Cluster", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
            0, 0, width, height, static_cast<HWND>(parent), nullptr, instance, this);
        if (!window) return kResultFalse;
        SetTimer(window, 1, 100, nullptr); systemWindow = parent; return kResultOk;
    }
    tresult PLUGIN_API removed() override {
        if (window) {
            KillTimer(window, 1); DestroyWindow(window); window = nullptr;
            // Fails harmlessly while another instance still owns a window.
            UnregisterClassA("JerzyAtomicClusterEditor010", instance);
        }
        systemWindow = nullptr; return kResultOk;
    }
};
IPlugView* createEditor(Controller* c) { return new PedalEditor(c); }
}
#else
namespace atomiccluster { Steinberg::IPlugView* createEditor(Controller*) { return nullptr; } }
#endif
