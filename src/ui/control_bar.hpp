// =================================================================================
// ui/control_bar.hpp
// =================================================================================

#pragma once

#include "riot/client.hpp"
#include <QLabel>
#include <QPushButton>
#include <QWidget>

namespace ui {

/// @class Control_Bar
/// @brief A horizontal bottom bar for account management and login controls.
///
/// This widget displays controls relevant to the selected game, such as logging
/// in, and adding or removing accounts from the configuration.
class Control_Bar final : public QWidget {
    Q_OBJECT

  public:
    /// @brief Constructs the control bar.
    /// @param parent The parent widget.
    explicit Control_Bar(QWidget *parent = nullptr);
    ~Control_Bar() override = default;

    /// @brief Enables or disables controls based on account selection.
    /// @param enabled True to enable controls, false to disable.
    auto set_controls_enabled(bool enabled) -> void;

    /// @brief Updates the bar's UI to reflect the current game context.
    /// @param game The currently selected game.
    /// @param icon_path The file path to the icon for the specified game.
    auto update_game_context(riot::Game game, const QString &icon_path) -> void;

  signals:
    /// @brief Emitted when the user clicks the login button.
    auto login_clicked() -> void;

    /// @brief Emitted when the user clicks the add account button.
    auto add_account_clicked() -> void;

    /// @brief Emitted when the user clicks the remove account button.
    auto remove_account_clicked() -> void;

  private:
    /// @brief Sets up the widgets, layout, and connections for the bar.
    auto setup_ui() -> void;

  private:
    /// @brief Displays a small icon of the icon of the currently selected game.
    QLabel *game_icon_label_;

    QPushButton *login_button_;
    QPushButton *add_account_button_;
    QPushButton *remove_account_button_;
};

} // namespace ui
