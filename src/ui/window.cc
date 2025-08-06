// =================================================================================
// ui/window.cc
// =================================================================================

#include "ui/window.hpp"

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
    , widget_main_stacked_{new QStackedWidget{this}}
    , widget_menu_{new QWidget{}}
    , layout_menu_{new QHBoxLayout{widget_menu_}}
    , widget_accounts_content_{new QWidget{}}
    , label_accounts_{new QLabel{}}
    , table_accounts_{new QTableWidget{0, 3}}
    , widget_progress_page_{new QWidget{}}
    , label_progress_status_{new QLabel{"Initializing..."}}
    , label_progress_game_icon_{new QLabel{}}
    , button_progress_back_{new QPushButton{"back"}}
    , widget_top_bar_{new QWidget{this}}
    , button_top_bar_home_{new QPushButton{"\tback", widget_top_bar_}}
    , button_top_bar_options_{new QPushButton{"", widget_top_bar_}}
    , widget_bottom_bar_{new QWidget{this}}
    , updater_{new Updater{this}}
    , theme_config_{new core::Theme_Config{}}
    , account_config_{new core::Account_Config{}}
    , label_game_icon_placeholder_{new QLabel{widget_bottom_bar_}}
    , button_login_{new QPushButton{"Login", widget_bottom_bar_}}
    , button_add_account_{new QPushButton{"Add Account", widget_bottom_bar_}}
    , button_remove_account_{new QPushButton{"Remove Account", widget_bottom_bar_}}
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

    const auto available_geometry = QGuiApplication::primaryScreen()->availableGeometry();
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), available_geometry));

    auto *main_window_layout = new QVBoxLayout{};
    main_window_layout->setContentsMargins(0, 0, 0, 0);
    main_window_layout->setSpacing(0);

    layout_menu_->setSpacing(20);
    layout_menu_->setContentsMargins(20, 0, 20, 0);

    setup_common_ui();

    main_window_layout->addWidget(widget_top_bar_, 0, Qt::AlignTop);
    setup_home_page();
    setup_accounts_page();

    auto *progress_layout = new QVBoxLayout{widget_progress_page_};
    label_progress_game_icon_->setFixedSize(128, 128);
    label_progress_game_icon_->setAlignment(Qt::AlignCenter);
    label_progress_status_->setAlignment(Qt::AlignCenter);
    button_progress_back_->hide();
    button_progress_back_->setFixedSize(150, 30);

    progress_layout->addStretch();
    progress_layout->addWidget(label_progress_game_icon_, 0, Qt::AlignCenter);
    progress_layout->addWidget(label_progress_status_, 0, Qt::AlignCenter);
    progress_layout->addWidget(button_progress_back_, 0, Qt::AlignCenter);
    progress_layout->addStretch();

    connect(button_progress_back_, &QPushButton::clicked, this, &Window::handle_home_button_click);

    widget_main_stacked_->addWidget(widget_menu_);
    widget_main_stacked_->addWidget(widget_accounts_content_);
    widget_main_stacked_->addWidget(widget_progress_page_);

    main_window_layout->addWidget(widget_main_stacked_);
    main_window_layout->addWidget(widget_bottom_bar_, 0, Qt::AlignBottom);

    auto *central_widget = new QWidget{this};
    central_widget->setObjectName("central_widget");
    central_widget->setLayout(main_window_layout);
    setCentralWidget(central_widget);

    widget_bottom_bar_->hide();
    label_game_icon_placeholder_->hide();
    button_login_->hide();
    button_add_account_->hide();
    button_remove_account_->hide();
    widget_top_bar_->show();

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
    layout_menu_->blockSignals(true);

    const int available_content_height = height() - widget_top_bar_->height() - widget_bottom_bar_->height();
    const int available_content_width = width();
    if (available_content_width <= 0 || available_content_height <= 0) {
        layout_menu_->blockSignals(false);
        return;
    }

    const int num_banners = layout_menu_->count();
    if (num_banners == 0) {
        layout_menu_->blockSignals(false);
        return;
    }

    const int horizontal_margins = layout_menu_->contentsMargins().left() + layout_menu_->contentsMargins().right();

    int total_spacing = layout_menu_->spacing() * (num_banners - 1);
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

    for (int i = 0; i < layout_menu_->count(); ++i) {
        auto *button = qobject_cast<QPushButton *>(layout_menu_->itemAt(i)->widget());
        if (!button) continue;

        if (riot::is_game_index_out_of_range(i)) continue;
        const auto current_game = static_cast<riot::Game>(i);

        const auto original_pixmap = original_banner_pixmaps_[current_game];
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

    layout_menu_->blockSignals(false);
}

