// =================================================================================
// ui/window.cc
// =================================================================================

#include "ui/window.hpp"

#include "central_widget.hpp"
#include "core/account.hpp"
#include "ui/add_account_dialog.hpp"
#include "ui/password_table_widget.hpp"
#include "ui/theme_editor.hpp"

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
    , accounts_table_{new QTableWidget{0, 3}}
    , progress_page_{new QWidget{}}
    , progress_status_label_{new QLabel{"Initializing..."}}
    , progress_back_button_{new QPushButton{"back"}}
    , progress_game_icon_label_{new QLabel{}}
    , title_bar_{new Title_Bar{this, "a flame alighteth"}}
    , misc_bar_{new Misc_Bar{this}}
    , control_bar_{new Control_Bar{this}}
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

    QMainWindow::connect(&worker_thread_, &QThread::finished, worker, &QObject::deleteLater);
    QMainWindow::connect(this, &Window::start_login, worker, &Login_Worker::do_login);
    QMainWindow::connect(worker, &Login_Worker::progress_updated, this, &Window::on_login_progress_update);
    QMainWindow::connect(worker, &Login_Worker::login_finished, this, &Window::on_login_finished);

    worker_thread_.start();

    QMainWindow::setMinimumSize(750, 450);
    QMainWindow::setWindowTitle("a flame alighteth");

    QMainWindow::setAttribute(Qt::WA_TranslucentBackground);
    QMainWindow::setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

    const auto available_geometry = QGuiApplication::primaryScreen()->availableGeometry();
    QMainWindow::setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, QMainWindow::size(), available_geometry));

    auto *main_window_layout = new QHBoxLayout{};
    main_window_layout->setContentsMargins(3, 3, 3, 3);
    main_window_layout->setSpacing(0);

    home_page_layout_->setSpacing(20);
    home_page_layout_->setContentsMargins(20, 0, 20, 0);

    main_window_layout->addWidget(misc_bar_);

    auto *right_column_widget = new QWidget{};
    auto *right_column_layout = new QVBoxLayout{right_column_widget};
    right_column_layout->setContentsMargins(0, 0, 0, 0);
    right_column_layout->setSpacing(0);
    right_column_layout->addWidget(title_bar_, 0, Qt::AlignTop);

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

    QMainWindow::connect(progress_back_button_, &QPushButton::clicked, this, &Window::handle_home_button_click);

    main_stacked_widget_->addWidget(home_page_);
    main_stacked_widget_->addWidget(accounts_page_);
    main_stacked_widget_->addWidget(progress_page_);

    right_column_layout->addWidget(main_stacked_widget_, 1);
    right_column_layout->addWidget(control_bar_, 0, Qt::AlignBottom);

    main_window_layout->addWidget(right_column_widget);

    auto *central_widget = new Central_Widget{theme_config_, this};
    central_widget->setObjectName("central_widget");
    central_widget->setLayout(main_window_layout);
    QMainWindow::setCentralWidget(central_widget);

    control_bar_->hide();
    handle_home_button_click();

    QMainWindow::connect(title_bar_, &Title_Bar::home_button_clicked, this, &Window::handle_home_button_click);

    QMainWindow::connect(misc_bar_, &Misc_Bar::customize_theme_requested, this, &Window::handle_customize_theme_button_click);
    QMainWindow::connect(misc_bar_, &Misc_Bar::check_for_updates_requested, updater_, &Updater::check_for_updates);
    QMainWindow::connect(misc_bar_, &Misc_Bar::open_config_directory_requested, this, [this] {
        const QString config_dir = account_config_->get_config_directory_path();
        QDesktopServices::openUrl(QUrl::fromLocalFile(config_dir));
    });

    QMainWindow::connect(control_bar_, &Control_Bar::login_clicked, this, &Window::handle_login_button_click);
    QMainWindow::connect(control_bar_, &Control_Bar::add_account_clicked, this, &Window::handle_add_account_button_click);
    QMainWindow::connect(control_bar_, &Control_Bar::remove_account_clicked, this, &Window::handle_remove_account_button_click);

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

    const int available_content_height = height() - title_bar_->height() - control_bar_->height();
    const int available_content_width = width() - misc_bar_->width();
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

        const QRect size = {window_size_};
        const auto [x, y] = mouse_click_position_;

        const bool on_top_edge = y <= resize_margin;
        const bool on_left_edge = x <= resize_margin;
        const bool on_right_edge = x >= size.width() - resize_margin;
        const bool on_bottom_edge = y >= size.height() - resize_margin;

        // FIXME the images are a bit jittery for left / up movement due to our moving with resize calculations
        const bool is_resizing = on_left_edge || on_right_edge || on_top_edge || on_bottom_edge;
        if (is_resizing) {
            QRect new_size = {size};

            if (on_right_edge) {
                const int delta_x = event->pos().x() - x;
                new_size.setWidth(size.width() + delta_x);
            } else if (on_left_edge) {
                const int new_x = event->globalPosition().x() - x;
                const int new_width = size.right() - new_x;
                const int min_width = QMainWindow::minimumWidth();

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

                if (new_height >= min_height) {
                    new_size.setY(new_y);
                    new_size.setHeight(new_height);
                } else {
                    new_size.setHeight(min_height);
                    new_size.setY(size.bottom() - min_height);
                }
            }

            QMainWindow::setGeometry(new_size);
        }
    }
}

