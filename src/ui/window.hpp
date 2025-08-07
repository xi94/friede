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

/// @brief Represents the pages available in the main stacked widget
enum class Page { Home, Accounts, Progress };

/// @class Window
/// @brief The main application window manages the ui and user interaction
class Window final : public QMainWindow {
    Q_OBJECT

  public:
    /// @brief constructs the main window
    /// @param parent the parent widget
    explicit Window(QWidget *parent = nullptr);
    ~Window();

  protected:
    /// @brief handles window resize events to scale ui elements
    auto resizeEvent(QResizeEvent *event) -> void override;

    /// @brief filters events through registered child widgets
    auto eventFilter(QObject *watched, QEvent *event) -> bool override;

    /// @brief handles global key press events
    auto keyPressEvent(QKeyEvent *event) -> void override;

    /// @brief handles mouse movement events
    auto mouseMoveEvent(QMouseEvent *event) -> void override;

    /// @brief handles mouse click events
    auto mousePressEvent(QMouseEvent *event) -> void override;

  signals:
    /// @brief signals the worker thread to begin a login attempt
    /// @param game the target game to launch
    /// @param username the account username
    /// @param password the account password
    auto start_login(riot::Game game, const QString &username, const QString &password) -> void;

  private slots:
    /// @brief slot to receive and display progress messages from the worker
    auto on_login_progress_update(const QString &message) -> void;

    /// @brief slot to handle the result of a completed login attempt
    auto on_login_finished(bool success, const QString &message) -> void;

    /// @brief reloads the account data from the config and repopulates the table
    auto refresh_accounts_table() -> void;

  private:
    /// @brief generates the full stylesheet string from the current theme
    /// @param theme the theme data to use for generating color styles
    /// @return a single qstring containing all application styles
    auto generate_stylesheet(const core::Theme &theme) -> QString;

    /// @brief applies the current theme to the application
    auto apply_theme() -> void;

  private:
    //
    // event handlers
    //

    /// @brief handles the click event for the "customize theme" menu action
    auto handle_customize_theme_button_click() -> void;

    /// @brief handles the click event for the home/back button
    auto handle_home_button_click() -> void;

    /// @brief handles the click event for a game banner on the home page
    auto handle_game_banner_click(riot::Game game) -> void;

    /// @brief handles selection changes in the accounts table
    auto handle_table_selection_changed() -> void;

    /// @brief handles the click event for the main login button
    auto handle_login_button_click() -> void;

    /// @brief handles the click event for the "add account" button
    auto handle_add_account_button_click() -> void;

    /// @brief handles the click event for the "remove account" button
    auto handle_remove_account_button_click() -> void;

    /// @brief handles data changes within a cell of the accounts table
    auto handle_account_cell_updated(int row, int column) -> void;

    //
    // ui setup
    //

    /// @brief sets up ui elements common to all pages (eg, top bar)
    auto setup_common_ui() -> void;

    /// @brief sets up the home page with game selection banners
    auto setup_home_page() -> void;

    /// @brief sets up the accounts page with the accounts table
    auto setup_accounts_page() -> void;

    //
    // ui updates
    //

    /// @brief clears the current selection in the accounts table
    auto reset_account_selection() -> void;

    /// @brief updates the bottom bar content based on the selected game
    auto update_bottom_bar_content(riot::Game game) -> void;

    //
    // helpers
    //

    /// @brief creates a styled qpushbutton for a game banner
    auto create_banner_button(const QString &image_path, riot::Game game) -> QPushButton *;

  private:
    //
    // core logic & data
    //

    Updater *updater_;
    core::Theme_Config *theme_config_;
    core::Account_Config *account_config_;
    QThread worker_thread_;
    QVector<core::Account> accounts_cache_;
    QMap<riot::Game, QPixmap> banner_pixmaps_;
    riot::Game current_game_;

    //
    // ui state
    //

    QPoint mouse_click_position_;
    QString banners_dir_;
    QString game_icons_dir_;

    //
    // main layout & pages
    //

    QStackedWidget *main_stacked_widget_;
    QWidget *home_page_;
    QHBoxLayout *home_page_layout_;
    QWidget *accounts_page_;
    QWidget *progress_page_;

    //
    // top bar widgets
    //

    QWidget *top_bar_widget_;
    QPushButton *home_button_;
    QPushButton *options_button_;
    QPushButton *minimize_button_;
    QPushButton *maximize_button_;
    QPushButton *close_button_;

    //
    // bottom bar widgets
    //

    QWidget *bottom_bar_widget_;
    QPushButton *login_button_;
    QPushButton *add_account_button_;
    QPushButton *remove_account_button_;
    QLabel *bottom_bar_game_icon_label_;

    //
    // accounts page widgets
    //

    QLabel *accounts_label_;
    QTableWidget *accounts_table_;

    //
    // progress page widgets
    //

    QLabel *progress_status_label_;
    QPushButton *progress_back_button_;
    QLabel *progress_game_icon_label_;
};

} // namespace ui