auto Window::keyPressEvent(QKeyEvent *event) -> void
{
    if (event->key() == Qt::Key_Escape && !table_accounts_->selectedItems().isEmpty()) reset_account_selection();
    return QMainWindow::keyPressEvent(event);
}

auto Window::on_login_progress_update(const QString &message) -> void
{
    label_progress_status_->setText(message);
}

auto Window::on_login_finished(bool success, const QString &message) -> void
{
    label_progress_status_->setText(message);
    button_progress_back_->show();

    if (success) {
        label_progress_status_->setStyleSheet(QString("color: %1; font-weight: bold;").arg(theme_config_->load().success.name()));
    } else {
        label_progress_status_->setStyleSheet(QString("color: %1; font-weight: bold;").arg(theme_config_->load().error.name()));
    }
}

auto Window::refresh_accounts_table() -> void
{
    table_accounts_->blockSignals(true);

    table_accounts_->setRowCount(0);
    current_accounts_ = account_config_->get_accounts();

    for (const auto &account : current_accounts_) {
        const int row = table_accounts_->rowCount();
        table_accounts_->insertRow(row);
        table_accounts_->setItem(row, 0, new QTableWidgetItem{account.note});
        table_accounts_->setItem(row, 1, new QTableWidgetItem{account.username});

        auto *password_item = new Password_Table_Widget{};
        password_item->setData(Qt::DisplayRole, QString("************"));
        password_item->setData(Qt::UserRole, account.password);
        table_accounts_->setItem(row, 2, password_item);
    }

    table_accounts_->blockSignals(false);
    handle_table_selection_changed();
}

