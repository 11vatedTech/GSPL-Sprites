#pragma once

namespace gspl::studio {

enum class ColorScheme {
    Light,
    Dark,
    Unknown,
};

// Detect the OS color scheme preference
ColorScheme detect_color_scheme();

} // namespace gspl::studio
