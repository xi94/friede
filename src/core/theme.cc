// =================================================================================
// core/theme.cc
// =================================================================================

#include "theme.hpp"

namespace core {

Theme_Config::Theme_Config()
    : Config{"theme.toml"}
{
}

auto Theme_Config::load() const -> Theme
{
    const auto config = Config::load();
    Theme theme;

    auto get_color_or_default = [&](const char *key, const char *default_hex) {
        auto value = config[key].value<std::string>();
        return value ? QColor(QString::fromStdString(*value)) : QColor(default_hex);
    };

    theme.background_super_dark = get_color_or_default("background_super_dark", "#151515");
    theme.background_dark = get_color_or_default("background_dark", "#191919");
    theme.background_light = get_color_or_default("background_light", "#202020");
    theme.text_primary = get_color_or_default("text_primary", "#FFFFFF");
    theme.text_secondary = get_color_or_default("text_secondary", "#B0B0B0");
    theme.border = get_color_or_default("border", "#333333");
    theme.accent = get_color_or_default("accent", "#3a3a3a");
    theme.accent_hover = get_color_or_default("accent_hover", "#4a4a4a");
    theme.button_primary = get_color_or_default("button_primary", "#3a3a3a");
    theme.button_hover = get_color_or_default("button_hover", "#4a4a4a");
    theme.button_disabled = get_color_or_default("button_disabled", "#2a2a2a");
    theme.text_disabled = get_color_or_default("text_disabled", "#8a8a8a");
    theme.success = get_color_or_default("success", "#2ecc71");
    theme.error = get_color_or_default("error", "#e74c3c");

    return theme;
}

auto Theme_Config::save(const Theme &theme) -> bool
{
    toml::table config;
    auto set_color = [&](const char *key, const QColor &color) { config.insert_or_assign(key, color.name(QColor::HexRgb).toStdString()); };

    set_color("background_super_dark", theme.background_super_dark);
    set_color("background_dark", theme.background_dark);
    set_color("background_light", theme.background_light);
    set_color("text_primary", theme.text_primary);
    set_color("text_secondary", theme.text_secondary);
    set_color("border", theme.border);
    set_color("accent", theme.accent);
    set_color("accent_hover", theme.accent_hover);
    set_color("button_primary", theme.button_primary);
    set_color("button_hover", theme.button_hover);
    set_color("button_disabled", theme.button_disabled);
    set_color("text_disabled", theme.text_disabled);
    set_color("success", theme.success);
    set_color("error", theme.error);

    return Config::save(config);
}

} // namespace core