auto Window::mousePressEvent(QMouseEvent *event) -> void
{
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

    //    misc_bar_->show();
    //    title_bar_->set_home_button_visible(true);

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
                                "QWidget#title_bar, QWidget#bottom_bar_widget, QWidget#left_bar_widget {"
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

    const auto layout = QString{"QWidget#title_bar {"
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

    return colors + layout;
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
    title_bar_->set_home_button_visible(false);

    misc_bar_->show();
    control_bar_->hide();
    reset_account_selection();

    main_stacked_widget_->setCurrentIndex(static_cast<int>(Page::Home));
    title_bar_->show();
}

auto Window::handle_game_banner_click(riot::Game game) -> void
{
    current_game_ = game;
    update_bottom_bar_content(game);
    main_stacked_widget_->setCurrentIndex(static_cast<int>(Page::Accounts));

    title_bar_->show();
    title_bar_->set_home_button_visible(true);
    control_bar_->show();
}

auto Window::handle_table_selection_changed() -> void
{
    const bool row_is_selected = !accounts_table_->selectedItems().isEmpty();
    control_bar_->set_controls_enabled(row_is_selected);
}

auto Window::handle_login_button_click() -> void
{
    const int row = accounts_table_->currentRow();
    if (row == -1) return;

    misc_bar_->hide();
    control_bar_->hide();
    title_bar_->set_home_button_visible(false);

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

    QMainWindow::connect(button_league, &QPushButton::clicked, this, [this] { handle_game_banner_click(riot::Game::League_of_Legends); });
    QMainWindow::connect(button_valorant, &QPushButton::clicked, this, [this] { handle_game_banner_click(riot::Game::Valorant); });
    QMainWindow::connect(button_teamfight, &QPushButton::clicked, this,
                         [this] { handle_game_banner_click(riot::Game::Teamfight_Tactics); });
    QMainWindow::connect(button_runeterra, &QPushButton::clicked, this,
                         [this] { handle_game_banner_click(riot::Game::Legends_of_Runeterra); });
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

    QMainWindow::connect(accounts_table_, &QTableWidget::cellChanged, this, &Window::handle_account_cell_updated);

    accounts_layout->addWidget(accounts_table_, 1);

    QMainWindow::connect(accounts_table_, &QTableWidget::itemSelectionChanged, this, &Window::handle_table_selection_changed);

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

    control_bar_->update_game_context(game, game_icons_dir_ + icon_filename);
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
