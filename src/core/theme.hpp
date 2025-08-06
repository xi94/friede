// =================================================================================
// core/theme.hpp
// =================================================================================

#pragma once

#include "config.hpp"
#include <QColor>

namespace core {

/// @struct Theme
/// @brief Defines the set of colors used for application theming.
struct Theme {
    QColor background_super_dark;
    QColor background_dark;
    QColor background_light;
    QColor text_primary;
    QColor text_secondary;
    QColor border;
    QColor accent;
    QColor accent_hover;
    QColor button_primary;
    QColor button_hover;
    QColor button_disabled;
    QColor text_disabled;
    QColor success;
    QColor error;
};

/// @class Theme_Config
/// @brief Manages loading and saving theme data from a configuration file.
class Theme_Config final : public Config {
  public:
    /// @brief Constructs the theme configuration manager.
    Theme_Config();

    /// @brief Loads the theme from the configuration file, with defaults.
    auto load() const -> Theme;

    /// @brief Saves the specified theme to the configuration file.
    auto save(const Theme &theme) -> bool;
};

} // namespace core
