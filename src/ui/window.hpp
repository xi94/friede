// =================================================================================
// ui/window.hpp
// =================================================================================

#pragma once

#include "core/account.hpp"
#include "core/theme.hpp"
#include "riot/client.hpp"
#include "theme_editor.hpp"
#include "ui/login_worker.hpp"
#include "updater.hpp"

#include <QApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QMainWindow>
#include <QMap>
#include <QMenu>
#include <QMessageBox>
#include <QObject>
#include <QPixmap>
#include <QPushButton>
#include <QStackedWidget>
#include <QString>
#include <QTableWidget>
#include <QThread>
#include <QWidget>

#define TOML_EXCEPTIONS 0
#define TOML_HEADER_ONLY 1

#include <toml++/toml.hpp>
using namespace std::literals;

namespace ui {

/// @brief Represents the pages available in the main stacked widget.
enum class Page { Home, Accounts, Progress };

/// @class Window
/// @brief The main application window. Manages the UI and user interaction.
class Window final : public QMainWindow {
    Q_OBJECT

  public:
    /// @brief Constructs the main window.
    /// @param parent The parent widget.
    explicit Window(QWidget *parent = nullptr);
    ~Window();

  protected:
    /// @brief Handles window resize events to scale UI elements.
    auto resizeEvent(QResizeEvent *event) -> void override;

    /// @brief Handles global key press events.
    auto keyPressEvent(QKeyEvent *event) -> void override;

  signals:
    /// @brief Signals the worker thread to begin a login attempt.
    /// @param game The target game to launch.
    /// @param username The account username.
    /// @param password The account password.
    auto start_login(riot::Game game, const QString &username, const QString &password) -> void;

  private slots:
    /// @brief Slot to receive and display progress messages from the worker.
    auto on_login_progress_update(const QString &message) -> void;

    /// @brief Slot to handle the result of a completed login attempt.
    auto on_login_finished(bool success, const QString &message) -> void;

    /// @brief Reloads the account data from the config and repopulates the table.
    auto refresh_accounts_table() -> void;

  private:
    /// @brief Generates the full stylesheet string from the current theme.
    /// @param theme The theme data to use for generating color styles.
    /// @return A single QString containing all application styles.
    auto generate_stylesheet(const core::Theme &theme) -> QString;

    /// @brief Applies the current theme to the application.
    auto apply_theme() -> void;

  private:
    // Event Handlers
    /// @brief Handles the click event for the "Customize Theme" menu action.
    auto handle_customize_theme_button_click() -> void;

    /// @brief Handles the click event for the home/back button.
    auto handle_home_button_click() -> void;

    /// @brief Handles the click event for a game banner on the home page.
    auto handle_game_banner_click(riot::Game game) -> void;

    /// @brief Handles selection changes in the accounts table.
    auto handle_table_selection_changed() -> void;

    /// @brief Handles the click event for the main login button.
    auto handle_login_button_click() -> void;

    /// @brief Handles the click event for the "Add Account" button.
    auto handle_add_account_button_click() -> void;

    /// @brief Handles the click event for the "Remove Account" button.
    auto handle_remove_account_button_click() -> void;

    /// @brief Handles data changes within a cell of the accounts table.
    auto handle_account_cell_updated(int row, int column) -> void;

    // UI Setup
    /// @brief Sets up UI elements common to all pages (e.g., top bar).
    auto setup_common_ui() -> void;

    /// @brief Sets up the home page with game selection banners.
    auto setup_home_page() -> void;

    /// @brief Sets up the accounts page with the accounts table.
    auto setup_accounts_page() -> void;

    // UI Updates
    /// @brief Clears the current selection in the accounts table.
    auto reset_account_selection() -> void;

    /// @brief Updates the bottom bar content based on the selected game.
    auto update_bottom_bar_content(riot::Game game) -> void;

    // Helpers
    /// @brief Creates a styled QPushButton for a game banner.
    auto create_banner_button(const QString &image_path, riot::Game game) -> QPushButton *;

  private:
    // UI Widgets
    QStackedWidget *widget_main_stacked_;
    QWidget *widget_menu_;
    QHBoxLayout *layout_menu_;
    QWidget *widget_accounts_content_;
    QLabel *label_accounts_;
    QTableWidget *table_accounts_;
    QWidget *widget_progress_page_;
    QLabel *label_progress_status_;
    QPushButton *button_progress_back_;
    QLabel *label_progress_game_icon_;
    QWidget *widget_top_bar_;
    QPushButton *button_top_bar_home_;
    QPushButton *button_top_bar_options_;
    QWidget *widget_bottom_bar_;
    QLabel *label_game_icon_placeholder_;
    QPushButton *button_login_;
    QPushButton *button_add_account_;
    QPushButton *button_remove_account_;

    // App management
    Updater *updater_;
    core::Theme_Config *theme_config_;
    core::Account_Config *account_config_;

    // State & Data
    riot::Game current_game_;
    QString banners_dir_;
    QString game_icons_dir_;
    QThread worker_thread_;
    QMap<riot::Game, QPixmap> original_banner_pixmaps_;
    QVector<core::Account> current_accounts_;
};

} // namespace ui
