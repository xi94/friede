// =================================================================================
// window.hpp
// =================================================================================

#pragma once

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

#include "riot/client.hpp"
#include "ui/login_worker.hpp"
#include "updater.hpp"

#define TOML_EXCEPTIONS 0
#define TOML_HEADER_ONLY 1

#include <toml++/toml.hpp>
using namespace std::literals;

namespace ui {

/// @brief Represents the pages available in the main stacked widget.
enum class Page { Home, Accounts, Progress };

/**
 * @class Window
 * @brief The main application window. Manages the UI and user interaction.
 */
class Window final : public QMainWindow {
    Q_OBJECT

  public:
    explicit Window(QWidget *parent = nullptr);
    ~Window();

  protected:
    /// @brief Handles window resize events to scale UI elements.
    auto resizeEvent(QResizeEvent *event) -> void override;

    /// @brief Handles global key press events.
    auto keyPressEvent(QKeyEvent *event) -> void override;

  signals:
    /**
     * @brief Signals the worker thread to begin a login attempt.
     * @param game The target game to launch.
     * @param username The account username.
     * @param password The account password.
     */
    void start_login(riot::Game game, const QString &username, const QString &password);

  private slots:
    /// @brief Slot to receive and display progress messages from the worker.
    void on_login_progress_update(const QString &message);

    /// @brief Slot to handle the result of a completed login attempt.
    void on_login_finished(bool success, const QString &message);

    /// @brief Slot to check for updates on click
    void on_check_for_updates_clicked();

  private:
    // UI Widgets
    QStackedWidget *widget_main_stacked;
    QWidget *widget_menu;
    QHBoxLayout *layout_menu;
    QWidget *widget_accounts_content;
    QLabel *label_accounts;
    QTableWidget *table_accounts;
    QWidget *widget_progress_page;
    QLabel *label_progress_status;
    QPushButton *button_progress_back;
    QLabel *label_progress_game_icon;
    QWidget *widget_top_bar;
    QPushButton *button_top_bar_home;
    QPushButton *button_top_bar_options;
    QWidget *widget_bottom_bar;
    QLabel *label_game_icon_placeholder;
    QPushButton *button_login;
    QPushButton *button_add_account;
    QPushButton *button_remove_account;

    // App management
    Updater *updater;

    // State & Data
    riot::Game current_game;
    QString banners_dir;
    QString game_icons_dir;
    QString config_path;
    toml::parse_result config;
    QThread worker_thread;
    QMap<riot::Game, QPixmap> original_banner_pixmaps;

    // Event Handlers
    auto handle_home_button_click() -> void;
    auto handle_game_banner_click(riot::Game game) -> void;
    auto handle_table_selection_changed() -> void;
    auto handle_login_button_click() -> void;
    auto handle_add_account_button_click() -> void;
    auto handle_remove_account_button_click() -> void;
    auto handle_account_cell_updated(int row, int column) -> void;

    // UI Setup
    auto setup_common_ui() -> void;
    auto setup_home_page() -> void;
    auto setup_accounts_page() -> void;

    // Data Management
    auto save_config_to_file() -> bool;
    auto save_account_data(int row, int column, const QString &new_value) -> void;
    auto add_account_to_config(const QString &note, const QString &username, const QString &password) -> void;

    // UI Updates
    auto reset_account_selection() -> void;
    auto update_bottom_bar_content(riot::Game game) -> void;

    // Helpers
    auto create_banner_button(const QString &image_path, riot::Game game) -> QPushButton *;
};

} // namespace ui
