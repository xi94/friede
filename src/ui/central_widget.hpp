// =================================================================================
// ui/central_widget.hpp
// =================================================================================

#pragma once

#include <QPainter>
#include <QPainterPath>
#include <QWidget>

#include "core/theme.hpp"

namespace ui {

/// @class Central_Widget
/// @brief The main content widget for the application, featuring a custom-painted background.
///
/// @note This widget overrides the paint event to draw a rounded rectangle,
/// providing the application's primary background and border.
class Central_Widget : public QWidget {
  public:
    /// @brief Constructs the central widget.
    /// @param theme_config A pointer to the application's theme configuration for styling.
    /// @param parent The parent widget.
    explicit Central_Widget(core::Theme_Config *theme_config, QWidget *parent = nullptr);

  protected:
    /// @brief Handles the paint event to draw the custom background.
    /// @param event The paint event.
    auto paintEvent(QPaintEvent *event) -> void override;

  private:
    /// @brief A pointer to the theme configuration used for styling the widget.
    core::Theme_Config *theme_config_;
};

} // namespace ui
