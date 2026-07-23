#include "gspl/studio/theme.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>

namespace gspl::studio {

namespace {

auto hex_val(char c) -> std::uint8_t {
    if (c >= '0' && c <= '9') return static_cast<std::uint8_t>(c - '0');
    if (c >= 'a' && c <= 'f') return static_cast<std::uint8_t>(c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return static_cast<std::uint8_t>(c - 'A' + 10);
    return 0;
}

[[nodiscard]] auto srgb_to_linear(std::uint8_t v) -> float {
    auto s = static_cast<float>(v) / 255.0f;
    if (s <= 0.04045f) return s / 12.92f;
    return std::pow((s + 0.055f) / 1.055f, 2.4f);
}

[[nodiscard]] auto relative_luminance(const ThemeColor& c) -> float {
    auto r = srgb_to_linear(c.r);
    auto g = srgb_to_linear(c.g);
    auto b = srgb_to_linear(c.b);
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

[[nodiscard]] auto make_light_theme() -> ThemeDefinition {
    ThemeDefinition t;
    t.id = "gspl-light";
    t.name = "Light";
    t.author = "GSPL Studio";
    t.is_dark = false;

    t.window_bg            = ThemeColor::from_hex("#FFFFFF");
    t.window_text          = ThemeColor::from_hex("#1E1E1E");
    t.editor_bg            = ThemeColor::from_hex("#FAFAFA");
    t.editor_text          = ThemeColor::from_hex("#1E1E1E");
    t.line_number_bg       = ThemeColor::from_hex("#F0F0F0");
    t.line_number_text     = ThemeColor::from_hex("#6E6E6E");
    t.selection_bg         = ThemeColor::from_hex("#ADD6FF");
    t.cursor_color         = ThemeColor::from_hex("#1E1E1E");
    t.accent_color         = ThemeColor::from_hex("#007ACC");
    t.error_color          = ThemeColor::from_hex("#E51400");
    t.warning_color        = ThemeColor::from_hex("#BF8803");
    t.info_color           = ThemeColor::from_hex("#007ACC");
    t.gutter_bg            = ThemeColor::from_hex("#F0F0F0");
    t.gutter_text          = ThemeColor::from_hex("#6E6E6E");
    t.line_highlight_bg    = ThemeColor::from_hex("#F5F5F5");
    t.bracket_match_bg     = ThemeColor::from_hex("#D4EDDA");
    t.bracket_match_outline= ThemeColor::from_hex("#28A745");

    t.keyword_color        = ThemeColor::from_hex("#0000FF");
    t.string_color         = ThemeColor::from_hex("#A31515");
    t.number_color         = ThemeColor::from_hex("#098658");
    t.comment_color        = ThemeColor::from_hex("#008000");
    t.type_color           = ThemeColor::from_hex("#267F99");
    t.function_color       = ThemeColor::from_hex("#795E26");
    t.variable_color       = ThemeColor::from_hex("#001080");
    t.property_color       = ThemeColor::from_hex("#800000");
    t.operator_color       = ThemeColor::from_hex("#000000");
    t.punctuation_color    = ThemeColor::from_hex("#6E6E6E");
    t.preprocessor_color   = ThemeColor::from_hex("#800000");

    return t;
}

[[nodiscard]] auto make_dark_theme() -> ThemeDefinition {
    ThemeDefinition t;
    t.id = "gspl-dark";
    t.name = "Dark";
    t.author = "GSPL Studio";
    t.is_dark = true;

    t.window_bg            = ThemeColor::from_hex("#1E1E1E");
    t.window_text          = ThemeColor::from_hex("#CCCCCC");
    t.editor_bg            = ThemeColor::from_hex("#1E1E1E");
    t.editor_text          = ThemeColor::from_hex("#D4D4D4");
    t.line_number_bg       = ThemeColor::from_hex("#252526");
    t.line_number_text     = ThemeColor::from_hex("#858585");
    t.selection_bg         = ThemeColor::from_hex("#264F78");
    t.cursor_color         = ThemeColor::from_hex("#AEAFAD");
    t.accent_color         = ThemeColor::from_hex("#569CD6");
    t.error_color          = ThemeColor::from_hex("#F44747");
    t.warning_color        = ThemeColor::from_hex("#CCA700");
    t.info_color           = ThemeColor::from_hex("#75BEFF");
    t.gutter_bg            = ThemeColor::from_hex("#252526");
    t.gutter_text          = ThemeColor::from_hex("#858585");
    t.line_highlight_bg    = ThemeColor::from_hex("#2A2D2E");
    t.bracket_match_bg     = ThemeColor::from_hex("#343B41");
    t.bracket_match_outline= ThemeColor::from_hex("#569CD6");

    t.keyword_color        = ThemeColor::from_hex("#569CD6");
    t.string_color         = ThemeColor::from_hex("#CE9178");
    t.number_color         = ThemeColor::from_hex("#B5CEA8");
    t.comment_color        = ThemeColor::from_hex("#6A9955");
    t.type_color           = ThemeColor::from_hex("#4EC9B0");
    t.function_color       = ThemeColor::from_hex("#DCDCAA");
    t.variable_color       = ThemeColor::from_hex("#9CDCFE");
    t.property_color       = ThemeColor::from_hex("#CE9178");
    t.operator_color       = ThemeColor::from_hex("#D4D4D4");
    t.punctuation_color    = ThemeColor::from_hex("#858585");
    t.preprocessor_color   = ThemeColor::from_hex("#C586C0");

    return t;
}

[[nodiscard]] auto make_high_contrast_theme() -> ThemeDefinition {
    ThemeDefinition t;
    t.id = "gspl-high-contrast";
    t.name = "High Contrast";
    t.author = "GSPL Studio";
    t.is_dark = true;
    t.is_high_contrast = true;

    t.window_bg            = ThemeColor::from_hex("#000000");
    t.window_text          = ThemeColor::from_hex("#FFFFFF");
    t.editor_bg            = ThemeColor::from_hex("#000000");
    t.editor_text          = ThemeColor::from_hex("#FFFFFF");
    t.line_number_bg       = ThemeColor::from_hex("#0A0A0A");
    t.line_number_text     = ThemeColor::from_hex("#FFFFFF");
    t.selection_bg         = ThemeColor::from_hex("#FF0000");
    t.cursor_color         = ThemeColor::from_hex("#FFFFFF");
    t.accent_color         = ThemeColor::from_hex("#FFFF00");
    t.error_color          = ThemeColor::from_hex("#FF0000");
    t.warning_color        = ThemeColor::from_hex("#FFFF00");
    t.info_color           = ThemeColor::from_hex("#00FFFF");
    t.gutter_bg            = ThemeColor::from_hex("#0A0A0A");
    t.gutter_text          = ThemeColor::from_hex("#FFFFFF");
    t.line_highlight_bg    = ThemeColor::from_hex("#1A1A1A");
    t.bracket_match_bg     = ThemeColor::from_hex("#FFFF00");
    t.bracket_match_outline= ThemeColor::from_hex("#FFFFFF");

    t.keyword_color        = ThemeColor::from_hex("#FFFFFF");
    t.string_color         = ThemeColor::from_hex("#FF9DA4");
    t.number_color         = ThemeColor::from_hex("#B5CEA8");
    t.comment_color        = ThemeColor::from_hex("#7C7C7C");
    t.type_color           = ThemeColor::from_hex("#4EC9B0");
    t.function_color       = ThemeColor::from_hex("#DCDCAA");
    t.variable_color       = ThemeColor::from_hex("#9CDCFE");
    t.property_color       = ThemeColor::from_hex("#CE9178");
    t.operator_color       = ThemeColor::from_hex("#FFFFFF");
    t.punctuation_color    = ThemeColor::from_hex("#FFFFFF");
    t.preprocessor_color   = ThemeColor::from_hex("#C586C0");

    return t;
}

[[nodiscard]] auto read_file(const std::string& path) -> std::string {
    std::ifstream file(path);
    if (!file) return {};
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

[[nodiscard]] auto strip_jsonc_comments(std::string_view jsonc) -> std::string {
    std::string result;
    result.reserve(jsonc.size());
    bool in_string = false;
    for (size_t i = 0; i < jsonc.size(); ++i) {
        char c = jsonc[i];
        if (in_string) {
            result += c;
            if (c == '"' && (i == 0 || jsonc[i - 1] != '\\')) {
                in_string = false;
            }
            continue;
        }
        if (c == '"') {
            result += c;
            in_string = true;
            continue;
        }
        if (c == '/' && i + 1 < jsonc.size()) {
            if (jsonc[i + 1] == '/') {
                while (i < jsonc.size() && jsonc[i] != '\n') ++i;
                continue;
            }
            if (jsonc[i + 1] == '*') {
                i += 2;
                while (i + 1 < jsonc.size() && !(jsonc[i] == '*' && jsonc[i + 1] == '/')) ++i;
                if (i < jsonc.size()) ++i;
                continue;
            }
        }
        result += c;
    }
    return result;
}

[[nodiscard]] auto extract_string(std::string_view json, std::string_view key) -> std::string {
    auto pos = json.find(key);
    if (pos == std::string_view::npos) return {};
    auto colon = json.find(':', pos + key.size());
    if (colon == std::string_view::npos) return {};
    auto start = json.find_first_of('"', colon);
    if (start == std::string_view::npos) return {};
    auto end = json.find('"', start + 1);
    if (end == std::string_view::npos) return {};
    return std::string(json.substr(start + 1, end - start - 1));
}

[[nodiscard]] auto extract_hex_color(std::string_view json, std::string_view key) -> ThemeColor {
    auto raw = extract_string(json, key);
    if (raw.empty()) return {};
    return ThemeColor::from_hex(raw);
}

} // anonymous namespace

auto ThemeColor::from_hex(std::string_view hex) -> ThemeColor {
    ThemeColor c;
    auto s = hex;
    if (s.empty()) return c;
    if (s.front() == '#') s.remove_prefix(1);
    auto len = s.size();
    if (len == 3) {
        c.r = static_cast<std::uint8_t>(hex_val(s[0]) * 17);
        c.g = static_cast<std::uint8_t>(hex_val(s[1]) * 17);
        c.b = static_cast<std::uint8_t>(hex_val(s[2]) * 17);
        c.a = 255;
    } else if (len == 4) {
        c.r = static_cast<std::uint8_t>(hex_val(s[0]) * 17);
        c.g = static_cast<std::uint8_t>(hex_val(s[1]) * 17);
        c.b = static_cast<std::uint8_t>(hex_val(s[2]) * 17);
        c.a = static_cast<std::uint8_t>(hex_val(s[3]) * 17);
    } else if (len >= 6) {
        c.r = static_cast<std::uint8_t>((hex_val(s[0]) << 4) | hex_val(s[1]));
        c.g = static_cast<std::uint8_t>((hex_val(s[2]) << 4) | hex_val(s[3]));
        c.b = static_cast<std::uint8_t>((hex_val(s[4]) << 4) | hex_val(s[5]));
        c.a = (len >= 8)
            ? static_cast<std::uint8_t>((hex_val(s[6]) << 4) | hex_val(s[7]))
            : 255;
    }
    return c;
}

auto ThemeColor::to_hex() const -> std::string {
    constexpr auto hex_digits = "0123456789ABCDEF";
    std::string result = "#";
    result += hex_digits[(r >> 4) & 0xF];
    result += hex_digits[r & 0xF];
    result += hex_digits[(g >> 4) & 0xF];
    result += hex_digits[g & 0xF];
    result += hex_digits[(b >> 4) & 0xF];
    result += hex_digits[b & 0xF];
    if (a != 255) {
        result += hex_digits[(a >> 4) & 0xF];
        result += hex_digits[a & 0xF];
    }
    return result;
}

auto ThemeColor::contrast_ratio(const ThemeColor& other) const -> float {
    auto l1 = relative_luminance(*this);
    auto l2 = relative_luminance(other);
    if (l1 < l2) std::swap(l1, l2);
    return (l1 + 0.05f) / (l2 + 0.05f);
}

struct ThemeManager::Impl {
    ThemeDefinition active;
    std::vector<ThemeDefinition> themes;
    bool os_dark{false};

    bool has_active{false};
};

ThemeManager::ThemeManager()
    : impl_(std::make_unique<Impl>())
{
}

ThemeManager::~ThemeManager() = default;

void ThemeManager::load_builtin_themes() {
    impl_->themes.clear();
    impl_->themes.push_back(make_light_theme());
    impl_->themes.push_back(make_dark_theme());
    impl_->themes.push_back(make_high_contrast_theme());

    if (!impl_->has_active) {
        auto preferred = impl_->os_dark ? "gspl-dark" : "gspl-light";
        activate_theme(preferred);
    }
}

bool ThemeManager::load_theme_from_file(const std::string& path) {
    auto raw = read_file(path);
    if (raw.empty()) return false;

    auto json = strip_jsonc_comments(raw);
    if (json.empty()) return false;

    ThemeDefinition t;
    t.id = extract_string(json, "\"id\"");
    t.name = extract_string(json, "\"name\"");
    t.author = extract_string(json, "\"author\"");
    if (t.id.empty()) return false;

    auto dark_val = extract_string(json, "\"is_dark\"");
    t.is_dark = (dark_val == "true");
    auto hc_val = extract_string(json, "\"is_high_contrast\"");
    t.is_high_contrast = (hc_val == "true");

    t.window_bg             = extract_hex_color(json, "\"window_bg\"");
    t.window_text           = extract_hex_color(json, "\"window_text\"");
    t.editor_bg             = extract_hex_color(json, "\"editor_bg\"");
    t.editor_text           = extract_hex_color(json, "\"editor_text\"");
    t.line_number_bg        = extract_hex_color(json, "\"line_number_bg\"");
    t.line_number_text      = extract_hex_color(json, "\"line_number_text\"");
    t.selection_bg          = extract_hex_color(json, "\"selection_bg\"");
    t.cursor_color          = extract_hex_color(json, "\"cursor_color\"");
    t.accent_color          = extract_hex_color(json, "\"accent_color\"");
    t.error_color           = extract_hex_color(json, "\"error_color\"");
    t.warning_color         = extract_hex_color(json, "\"warning_color\"");
    t.info_color            = extract_hex_color(json, "\"info_color\"");
    t.gutter_bg             = extract_hex_color(json, "\"gutter_bg\"");
    t.gutter_text           = extract_hex_color(json, "\"gutter_text\"");
    t.line_highlight_bg     = extract_hex_color(json, "\"line_highlight_bg\"");
    t.bracket_match_bg      = extract_hex_color(json, "\"bracket_match_bg\"");
    t.bracket_match_outline = extract_hex_color(json, "\"bracket_match_outline\"");

    t.keyword_color         = extract_hex_color(json, "\"keyword_color\"");
    t.string_color          = extract_hex_color(json, "\"string_color\"");
    t.number_color          = extract_hex_color(json, "\"number_color\"");
    t.comment_color         = extract_hex_color(json, "\"comment_color\"");
    t.type_color            = extract_hex_color(json, "\"type_color\"");
    t.function_color        = extract_hex_color(json, "\"function_color\"");
    t.variable_color        = extract_hex_color(json, "\"variable_color\"");
    t.property_color        = extract_hex_color(json, "\"property_color\"");
    t.operator_color        = extract_hex_color(json, "\"operator_color\"");
    t.punctuation_color     = extract_hex_color(json, "\"punctuation_color\"");
    t.preprocessor_color    = extract_hex_color(json, "\"preprocessor_color\"");

    // Replace existing theme with same id, or append
    auto existing = std::find_if(impl_->themes.begin(), impl_->themes.end(),
        [&](const auto& th) { return th.id == t.id; });
    if (existing != impl_->themes.end()) {
        *existing = std::move(t);
    } else {
        impl_->themes.push_back(std::move(t));
    }
    return true;
}

bool ThemeManager::activate_theme(const std::string& id) {
    auto* t = find(id);
    if (!t) return false;
    return activate_theme(*t);
}

bool ThemeManager::activate_theme(const ThemeDefinition& theme) {
    impl_->active = theme;
    impl_->has_active = true;
    return true;
}

auto ThemeManager::active_theme() const -> const ThemeDefinition& {
    return impl_->active;
}

auto ThemeManager::available_themes() const -> const std::vector<ThemeDefinition>& {
    return impl_->themes;
}

auto ThemeManager::find(const std::string& id) -> ThemeDefinition* {
    for (auto& t : impl_->themes) {
        if (t.id == id) return &t;
    }
    return nullptr;
}

void ThemeManager::set_os_dark_mode(bool dark) {
    impl_->os_dark = dark;
}

bool ThemeManager::os_dark_mode() const {
    return impl_->os_dark;
}

} // namespace gspl::studio
