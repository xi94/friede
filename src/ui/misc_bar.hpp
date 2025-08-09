// =================================================================================
// ui/misc_bar.hpp
// =================================================================================

#pragma once

#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

namespace ui {

/// @class Misc_Bar
/// @brief A vertical side bar containing a menu for miscellaneous actions.
///
/// This widget provides access to application-wide functionalities like theme
/// customization, update checks, and opening the configuration directory.
class Misc_Bar final : public QWidget {
    Q_OBJECT

  public:
    /// @brief Constructs the miscellaneous actions bar.
    /// @param parent The parent widget.
    explicit Misc_Bar(QWidget *parent = nullptr);
    ~Misc_Bar() override = default;

  signals:
    /// @brief Emitted when the user requests to customize the theme.
    auto customize_theme_requested() -> void;

    /// @brief Emitted when the user requests to check for application updates.
    auto check_for_updates_requested() -> void;

    /// @brief Emitted when the user requests to open the config directory.
    auto open_config_directory_requested() -> void;

  private:
    /// @brief Sets up the widgets, layout, and connections for the bar.
    auto setup_ui() -> void;

    /// @brief The button that reveals the options menu.
    QPushButton *options_button_;

    /// @brief The menu containing miscellaneous actions.
    QMenu *options_menu_;
};

} // namespace ui
