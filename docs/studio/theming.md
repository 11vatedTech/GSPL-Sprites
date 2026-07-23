# GSPL Authoring Studio Theming Guide

## Theme File Format

Themes are JSONC files with a flat structure of color properties. Each property value is a hex RGB or ARGB string (`#RRGGBB` or `#AARRGGBB`).

```jsonc
{
  "name": "My Custom Theme",
  "author": "Your Name",
  "version": "1.0.0",
  "type": "dark",                // "light", "dark", or "high-contrast"
  "colors": {
    "window_bg": "#1e1e1e",
    "window_fg": "#d4d4d4",
    "editor_bg": "#1e1e1e",
    "editor_line_highlight": "#2a2a2a",
    "keyword_color": "#569cd6",
    "string_color": "#ce9178",
    "comment_color": "#6a9955",
    "number_color": "#b5cea8",
    "type_color": "#4ec9b0",
    "function_color": "#dcdcaa",
    "variable_color": "#9cdcfe",
    "constant_color": "#4fc1ff",
    "operator_color": "#d4d4d4",
    "diagnostic_error": "#f14c4c",
    "diagnostic_warning": "#cca700",
    "diagnostic_info": "#3794ff",
    "selection_bg": "#264f78",
    "gutter_bg": "#1e1e1e",
    "gutter_fg": "#858585",
    "line_number_fg": "#858585",
    "scrollbar_bg": "#1e1e1e",
    "scrollbar_fg": "#424242",
    "panel_bg": "#252526",
    "panel_border": "#3c3c3c",
    "status_bar_bg": "#007acc",
    "status_bar_fg": "#ffffff",
    "tab_active_bg": "#1e1e1e",
    "tab_inactive_bg": "#2d2d2d",
    "button_bg": "#0e639c",
    "button_fg": "#ffffff",
    "input_bg": "#3c3c3c",
    "input_fg": "#cccccc",
    "input_border": "#3c3c3c",
    "list_hover_bg": "#2a2d2e",
    "list_active_bg": "#37373d",
    "tooltip_bg": "#252526",
    "tooltip_border": "#3c3c3c"
  }
}
```

## Color Properties Reference

### Editor Colors

| Property | Description |
|----------|-------------|
| `editor_bg` | Editor background |
| `editor_line_highlight` | Current line highlight |
| `keyword_color` | Language keywords |
| `string_color` | String literals |
| `comment_color` | Comments |
| `number_color` | Numeric literals |
| `type_color` | Type names |
| `function_color` | Function and method names |
| `variable_color` | Variables and parameters |
| `constant_color` | Constants |
| `operator_color` | Operators and punctuation |
| `selection_bg` | Text selection background |
| `diagnostic_error` | Error underline/wave color |
| `diagnostic_warning` | Warning underline/wave color |
| `diagnostic_info` | Info underline/wave color |

### Window / Panel Colors

| Property | Description |
|----------|-------------|
| `window_bg` | Main window background |
| `window_fg` | Default text color |
| `panel_bg` | Side panel background |
| `panel_border` | Panel border color |
| `gutter_bg` | Editor gutter background |
| `gutter_fg` | Gutter foreground (breakpoints, folds) |
| `line_number_fg` | Line number text |
| `scrollbar_bg` | Scrollbar track |
| `scrollbar_fg` | Scrollbar thumb |

### UI Element Colors

| Property | Description |
|----------|-------------|
| `status_bar_bg` | Status bar background |
| `status_bar_fg` | Status bar text |
| `tab_active_bg` | Active editor tab background |
| `tab_inactive_bg` | Inactive editor tab background |
| `button_bg` | Button fill |
| `button_fg` | Button text |
| `input_bg` | Input field background |
| `input_fg` | Input field text |
| `input_border` | Input field border |
| `list_hover_bg` | List item hover background |
| `list_active_bg` | Selected list item background |
| `tooltip_bg` | Tooltip background |
| `tooltip_border` | Tooltip border |

## WCAG Contrast Requirements

The Theme Manager validates that foreground/background pairs meet WCAG AA contrast ratios:

- **Normal text** (< 18pt): minimum 4.5:1
- **Large text** (>= 18pt or >= 14pt bold): minimum 3:1
- **UI components**: minimum 3:1

The following pairs are checked:

- `window_fg` on `window_bg`
- `editor_bg` with `keyword_color`, `string_color`, `comment_color`, etc.
- `status_bar_fg` on `status_bar_bg`
- `button_fg` on `button_bg`
- `input_fg` on `input_bg`

Contrast ratio warnings appear in the Problems panel when a theme is loaded.

## Creating a Custom Theme

1. Create a `.jsonc` file with the structure shown above
2. Set `type` to `"light"`, `"dark"`, or `"high-contrast"`
3. Define at minimum `window_bg`, `window_fg`, `editor_bg`, `keyword_color`, `string_color`, `comment_color`, `diagnostic_error`, `diagnostic_warning`
4. All unspecified colors inherit from the built-in default for the given `type`
5. Validate contrast ratios using the Studio's built-in check (View > Themes > Validate)

## Installing Custom Themes

Place the theme file in one of these locations:

| Location | Scope |
|----------|-------|
| `workspace/themes/my-theme.jsonc` | Workspace-specific |
| `%APPDATA%/GSPL/Studio/themes/my-theme.jsonc` | User-wide |
| `<studio-install>/themes/` | System-wide (requires admin) |

The theme appears in **View > Themes** after the app is restarted (or select **Reload Themes** from the menu).

## Example Minimal Theme

```jsonc
{
  "name": "Minimal Dark",
  "type": "dark",
  "colors": {
    "window_bg": "#1e1e1e",
    "window_fg": "#d4d4d4",
    "editor_bg": "#1e1e1e",
    "keyword_color": "#569cd6",
    "string_color": "#ce9178",
    "comment_color": "#6a9955",
    "diagnostic_error": "#f14c4c",
    "diagnostic_warning": "#cca700",
    "selection_bg": "#264f78",
    "status_bar_bg": "#007acc",
    "status_bar_fg": "#ffffff"
  }
}
```
