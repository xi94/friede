// =================================================================================
// ui/window.hpp
// =================================================================================

#pragma once

#include "core/account.hpp"
#include "core/theme.hpp"
#include "riot/client.hpp"
#include "theme_editor.hpp"
#include "ui/login_worker.hpp"
#include "ui/title_bar.hpp" // <-- Include the new header
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

enum class Page { Home, Accounts, Progress };

class Window final : public QMainWindow {
    Q_OBJECT

  public:
    explicit Window(QWidget *parent = nullptr);
    ~Window();

  protected:
    auto resizeEvent(QResizeEvent *event) -> void override;
    auto keyPressEvent(QKeyEvent *event) -> void override;
    auto mouseMoveEvent(QMouseEvent *event) -> void override;
    auto mousePressEvent(QMouseEvent *event) -> void override;
    auto mouseReleaseEvent(QMouseEvent *event) -> void override;

  signals:
    auto start_login(riot::Game game, const QString &username, const QString &password) -> void;

  private slots:
    auto on_login_progress_update(const QString &message) -> void;
    auto on_login_finished(bool success, const QString &message) -> void;
    auto refresh_accounts_table() -> void;
    auto handle_home_button_click() -> void;

  private:
    auto generate_stylesheet(const core::Theme &theme) -> QString;
    auto apply_theme() -> void;
    auto handle_customize_theme_button_click() -> void;
    auto handle_game_banner_click(riot::Game game) -> void;
    auto handle_table_selection_changed() -> void;
    auto handle_login_button_click() -> void;
    auto handle_add_account_button_click() -> void;
    auto handle_remove_account_button_click() -> void;
    auto handle_account_cell_updated(int row, int column) -> void;

    auto setup_left_bar() -> void;
    auto setup_bottom_bar() -> void;
    auto setup_home_page() -> void;
    auto setup_accounts_page() -> void;

    auto reset_account_selection() -> void;
    auto update_bottom_bar_content(riot::Game game) -> void;
    auto create_banner_button(const QString &image_path, riot::Game game) -> QPushButton *;

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

    Title_Bar *title_bar_;
    QWidget *left_bar_widget_;
    QVBoxLayout *left_bar_layout_;
    QPushButton *options_button_;
    QWidget *bottom_bar_widget_;
    QPushButton *login_button_;
    QPushButton *add_account_button_;
    QPushButton *remove_account_button_;
    QLabel *bottom_bar_game_icon_label_;

    QLabel *accounts_label_;
    QTableWidget *accounts_table_;

    QLabel *progress_status_label_;
    QPushButton *progress_back_button_;
    QLabel *progress_game_icon_label_;
};

} // namespace ui
