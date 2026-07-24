#include "gspl/studio/dark_mode.hpp"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace gspl::studio {

ColorScheme detect_color_scheme() {
#if defined(_WIN32)
    // Windows: query registry for theme preference
    HKEY hKey;
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS) return ColorScheme::Unknown;

    DWORD value = 0;
    DWORD size = sizeof(value);
    result = RegQueryValueExW(hKey, L"AppsUseLightTheme", nullptr, nullptr,
        reinterpret_cast<LPBYTE>(&value), &size);
    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS) return ColorScheme::Unknown;
    return (value == 0) ? ColorScheme::Dark : ColorScheme::Light;
#elif defined(__linux__)
    // Linux: try gsettings (GNOME) or check xdg
    FILE* pipe = popen("gsettings get org.gnome.desktop.interface color-scheme 2>/dev/null || echo unknown", "r");
    if (!pipe) return ColorScheme::Unknown;

    char buf[64] = {};
    if (fgets(buf, sizeof(buf), pipe) != nullptr) {
        pclose(pipe);
        std::string s(buf);
        if (s.find("dark") != std::string_view::npos || s.find("prefer-dark") != std::string_view::npos)
            return ColorScheme::Dark;
        if (s.find("light") != std::string_view::npos || s.find("default") != std::string_view::npos)
            return ColorScheme::Light;
        return ColorScheme::Unknown;
    }
    pclose(pipe);
    return ColorScheme::Unknown;
#else
    return ColorScheme::Unknown;
#endif
}

} // namespace gspl::studio
