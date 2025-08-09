// =================================================================================
// ui/window.hpp
// =================================================================================

#pragma once

#include "core/account.hpp"
#include "core/theme.hpp"
#include "riot/client.hpp"
#include "theme_editor.hpp"
#include "ui/control_bar.hpp"
#include "ui/login_worker.hpp"
#include "ui/misc_bar.hpp"
#include "ui/title_bar.hpp"
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
#include <QMouseEvent>
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

/// @enum Page
/// @brief Defines the pages available in the main stacked widget.
enum class Page { Home, Accounts, Progress };

/// @class Window
/// @brief The main application window.
///
/// This class serves as the top-level UI container, orchestrating interactions
/// between the various UI components like the title bar, content pages, and
/// control bars. It also manages application state and core logic connections.
class Window final : public QMainWindow {
    Q_OBJECT

  public:
    /// @brief Constructs the main window and initializes all UI components.
    /// @param parent The parent widget, typically null for the main window.
    explicit Window(QWidget *parent = nullptr);

    /// @brief Destructs the main window.
    ~Window();

  protected:
    /// @brief Handles window resize events to dynamically adjust content.
    auto resizeEvent(QResizeEvent *event) -> void override;

    /// @brief Handles key press events, such as using Escape to clear selections.
    auto keyPressEvent(QKeyEvent *event) -> void override;

    /// @brief Handles mouse move events for custom window resizing.
    auto mouseMoveEvent(QMouseEvent *event) -> void override;

    /// @brief Handles mouse press events for custom window resizing.
    auto mousePressEvent(QMouseEvent *event) -> void override;

    /// @brief Handles mouse release events for custom window resizing.
    auto mouseReleaseEvent(QMouseEvent *event) -> void override;

  signals:
    /// @brief Emitted to start the login process in a worker thread.
    /// @param game The game to log in to.
    /// @param username The account username.
    /// @param password The account password.
    auto start_login(riot::Game game, const QString &username, const QString &password) -> void;

  private slots:
    /// @brief Updates the progress page with messages from the login worker.
    auto on_login_progress_update(const QString &message) -> void;

    /// @brief Handles the final result of the login attempt.
    auto on_login_finished(bool success, const QString &message) -> void;

    /// @brief Reloads and repopulates the accounts table from the configuration.
    auto refresh_accounts_table() -> void;

    /// @brief Handles the title bar's home button click to return to the main page.
    auto handle_home_button_click() -> void;

  private:
    /// @brief Generates a full stylesheet string from a theme configuration.
    auto generate_stylesheet(const core::Theme &theme) -> QString;

    /// @brief Applies the current theme's stylesheet to the application.
    auto apply_theme() -> void;

    /// @brief Handles the action to open the theme customization dialog.
    auto handle_customize_theme_button_click() -> void;

    /// @brief Handles a click on a game banner, switching to the accounts page.
    auto handle_game_banner_click(riot::Game game) -> void;

    /// @brief Updates control states when the account table selection changes.
    auto handle_table_selection_changed() -> void;

    /// @brief Handles the login button click action.
    auto handle_login_button_click() -> void;

    /// @brief Handles the add account button click action.
    auto handle_add_account_button_click() -> void;

    /// @brief Handles the remove account button click action.
    auto handle_remove_account_button_click() -> void;

    /// @brief Saves changes to an account after a cell is edited in the table.
    auto handle_account_cell_updated(int row, int column) -> void;

    /// @brief Initializes the main home page with game selection banners.
    auto setup_home_page() -> void;

    /// @brief Initializes the accounts page with the accounts table.
    auto setup_accounts_page() -> void;

    /// @brief Clears the current selection in the accounts table.
    auto reset_account_selection() -> void;

    /// @brief Factory method to create a game banner button.
    auto create_banner_button(const QString &image_path, riot::Game game) -> QPushButton *;

    /// @brief Updates the bottom control bar with context for the selected game.
    auto update_bottom_bar_content(riot::Game game) -> void;

  private:
    Updater *updater_;
    core::Theme_Config *theme_config_;
    core::Account_Config *account_config_;
    QThread worker_thread_;
    QVector<core::Account> accounts_cache_;
    QMap<riot::Game, QPixmap> banner_pixmaps_;
    riot::Game current_game_;

    QRect window_size_;
    QPoint mouse_click_position_;

    QString banners_dir_;
    QString game_icons_dir_;

    QStackedWidget *main_stacked_widget_;
    QWidget *home_page_;
    QHBoxLayout *home_page_layout_;
    QWidget *accounts_page_;
    QWidget *progress_page_;

    Misc_Bar *misc_bar_;
    Title_Bar *title_bar_;
    Control_Bar *control_bar_;

    QLabel *accounts_label_;
    QTableWidget *accounts_table_;

    QLabel *progress_status_label_;
    QPushButton *progress_back_button_;
    QLabel *progress_game_icon_label_;
};

} // namespace ui