auto Window::generate_stylesheet(const core::Theme &theme) -> QString
{
    const auto colors = QString{"QMainWindow, QDialog, QWidget#central_widget, QWidget#widget_menu, "
                                "QWidget#widget_accounts_content, "
                                "QWidget#widget_progress_page {"
                                "    background-color: %1;"
                                "    color: %2;"
                                "}"
                                "QWidget#widget_top_bar, QWidget#widget_bottom_bar {"
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
                                "QPushButton#bannerButton {"
                                "    background-color: transparent;"
                                "    border-color: %9;"
                                "}"
                                "QPushButton#bannerButton:hover {"
                                "    border-color: %10;"
                                "}"
                                "QPushButton#homeButton, QPushButton#optionsButton {"
                                "    background-color: transparent;"
                                "    border-color: transparent;"
                                "}"
                                "QPushButton#homeButton:hover, QPushButton#optionsButton:hover {"
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
                                "QLabel#gameIconPlaceholder {"
                                "    background-color: transparent;"
                                "    border-color: transparent;"
                                "}"}
                            .arg(theme.background_dark.name(), theme.text_primary.name(), theme.border.name(), theme.button_primary.name(),
                                 theme.button_hover.name(), theme.button_disabled.name(), theme.text_disabled.name(),
                                 theme.background_light.name(), theme.accent.name(), theme.accent_hover.name());

    const auto layout = QString{"QWidget#widget_top_bar, QWidget#widget_bottom_bar {"
                                "    border-style: solid;"
                                "    border-top-width: 1px;"
                                "    border-bottom-width: 1px;"
                                "    border-left-width: 0px;"
                                "    border-right-width: 0px;"
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
                                "QPushButton#bannerButton {"
                                "    border-width: 3px;"
                                "    padding: 0px;"
                                "}"
                                "QPushButton#homeButton, QPushButton#optionsButton {"
                                "    border: none;"
                                "    padding: 2px;"
                                "}"
                                "QPushButton#optionsButton::menu-indicator {"
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
                                "QLabel#gameIconPlaceholder {"
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
    Theme_Editor editor(theme, this);

    if (editor.exec() == QDialog::Accepted) {
        if (theme_config_->save(theme)) {
            apply_theme();
        } else {
            QMessageBox::critical(this, "Theme Error", "Failed to save the updated theme.");
        }
    }
}

auto Window::handle_home_button_click() -> void
{
    auto *layout = qobject_cast<QHBoxLayout *>(widget_top_bar_->layout());
    layout->removeWidget(button_top_bar_options_);
    layout->insertWidget(0, button_top_bar_options_, 0, Qt::AlignLeft);

    button_top_bar_home_->hide();
    widget_bottom_bar_->hide();
    reset_account_selection();

    widget_main_stacked_->setCurrentIndex(static_cast<int>(Page::Home));
    widget_top_bar_->show();
}

auto Window::handle_game_banner_click(riot::Game game) -> void
{
    current_game_ = game;

    update_bottom_bar_content(game);
    widget_main_stacked_->setCurrentIndex(static_cast<int>(Page::Accounts));

    auto *layout = qobject_cast<QHBoxLayout *>(widget_top_bar_->layout());
    layout->removeWidget(button_top_bar_options_);
    layout->addWidget(button_top_bar_options_, 0, Qt::AlignRight);

    widget_top_bar_->show();
    button_top_bar_home_->show();
    widget_bottom_bar_->show();
}

auto Window::handle_table_selection_changed() -> void
{
    const bool row_is_selected = !table_accounts_->selectedItems().isEmpty();
    button_login_->setEnabled(row_is_selected);
    button_remove_account_->setEnabled(row_is_selected);
}

auto Window::handle_login_button_click() -> void
{
    const int row = table_accounts_->currentRow();
    if (row == -1) return;

    widget_top_bar_->hide();
    widget_bottom_bar_->hide();

    label_progress_status_->setText("Initializing...");
    button_progress_back_->hide();

    QString icon_filename;
    switch (current_game_) {
    case riot::Game::League_of_Legends: icon_filename = "league-icon.png"; break;
    case riot::Game::Valorant: icon_filename = "valorant-icon.png"; break;
    case riot::Game::Teamfight_Tactics: icon_filename = "teamfight-icon.png"; break;
    case riot::Game::Legends_of_Runeterra: icon_filename = "runeterra-icon.png"; break;
    }

    if (!icon_filename.isEmpty()) {
        const auto game_icon_pixmap = QPixmap(game_icons_dir_ + icon_filename);
        label_progress_game_icon_->setPixmap(game_icon_pixmap.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    widget_main_stacked_->setCurrentIndex(static_cast<int>(Page::Progress));

    const auto username = table_accounts_->item(row, 1)->text();
    const auto password = table_accounts_->item(row, 2)->data(Qt::UserRole).toString();

    reset_account_selection();
    emit start_login(current_game_, username, password);
}

auto Window::handle_add_account_button_click() -> void
{
    Add_Account_Dialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        core::Account new_account;
        new_account.note = dialog.get_note();
        new_account.username = dialog.get_username();
        new_account.password = dialog.get_password();
        if (new_account.username.isEmpty() || new_account.password.isEmpty()) {
            QMessageBox::critical(this, "Add Account", "Username and password cannot be empty.");
            return;
        }

        if (account_config_->add_account(new_account)) {
            refresh_accounts_table();
            table_accounts_->setCurrentCell(table_accounts_->rowCount() - 1, 0);
        } else {
            QMessageBox::critical(this, "Add Account", "Failed to save the new account.");
        }
    }
}

auto Window::handle_remove_account_button_click() -> void
{
    const int current_row = table_accounts_->currentRow();
    if (current_row == -1) {
        QMessageBox::warning(this, "Delete Account", "Please select an account to delete.");
        return;
    }

    const auto &account_to_delete = current_accounts_[current_row];
    const auto confirmation = QString{"Are you sure you want to delete '%1'?"}.arg(account_to_delete.username);
    const auto reply = QMessageBox::warning(this, "Confirm Deletion", confirmation, QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::No) return;

    if (account_config_->remove_account(current_row)) {
        refresh_accounts_table();
    } else {
        QMessageBox::critical(this, "Deletion Error", "Failed to remove the account from the configuration file.");
    }
}

auto Window::handle_account_cell_updated(int row, int column) -> void
{
    if (row < 0 || row >= current_accounts_.size()) return;
    auto updated_account = current_accounts_[row];

    auto *item = table_accounts_->item(row, column);
    if (!item) return;

    const QString new_value = item->text();
    switch (column) {
    case 0: updated_account.note = new_value; break;
    case 1: updated_account.username = new_value; break;
    case 2:
        updated_account.password = new_value;
        table_accounts_->blockSignals(true);
        item->setData(Qt::UserRole, new_value);
        item->setText("************");
        table_accounts_->blockSignals(false);
        break;
    }

    if (!account_config_->update_account(row, updated_account)) {
        QMessageBox::critical(this, "Save Error", "Failed to save changes to the account.");
        refresh_accounts_table();
    } else {
        current_accounts_[row] = updated_account;
    }
}

auto Window::setup_common_ui() -> void
{
    widget_top_bar_->setObjectName("widget_top_bar");
    widget_top_bar_->setFixedHeight(40);

    auto *top_bar_layout = new QHBoxLayout(widget_top_bar_);
    top_bar_layout->setContentsMargins(5, 0, 5, 0);
    top_bar_layout->setSpacing(5);

    button_top_bar_home_->setObjectName("homeButton");
    button_top_bar_home_->setIcon(QIcon::fromTheme("document-revert"));

    button_top_bar_options_->setObjectName("optionsButton");
    button_top_bar_options_->setIcon(QIcon::fromTheme("document-properties"));

    auto *options_menu = new QMenu(button_top_bar_options_);
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

    button_top_bar_options_->setMenu(options_menu);
    top_bar_layout->addWidget(button_top_bar_options_, 0, Qt::AlignLeft);
    top_bar_layout->addWidget(button_top_bar_home_, 0, Qt::AlignLeft);
    top_bar_layout->addStretch();
    button_top_bar_home_->hide();
    QObject::connect(button_top_bar_home_, &QPushButton::clicked, this, &Window::handle_home_button_click);

    handle_home_button_click();
    widget_bottom_bar_->setObjectName("widget_bottom_bar");
    widget_bottom_bar_->setFixedHeight(50);

    auto *bottom_bar_layout = new QHBoxLayout{widget_bottom_bar_};
    bottom_bar_layout->setContentsMargins(10, 0, 15, 0);
    bottom_bar_layout->setSpacing(10);

    bottom_bar_layout->addWidget(button_login_);
    bottom_bar_layout->addWidget(button_add_account_);
    bottom_bar_layout->addWidget(button_remove_account_);
    bottom_bar_layout->addStretch();

    label_game_icon_placeholder_->setObjectName("gameIconPlaceholder");
    label_game_icon_placeholder_->setFixedSize(32, 32);
    label_game_icon_placeholder_->setContentsMargins(0, 0, 0, 0);

    auto *game_info_layout = new QHBoxLayout{};
    game_info_layout->addWidget(label_game_icon_placeholder_);
    game_info_layout->setContentsMargins(0, 0, 0, 0);
    game_info_layout->setSpacing(5);
    bottom_bar_layout->addLayout(game_info_layout);

    QObject::connect(button_login_, &QPushButton::clicked, this, &Window::handle_login_button_click);
    QObject::connect(button_add_account_, &QPushButton::clicked, this, &Window::handle_add_account_button_click);
    QObject::connect(button_remove_account_, &QPushButton::clicked, this, &Window::handle_remove_account_button_click);
}

auto Window::setup_home_page() -> void
{
    QPushButton *button_league = create_banner_button("league.jpg", riot::Game::League_of_Legends);
    QPushButton *button_valorant = create_banner_button("valorant.jpg", riot::Game::Valorant);
    QPushButton *button_teamfight = create_banner_button("tft.jpg", riot::Game::Teamfight_Tactics);
    QPushButton *button_runeterra = create_banner_button("runeterra.jpg", riot::Game::Legends_of_Runeterra);

    layout_menu_->addWidget(button_league);
    layout_menu_->addWidget(button_valorant);
    layout_menu_->addWidget(button_teamfight);
    layout_menu_->addWidget(button_runeterra);

    QObject::connect(button_league, &QPushButton::clicked, this, [this] { handle_game_banner_click(riot::Game::League_of_Legends); });
    QObject::connect(button_valorant, &QPushButton::clicked, this, [this] { handle_game_banner_click(riot::Game::Valorant); });
    QObject::connect(button_teamfight, &QPushButton::clicked, this, [this] { handle_game_banner_click(riot::Game::Teamfight_Tactics); });
    QObject::connect(button_runeterra, &QPushButton::clicked, this, [this] { handle_game_banner_click(riot::Game::Legends_of_Runeterra); });
}

auto Window::setup_accounts_page() -> void
{
    auto *accounts_layout = new QVBoxLayout{widget_accounts_content_};
    table_accounts_->setHorizontalHeaderLabels({"Note", "Username", "Password"});
    table_accounts_->horizontalHeader()->setStretchLastSection(true);
    table_accounts_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table_accounts_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_accounts_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_accounts_->setEditTriggers(QAbstractItemView::DoubleClicked);
    table_accounts_->verticalHeader()->setVisible(false);

    QObject::connect(table_accounts_, &QTableWidget::cellChanged, this, &Window::handle_account_cell_updated);
    accounts_layout->addWidget(label_accounts_);
    accounts_layout->addWidget(table_accounts_);

    QObject::connect(table_accounts_, &QTableWidget::itemSelectionChanged, this, &Window::handle_table_selection_changed);

    refresh_accounts_table();
}

auto Window::reset_account_selection() -> void
{
    table_accounts_->setCurrentCell(-1, -1);
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
    label_game_icon_placeholder_->setPixmap(game_icon.pixmap(QSize(32, 32)));
    label_game_icon_placeholder_->show();

    button_login_->show();
    button_add_account_->show();
    button_remove_account_->show();
}

auto Window::create_banner_button(const QString &image_path, riot::Game game) -> QPushButton *
{
    auto *button = new QPushButton("", widget_menu_);
    button->setObjectName("bannerButton");

    const QPixmap original_pixmap(banners_dir_ + image_path);
    original_banner_pixmaps_[game] = original_pixmap;
    button->setIcon(QIcon(original_pixmap));
    return button;
}

} // namespace ui
