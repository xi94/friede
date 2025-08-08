// =================================================================================
// ui/window.cc
// =================================================================================

#include "ui/window.hpp"

#include "core/account.hpp"
#include "ui/add_account_dialog.hpp"
#include "ui/password_table_widget.hpp"
#include "ui/theme_editor.hpp"

#include "central_widget.hpp"

#include <QCoreApplication>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QResizeEvent>
#include <QScreen>
#include <QStandardPaths>
#include <QStyle>
#include <QTextStream>
#include <QVBoxLayout>

#include <fstream>
#include <iostream>
#include <string>

namespace ui {

Window::Window(QWidget *parent)
    : QMainWindow{parent}
    , main_stacked_widget_{new QStackedWidget{this}}
    , home_page_{new QWidget{}}
    , home_page_layout_{new QHBoxLayout{home_page_}}
    , accounts_page_{new QWidget{}}
    , accounts_label_{new QLabel{}}
    , accounts_table_{new QTableWidget{0, 3}}
    , progress_page_{new QWidget{}}
    , progress_status_label_{new QLabel{"Initializing..."}}
    , progress_back_button_{new QPushButton{"back"}}
    , progress_game_icon_label_{new QLabel{}}
    , top_bar_widget_{new QWidget{this}}
    , left_bar_widget_{new QWidget{this}}
    , left_bar_layout_{new QVBoxLayout{left_bar_widget_}}
    , home_button_{new QPushButton{"\tback", top_bar_widget_}}
    , options_button_{new QPushButton{"", left_bar_widget_}}
    , minimize_button_{new QPushButton{"", top_bar_widget_}}
    , maximize_button_{new QPushButton{"", top_bar_widget_}}
    , close_button_{new QPushButton{"", top_bar_widget_}}
    , bottom_bar_widget_{new QWidget{this}}
    , login_button_{new QPushButton{"Login", bottom_bar_widget_}}
    , add_account_button_{new QPushButton{"Add Account", bottom_bar_widget_}}
    , remove_account_button_{new QPushButton{"Remove Account", bottom_bar_widget_}}
    , bottom_bar_game_icon_label_{new QLabel{bottom_bar_widget_}}
    , updater_{new Updater{this}}
    , theme_config_{new core::Theme_Config{}}
    , account_config_{new core::Account_Config{}}
    , window_size_{}
    , mouse_click_position_{}
    , banners_dir_{QCoreApplication::applicationDirPath() + "/banners/"}
    , game_icons_dir_{QCoreApplication::applicationDirPath() + "/icons/"}
{
    auto *worker = new Login_Worker{};
    worker->moveToThread(&worker_thread_);

    connect(&worker_thread_, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &Window::start_login, worker, &Login_Worker::do_login);
    connect(worker, &Login_Worker::progress_updated, this, &Window::on_login_progress_update);
    connect(worker, &Login_Worker::login_finished, this, &Window::on_login_finished);

    worker_thread_.start();

    setMinimumSize(750, 450);
    setWindowTitle("a flame alighteth");

    QMainWindow::setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    const auto available_geometry = QGuiApplication::primaryScreen()->availableGeometry();
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), available_geometry));

    auto *main_window_layout = new QHBoxLayout{};
    main_window_layout->setContentsMargins(5, 5, 5, 5);
    main_window_layout->setSpacing(0);

    home_page_layout_->setSpacing(20);
    home_page_layout_->setContentsMargins(20, 0, 20, 0);

    setup_common_ui();

    left_bar_widget_->setObjectName("left_bar_widget");
    left_bar_widget_->setFixedWidth(30);

    left_bar_layout_->setContentsMargins(5, 10, 5, 10);
    left_bar_layout_->setSpacing(10);
    left_bar_layout_->addStretch();

    main_window_layout->addWidget(left_bar_widget_);

    auto *right_column_widget = new QWidget{};
    auto *right_column_layout = new QVBoxLayout{right_column_widget};
    right_column_layout->setContentsMargins(0, 0, 0, 0);
    right_column_layout->setSpacing(0);
    right_column_layout->addWidget(top_bar_widget_, 0, Qt::AlignTop);

    setup_home_page();
    setup_accounts_page();

    auto *progress_layout = new QVBoxLayout{progress_page_};
    progress_game_icon_label_->setFixedSize(128, 128);
    progress_game_icon_label_->setAlignment(Qt::AlignCenter);
    progress_status_label_->setAlignment(Qt::AlignCenter);
    progress_back_button_->hide();
    progress_back_button_->setFixedSize(150, 30);

    progress_layout->addStretch();
    progress_layout->addWidget(progress_game_icon_label_, 0, Qt::AlignCenter);
    progress_layout->addWidget(progress_status_label_, 0, Qt::AlignCenter);
    progress_layout->addWidget(progress_back_button_, 0, Qt::AlignCenter);
    progress_layout->addStretch();

    connect(progress_back_button_, &QPushButton::clicked, this, &Window::handle_home_button_click);

    main_stacked_widget_->addWidget(home_page_);
    main_stacked_widget_->addWidget(accounts_page_);
    main_stacked_widget_->addWidget(progress_page_);

    right_column_layout->addWidget(main_stacked_widget_);
    right_column_layout->addWidget(bottom_bar_widget_, 0, Qt::AlignBottom);

    main_window_layout->addWidget(right_column_widget);

    auto *central_widget = new Central_Widget{theme_config_, this};
    central_widget->setObjectName("central_widget");
    central_widget->setLayout(main_window_layout);
    setCentralWidget(central_widget);

    bottom_bar_widget_->hide();
    bottom_bar_game_icon_label_->hide();
    login_button_->hide();
    add_account_button_->hide();
    remove_account_button_->hide();
    top_bar_widget_->show();

    // registering event filters to all our push buttons after all the widgets are created to avoid race condition with dragging
    for (auto *button : QMainWindow::findChildren<QPushButton *>()) {
        button->installEventFilter(this);
    }

    apply_theme();
    updater_->check_for_updates();
}

