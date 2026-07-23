#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <cstdint>

namespace gspl::studio {

struct ThemeColor {
    std::uint8_t r{0}, g{0}, b{0}, a{255};

    static ThemeColor from_hex(std::string_view hex);
    [[nodiscard]] std::string to_hex() const;
    [[nodiscard]] float contrast_ratio(const ThemeColor& other) const;
};

struct ThemeDefinition {
    std::string id;
    std::string name;
    std::string author;
    bool is_dark{false};
    bool is_high_contrast{false};

    ThemeColor window_bg;
    ThemeColor window_text;
    ThemeColor editor_bg;
    ThemeColor editor_text;
    ThemeColor line_number_bg;
    ThemeColor line_number_text;
    ThemeColor selection_bg;
    ThemeColor cursor_color;
    ThemeColor accent_color;
    ThemeColor error_color;
    ThemeColor warning_color;
    ThemeColor info_color;
    ThemeColor gutter_bg;
    ThemeColor gutter_text;
    ThemeColor line_highlight_bg;
    ThemeColor bracket_match_bg;
    ThemeColor bracket_match_outline;

    ThemeColor keyword_color;
    ThemeColor string_color;
    ThemeColor number_color;
    ThemeColor comment_color;
    ThemeColor type_color;
    ThemeColor function_color;
    ThemeColor variable_color;
    ThemeColor property_color;
    ThemeColor operator_color;
    ThemeColor punctuation_color;
    ThemeColor preprocessor_color;
};

class ThemeManager {
public:
    ThemeManager();
    ~ThemeManager();

    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;

    void load_builtin_themes();
    bool load_theme_from_file(const std::string& path);
    bool activate_theme(const std::string& id);
    bool activate_theme(const ThemeDefinition& theme);

    [[nodiscard]] auto active_theme() const -> const ThemeDefinition&;
    [[nodiscard]] auto available_themes() const -> const std::vector<ThemeDefinition>&;
    [[nodiscard]] auto find(const std::string& id) -> ThemeDefinition*;

    void set_os_dark_mode(bool dark);
    [[nodiscard]] bool os_dark_mode() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gspl::studio
