// =================================================================================
// core/theme.hpp
// =================================================================================

#pragma once

#include "config.hpp"
#include <QColor>

namespace core {

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

class Theme_Config final : public Config {
  public:
    Theme_Config();

    auto load() const -> Theme;
    auto save(const Theme &theme) -> bool;
};

} // namespace core