Window::~Window()
{
    worker_thread_.quit();
    worker_thread_.wait();
}

auto Window::resizeEvent(QResizeEvent *event) -> void
{
    QMainWindow::resizeEvent(event);
    home_page_layout_->blockSignals(true);

    const int available_content_height = height() - top_bar_widget_->height() - bottom_bar_widget_->height();
    const int available_content_width = width() - left_bar_widget_->width();
    if (available_content_width <= 0 || available_content_height <= 0) {
        home_page_layout_->blockSignals(false);
        return;
    }

    const int num_banners = home_page_layout_->count();
    if (num_banners == 0) {
        home_page_layout_->blockSignals(false);
        return;
    }

    const int horizontal_margins = home_page_layout_->contentsMargins().left() + home_page_layout_->contentsMargins().right();

    int total_spacing = home_page_layout_->spacing() * (num_banners - 1);
    if (total_spacing < 0) total_spacing = 0;

    constexpr double aspect_ratio = 2160.0 / 1440.0;

    int effective_image_area_width = available_content_width - horizontal_margins - total_spacing;
    int desired_banner_width = effective_image_area_width / num_banners;
    int desired_banner_height = static_cast<int>(desired_banner_width * aspect_ratio);

    const auto min_banner_size = QSize{100, 150};
    if (desired_banner_width < min_banner_size.width()) {
        desired_banner_width = min_banner_size.width();
        desired_banner_height = static_cast<int>(desired_banner_width * aspect_ratio);
    }

    if (desired_banner_height < min_banner_size.height()) {
        desired_banner_height = min_banner_size.height();
        desired_banner_width = static_cast<int>(desired_banner_height / aspect_ratio);
    }

    if (desired_banner_height > available_content_height) {
        desired_banner_height = available_content_height;
        desired_banner_width = static_cast<int>(desired_banner_height / aspect_ratio);
    }

    for (int i = 0; i < home_page_layout_->count(); ++i) {
        auto *button = qobject_cast<QPushButton *>(home_page_layout_->itemAt(i)->widget());
        if (!button) continue;

        if (riot::is_game_index_out_of_range(i)) continue;
        const auto current_game = static_cast<riot::Game>(i);

        const auto original_pixmap = banner_pixmaps_[current_game];
        if (original_pixmap.isNull()) continue;

        const auto banner_width = desired_banner_width;
        const auto banner_height = desired_banner_height;
        const auto scaled_pixmap = original_pixmap.scaled(banner_width, banner_height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        button->setIcon(QIcon{scaled_pixmap});

        const auto image_display_size = scaled_pixmap.size();
        button->setIconSize(image_display_size);

        constexpr int image_border_padding = 2;
        const auto button_width = image_display_size.width() + (2 * image_border_padding);
        const auto button_height = image_display_size.height() + (2 * image_border_padding);
        button->setFixedSize(QSize{button_width, button_height});
    }

    home_page_layout_->blockSignals(false);
}

auto Window::eventFilter(QObject *watched, QEvent *event) -> bool
{
    if (event->type() == QEvent::MouseButtonPress) {
        auto *mouse_event = static_cast<QMouseEvent *>(event);

        // map the child widgets position on our main window, this is done to avoid race conditions with our custom dragging
        if (mouse_event->button() == Qt::LeftButton) {
            const auto object_name = watched->objectName();

            if (object_name == "minimize_button" || object_name == "maximize_button" || object_name == "close_button") {
                // since the these buttons are located on the top bar, we have to invalidate the position to avoid dragging,
                // for some reason this does not apply to buttons that create their own popup menu, they dont drag by default
                mouse_click_position_ = {-1, -1};
            } else {
                auto *child_widget = static_cast<QWidget *>(watched);
                mouse_click_position_ = child_widget->mapTo(this, mouse_event->pos());
            }
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

auto Window::keyPressEvent(QKeyEvent *event) -> void
{
    if (event->key() == Qt::Key_Escape) {
        const bool account_table_empty = accounts_table_->selectedItems().isEmpty();
        if (!account_table_empty) reset_account_selection();
    }

    QMainWindow::keyPressEvent(event);
}

// TODO clean this function up
auto Window::mouseMoveEvent(QMouseEvent *event) -> void
{
    if (event->buttons() == Qt::LeftButton) {
        constexpr int resize_margin = 8;

        const QRect initial_size = window_size_;
        const QPoint initial_position = mouse_click_position_;

        const QRect size = {window_size_};
        const auto [x, y] = mouse_click_position_;

        const bool is_dragging = (y > resize_margin && y < top_bar_widget_->height() && x > left_bar_widget_->width());
        if (is_dragging) {
            const auto new_position = event->globalPosition().toPoint() - mouse_click_position_;
            QMainWindow::move(new_position);
            return;
        }

        //
        // window resizing
        //

        // FIXME the images are a bit jittery for left / up movement due to our moving with resize calculations

        const bool on_top_edge = y <= resize_margin;
        const bool on_left_edge = x <= resize_margin;
        const bool on_right_edge = x >= size.width() - resize_margin;
        const bool on_bottom_edge = y >= size.height() - resize_margin;

        const bool is_resizing = on_left_edge || on_right_edge || on_top_edge || on_bottom_edge;
        if (is_resizing) {
            // TODO this isnt size, it updates both position and size, editor renaming feature is broken, and im too tired to rename it now
            QRect new_size = {size};

            if (on_right_edge) {
                const int delta_x = event->pos().x() - x;
                new_size.setWidth(size.width() + delta_x);
            } else if (on_left_edge) {
                const int new_x = event->globalPosition().x() - x;
                const int new_width = size.right() - new_x;
                const int min_width = QMainWindow::minimumWidth();

                // avoid going past min width or window drifts to narnia
                if (new_width >= min_width) {
                    new_size.setX(new_x);
                    new_size.setWidth(new_width);
                } else {
                    new_size.setWidth(min_width);
                    new_size.setX(size.right() - min_width);
                }
            }

            if (on_bottom_edge) {
                const int delta_y = event->pos().y() - y;
                new_size.setHeight(size.height() + delta_y);
            } else if (on_top_edge) {
                const int new_y = event->globalPosition().y() - y;
                const int new_height = size.bottom() - new_y;
                const int min_height = QMainWindow::minimumHeight();

                // avoid going past min height or window drifts to narnia
                if (new_height >= min_height) {
                    new_size.setY(new_y);
                    new_size.setHeight(new_height);
                } else {
                    new_size.setHeight(min_height);
                    new_size.setY(size.bottom() - min_height);
                }
            }

            setGeometry(new_size);
        }
    }
}

auto Window::mousePressEvent(QMouseEvent *event) -> void
{
    // maybe this should be checking if the mouse is within the window size we just stored?
    if (event->button() == Qt::LeftButton) {
        grabMouse();

        window_size_ = QMainWindow::geometry();
        mouse_click_position_ = event->pos();
    }
}

auto Window::mouseReleaseEvent(QMouseEvent *event) -> void
{
    if (event->button() == Qt::LeftButton) releaseMouse();
}

auto Window::on_login_progress_update(const QString &message) -> void
{
    progress_status_label_->setText(message);
}

auto Window::on_login_finished(bool success, const QString &message) -> void
{
    progress_status_label_->setText(message);
    progress_back_button_->show();

    if (success) {
        progress_status_label_->setStyleSheet(QString("color: %1; font-weight: bold;").arg(theme_config_->load().success.name()));
    } else {
        progress_status_label_->setStyleSheet(QString("color: %1; font-weight: bold;").arg(theme_config_->load().error.name()));
    }
}

auto Window::refresh_accounts_table() -> void
{
    accounts_table_->blockSignals(true);

    accounts_table_->setRowCount(0);
    accounts_cache_ = account_config_->get_accounts();

    for (const auto &account : accounts_cache_) {
        const int row = accounts_table_->rowCount();
        accounts_table_->insertRow(row);
        accounts_table_->setItem(row, 0, new QTableWidgetItem{account.note});
        accounts_table_->setItem(row, 1, new QTableWidgetItem{account.username});

        auto *password_item = new Password_Table_Widget{};
        password_item->setData(Qt::DisplayRole, QString("************"));
        password_item->setData(Qt::UserRole, account.password);
        accounts_table_->setItem(row, 2, password_item);
    }

    accounts_table_->blockSignals(false);
    handle_table_selection_changed();
}

auto Window::generate_stylesheet(const core::Theme &theme) -> QString
{
    const auto colors = QString{"QMainWindow, QDialog, QWidget#central_widget, QWidget#home_page, "
                                "QWidget#accounts_page, "
                                "QWidget#progress_page {"
                                "    background-color: %1;"
                                "    color: %2;"
                                "}"
                                "QWidget#central_widget {"
                                "    background-color: transparent;"
                                "}"
                                "QWidget#top_bar_widget, QWidget#bottom_bar_widget, QWidget#left_bar_widget {"
                                "    background-color: %8;"
                                "    border-color: %3;"
                                "}"
                                "QMenu {"
                                "    background-color: %8;"
                                "    color: %2;"
                                "    border-color: %3;"
                                "}"
                                "QMenu::item:selected {"
                                "    background-color: %5;"
                                "}"
                                "QPushButton {"
                                "    background-color: %4;"
                                "    color: %2;"
                                "    border-color: %3;"
                                "}"
                                "QPushButton:hover {"
                                "    background-color: %5;"
                                "}"
                                "QPushButton:disabled {"
                                "    background-color: %6;"
                                "    color: %7;"
                                "    border-color: %3;"
                                "}"
                                "QPushButton#banner_button {"
                                "    background-color: transparent;"
                                "    border-color: %9;"
                                "}"
                                "QPushButton#banner_button:hover {"
                                "    border-color: %10;"
                                "}"
                                "QPushButton#home_button, QPushButton#options_button {"
                                "    background-color: transparent;"
                                "    border-color: transparent;"
                                "}"
                                "QPushButton#home_button:hover, QPushButton#options_button:hover {"
                                "    background-color: %5;"
                                "}"
                                "QTableWidget {"
                                "    background-color: %1;"
                                "    border-color: %3;"
                                "    gridline-color: %3;"
                                "}"
                                "QTableWidget::item {"
                                "    background-color: %1;"
                                "    color: %2;"
                                "    border-color: %3;"
                                "}"
                                "QTableWidget::item:hover, QTableWidget::item:selected {"
                                "    background-color: %5;"
                                "    color: %2;"
                                "}"
                                "QHeaderView::section {"
                                "    background-color: %8;"
                                "    color: %2;"
                                "    border-color: %3;"
                                "}"
                                "QLineEdit {"
                                "    background-color: %8;"
                                "    color: %2;"
                                "    border-color: %3;"
                                "}"
                                "QLabel#game_icon_placeholder {"
                                "    background-color: transparent;"
                                "    border-color: transparent;"
                                "}"}
                            .arg(theme.background_dark.name(), theme.text_primary.name(), theme.border.name(), theme.button_primary.name(),
                                 theme.button_hover.name(), theme.button_disabled.name(), theme.text_disabled.name(),
                                 theme.background_light.name(), theme.accent.name(), theme.accent_hover.name());

    const auto layout = QString{"QWidget#top_bar_widget {"
                                "    border-style: solid;"
                                "    border-width: 0px 0px 1px 0px;"
                                "}"
                                "QWidget#bottom_bar_widget {"
                                "    border-style: solid;"
                                "    border-width: 1px 0px 0px 0px;"
                                "}"
                                "QWidget#left_bar_widget {"
                                "    border-style: solid;"
                                "    border-width: 0px 1px 0px 0px;"
                                "}"
                                "QMenu {"
                                "    border: 1px solid;"
                                "}"
                                "QMenu::item {"
                                "    padding: 4px 20px;"
                                "}"
                                "QPushButton, QLineEdit, QTableWidget, QHeaderView {"
                                "    outline: none;"
                                "}"
                                "QPushButton {"
                                "    border-style: solid;"
                                "    border-width: 1px;"
                                "    padding: 5px 15px;"
                                "    border-radius: 5px;"
                                "}"
                                "QPushButton#banner_button {"
                                "    border-width: 3px;"
                                "    padding: 0px;"
                                "}"
                                "QPushButton#home_button, QPushButton#options_button {"
                                "    border: none;"
                                "    padding: 2px;"
                                "}"
                                "QPushButton#options_button::menu-indicator {"
                                "    image: none;"
                                "    width: 0px;"
                                "}"
                                "QTableWidget::item {"
                                "    border-style: solid;"
                                "    border-width: 0px 1px 1px 1px;"
                                "}"
                                "QHeaderView::section {"
                                "    border: 1px solid;"
                                "    padding: 4px;"
                                "}"
                                "QLineEdit {"
                                "    border: 1px solid;"
                                "    padding: 5px;"
                                "    border-radius: 3px;"
                                "}"
                                "QLabel#game_icon_placeholder {"
                                "    border: none;"
                                "}"};

    return layout + colors;
}

auto Window::apply_theme() -> void
{
    const auto theme = theme_config_->load();
    const auto stylesheet = generate_stylesheet(theme);

    QMainWindow::setStyleSheet(stylesheet);
}

auto Window::handle_customize_theme_button_click() -> void
{
    auto theme = theme_config_->load();
    auto editor = Theme_Editor{theme, this};

    if (editor.exec() == QDialog::Accepted) {
        if (theme_config_->save(theme)) {
            apply_theme();
        } else {
            QMessageBox::critical(this, "Theme Error", "Failed to save the updated theme");
        }
    }
}

auto Window::handle_home_button_click() -> void
{
    home_button_->hide();
    bottom_bar_widget_->hide();
    reset_account_selection();

    main_stacked_widget_->setCurrentIndex(static_cast<int>(Page::Home));
    top_bar_widget_->show();
}

auto Window::handle_game_banner_click(riot::Game game) -> void
{
    current_game_ = game;
    update_bottom_bar_content(game);
    main_stacked_widget_->setCurrentIndex(static_cast<int>(Page::Accounts));

    top_bar_widget_->show();
    home_button_->show();
    bottom_bar_widget_->show();
}

auto Window::handle_table_selection_changed() -> void
{
    const bool row_is_selected = !accounts_table_->selectedItems().isEmpty();
    login_button_->setEnabled(row_is_selected);
    remove_account_button_->setEnabled(row_is_selected);
}

auto Window::handle_login_button_click() -> void
{
    const int row = accounts_table_->currentRow();
    if (row == -1) return;

    top_bar_widget_->hide();
    bottom_bar_widget_->hide();

    progress_status_label_->setText("Initializing...");
    progress_back_button_->hide();

    QString icon_filename;
    switch (current_game_) {
    case riot::Game::League_of_Legends: icon_filename = "league-icon.png"; break;
    case riot::Game::Valorant: icon_filename = "valorant-icon.png"; break;
    case riot::Game::Teamfight_Tactics: icon_filename = "teamfight-icon.png"; break;
    case riot::Game::Legends_of_Runeterra: icon_filename = "runeterra-icon.png"; break;
    }

    if (!icon_filename.isEmpty()) {
        const auto game_icon_pixmap = QPixmap(game_icons_dir_ + icon_filename);
        progress_game_icon_label_->setPixmap(game_icon_pixmap.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    main_stacked_widget_->setCurrentIndex(static_cast<int>(Page::Progress));

    const auto username = accounts_table_->item(row, 1)->text();
    const auto password = accounts_table_->item(row, 2)->data(Qt::UserRole).toString();

    reset_account_selection();
    emit start_login(current_game_, username, password);
}

auto Window::handle_add_account_button_click() -> void
{
    auto dialog = Add_Account_Dialog{this};
    if (dialog.exec() == QDialog::Accepted) {
        auto new_account = core::Account{};
        new_account.note = dialog.get_note();
        new_account.username = dialog.get_username();
        new_account.password = dialog.get_password();
        if (new_account.username.isEmpty() || new_account.password.isEmpty()) {
            QMessageBox::critical(this, "Add Account", "Username and password cannot be empty");
            return;
        }

        if (account_config_->add_account(new_account)) {
            refresh_accounts_table();
            accounts_table_->setCurrentCell(accounts_table_->rowCount() - 1, 0);
        } else {
            QMessageBox::critical(this, "Add Account", "Failed to save the new account");
        }
    }
}

auto Window::handle_remove_account_button_click() -> void
{
    const int current_row = accounts_table_->currentRow();
    if (current_row == -1) {
        QMessageBox::warning(this, "Delete Account", "Please select an account to delete");
        return;
    }

    const auto &account_to_delete = accounts_cache_[current_row];
    const auto confirmation = QString{"Are you sure you want to delete '%1'?"}.arg(account_to_delete.username);
    const auto reply = QMessageBox::warning(this, "Confirm Deletion", confirmation, QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::No) return;

    if (account_config_->remove_account(current_row)) {
        refresh_accounts_table();
    } else {
        QMessageBox::critical(this, "Deletion Error", "Failed to remove the account from the configuration file");
    }
}

auto Window::handle_account_cell_updated(int row, int column) -> void
{
    if (row < 0 || row >= accounts_cache_.size()) return;
    auto updated_account = accounts_cache_[row];

    auto *item = accounts_table_->item(row, column);
    if (!item) return;

    const QString new_value = item->text();
    switch (column) {
    case 0: updated_account.note = new_value; break;
    case 1: updated_account.username = new_value; break;
    case 2:
        updated_account.password = new_value;
        accounts_table_->blockSignals(true);
        item->setData(Qt::UserRole, new_value);
        item->setText("************");
        accounts_table_->blockSignals(false);
        break;
    }

    if (!account_config_->update_account(row, updated_account)) {
        QMessageBox::critical(this, "Save Error", "Failed to save changes to the account");
        refresh_accounts_table();
    } else {
        accounts_cache_[row] = updated_account;
    }
}

auto Window::setup_common_ui() -> void
{
    top_bar_widget_->setObjectName("top_bar_widget");
    top_bar_widget_->setFixedHeight(40);

    auto *top_bar_layout = new QHBoxLayout{top_bar_widget_};
    top_bar_layout->setContentsMargins(5, 0, 5, 0);

    constexpr std::string_view control_button_stylesheet = "QPushButton {"
                                                           "  background-color: rgba(255, 255, 255, 0);"
                                                           "  border-color: rgba(255, 255, 255, 0);"
                                                           "}"
                                                           "QPushButton#maximize_button:Hover,"
                                                           "QPushButton#minimize_button:Hover {"
                                                           "  background-color: rgba(200, 200, 200, 30);"
                                                           "}"
                                                           "QPushButton#close_button:Hover {"
                                                           "  background-color: rgba(240, 0, 0, 200);"
                                                           "}"
                                                           "QPushButton#maximize_button:Pressed,"
                                                           "QPushButton#minimize_button:Pressed {"
                                                           "  background-color: rgba(100, 100, 100, 30);"
                                                           "}"
                                                           "QPushButton#close_button:Pressed {"
                                                           "  background-color: rgba(150, 0, 0, 200);"
                                                           "}";

    minimize_button_->setObjectName("minimize_button");
    minimize_button_->setIcon(QIcon::fromTheme("list-remove"));
    minimize_button_->setFixedSize(30, 30);
    minimize_button_->setIconSize({12, 12});
    minimize_button_->setStyleSheet(control_button_stylesheet.data());

    maximize_button_->setObjectName("maximize_button");
    maximize_button_->setIcon(QIcon::fromTheme("view-fullscreen"));
    maximize_button_->setFixedSize(30, 30);
    maximize_button_->setIconSize({14, 14});
    maximize_button_->setStyleSheet(control_button_stylesheet.data());

    close_button_->setObjectName("close_button");
    close_button_->setIcon(QIcon::fromTheme("window-close"));
    close_button_->setFixedSize(30, 30);
    close_button_->setIconSize({10, 10});
    close_button_->setStyleSheet(control_button_stylesheet.data());

    home_button_->setObjectName("home_button");
    home_button_->setIcon(QIcon::fromTheme("document-revert"));

    options_button_->setObjectName("options_button");
    options_button_->setIcon(QIcon::fromTheme("document-properties"));
    options_button_->setFixedSize(20, 20);

    auto *options_menu = new QMenu{options_button_};
    auto *action_customize_theme = options_menu->addAction("customize theme");
    action_customize_theme->setIcon(QIcon::fromTheme("weather-clear"));
    connect(action_customize_theme, &QAction::triggered, this, &Window::handle_customize_theme_button_click);

    options_menu->addSeparator();

    auto *action_check_for_updates = options_menu->addAction("check for updates");
    action_check_for_updates->setIcon(QIcon::fromTheme("emblem-synchronized"));
    connect(action_check_for_updates, &QAction::triggered, updater_, &Updater::check_for_updates);

    auto *action_open_directory = options_menu->addAction("open config directory");
    action_open_directory->setIcon(QIcon::fromTheme("folder-open"));
    connect(action_open_directory, &QAction::triggered, this, [this] {
        const QString config_dir = account_config_->get_config_directory_path();
        QDesktopServices::openUrl(QUrl::fromLocalFile(config_dir));
    });

    options_menu->addSeparator();

    auto *action_close = options_menu->addAction("close");
    action_close->setIcon(QIcon::fromTheme("window-close"));
    connect(action_close, &QAction::triggered, options_menu, &QWidget::close);

    options_button_->setMenu(options_menu);

    //
    // layout for top bar
    //

    top_bar_layout->addWidget(home_button_);
    top_bar_layout->addStretch();
    top_bar_layout->addWidget(minimize_button_);
    top_bar_layout->addWidget(maximize_button_);
    top_bar_layout->addWidget(close_button_);

    //
    // layout for left bar
    //
    left_bar_layout_->addWidget(options_button_);
    left_bar_layout_->addStretch();

    home_button_->hide();
    connect(home_button_, &QPushButton::clicked, this, &Window::handle_home_button_click);
    connect(close_button_, &QPushButton::clicked, this, &QWidget::close);
    connect(minimize_button_, &QPushButton::clicked, this, &QWidget::showMinimized);
    connect(maximize_button_, &QPushButton::clicked, this, [this] {
        const bool is_maximized = QMainWindow::isMaximized();
        if (is_maximized) {
            QMainWindow::showNormal();
        } else {
            QMainWindow::showMaximized();
        }
    });

    handle_home_button_click();

    bottom_bar_widget_->setObjectName("bottom_bar_widget");
    bottom_bar_widget_->setFixedHeight(50);

    auto *bottom_bar_layout = new QHBoxLayout{bottom_bar_widget_};
    bottom_bar_layout->setContentsMargins(10, 0, 15, 0);
    bottom_bar_layout->setSpacing(10);

    bottom_bar_layout->addWidget(login_button_);
    bottom_bar_layout->addWidget(add_account_button_);
    bottom_bar_layout->addWidget(remove_account_button_);
    bottom_bar_layout->addStretch();

    bottom_bar_game_icon_label_->setObjectName("game_icon_placeholder");
    bottom_bar_game_icon_label_->setFixedSize(32, 32);
    bottom_bar_game_icon_label_->setContentsMargins(0, 0, 0, 0);

    auto *game_info_layout = new QHBoxLayout{};
    game_info_layout->addWidget(bottom_bar_game_icon_label_);
    game_info_layout->setContentsMargins(0, 0, 0, 0);
    game_info_layout->setSpacing(5);
    bottom_bar_layout->addLayout(game_info_layout);

    connect(login_button_, &QPushButton::clicked, this, &Window::handle_login_button_click);
    connect(add_account_button_, &QPushButton::clicked, this, &Window::handle_add_account_button_click);
    connect(remove_account_button_, &QPushButton::clicked, this, &Window::handle_remove_account_button_click);
}

auto Window::setup_home_page() -> void
{
    QPushButton *button_league = create_banner_button("league.jpg", riot::Game::League_of_Legends);
    QPushButton *button_valorant = create_banner_button("valorant.jpg", riot::Game::Valorant);
    QPushButton *button_teamfight = create_banner_button("tft.jpg", riot::Game::Teamfight_Tactics);
    QPushButton *button_runeterra = create_banner_button("runeterra.jpg", riot::Game::Legends_of_Runeterra);

    home_page_layout_->addWidget(button_league);
    home_page_layout_->addWidget(button_valorant);
    home_page_layout_->addWidget(button_teamfight);
    home_page_layout_->addWidget(button_runeterra);

    connect(button_league, &QPushButton::clicked, this, [this] { handle_game_banner_click(riot::Game::League_of_Legends); });
    connect(button_valorant, &QPushButton::clicked, this, [this] { handle_game_banner_click(riot::Game::Valorant); });
    connect(button_teamfight, &QPushButton::clicked, this, [this] { handle_game_banner_click(riot::Game::Teamfight_Tactics); });
    connect(button_runeterra, &QPushButton::clicked, this, [this] { handle_game_banner_click(riot::Game::Legends_of_Runeterra); });
}

auto Window::setup_accounts_page() -> void
{
    auto *accounts_layout = new QVBoxLayout{accounts_page_};
    accounts_table_->setHorizontalHeaderLabels({"Note", "Username", "Password"});
    accounts_table_->horizontalHeader()->setStretchLastSection(true);
    accounts_table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    accounts_table_->setSelectionMode(QAbstractItemView::SingleSelection);
    accounts_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    accounts_table_->setEditTriggers(QAbstractItemView::DoubleClicked);
    accounts_table_->verticalHeader()->setVisible(false);

    connect(accounts_table_, &QTableWidget::cellChanged, this, &Window::handle_account_cell_updated);
    accounts_layout->addWidget(accounts_label_);
    accounts_layout->addWidget(accounts_table_);

    connect(accounts_table_, &QTableWidget::itemSelectionChanged, this, &Window::handle_table_selection_changed);

    refresh_accounts_table();
}

auto Window::reset_account_selection() -> void
{
    accounts_table_->setCurrentCell(-1, -1);
}

auto Window::update_bottom_bar_content(riot::Game game) -> void
{
    QString icon_filename;

    switch (game) {
    case riot::Game::Valorant: icon_filename = "valorant-icon.png"; break;
    case riot::Game::League_of_Legends: icon_filename = "league-icon.png"; break;
    case riot::Game::Teamfight_Tactics: icon_filename = "teamfight-icon.png"; break;
    case riot::Game::Legends_of_Runeterra: icon_filename = "runeterra-icon.png"; break;
    }

    const auto game_icon = QIcon{game_icons_dir_ + icon_filename};
    bottom_bar_game_icon_label_->setPixmap(game_icon.pixmap(QSize(32, 32)));
    bottom_bar_game_icon_label_->show();

    login_button_->show();
    add_account_button_->show();
    remove_account_button_->show();
}

auto Window::create_banner_button(const QString &image_path, riot::Game game) -> QPushButton *
{
    auto *button = new QPushButton{"", home_page_};
    button->setObjectName("banner_button");

    const QPixmap original_pixmap(banners_dir_ + image_path);
    banner_pixmaps_[game] = original_pixmap;
    button->setIcon(QIcon(original_pixmap));
    return button;
}

} // namespace ui
