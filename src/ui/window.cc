// =================================================================================
// window.cc
// =================================================================================

#include "ui/window.hpp"

#include "ui/add_account_dialog.hpp"
#include "ui/password_table_widget.hpp"

#include <QCoreApplication>
#include <QDebug>
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

static auto get_config_path() -> QString {
    const QString appdata_path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (appdata_path.isEmpty()) {
        QMessageBox::critical(nullptr, "Configuration Error", "Failed to get AppData directory path. Cannot initialize configuration.");
        return QString{};
    }

    const auto appdata_dir = QDir{appdata_path};
    if (!appdata_dir.exists() && !appdata_dir.mkpath(".")) {
        QMessageBox::critical(nullptr, "Configuration Error", "Failed to create configuration directory: " + appdata_path);
        return QString{};
    }

    const QString config_file_path = appdata_dir.absoluteFilePath("configuration.toml");
    if (QFile::exists(config_file_path)) return config_file_path;

    auto config_file = QFile{config_file_path};
    if (!config_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(nullptr, "Configuration Error", "failed to create config: " + config_file_path);
        return QString{};
    }

    auto stream = QTextStream{&config_file};
    stream << "# Friede Configuration File\n";
    stream << "# This file stores account information and application settings\n\n";
    stream << "[[accounts]]\n";
    stream << "# Example account entry:\n";
    stream << "# note = \"Main Account\"\n";
    stream << "# username = \"your_username\"\n";
    stream << "# password = \"your_password\"\n\n";

    config_file.close();
    return config_file_path;
}

Window::Window(QWidget *parent)
    : QMainWindow(parent)
    , widget_main_stacked(new QStackedWidget(this))
    , widget_menu(new QWidget())
    , layout_menu(new QHBoxLayout(widget_menu))
    , widget_accounts_content(new QWidget())
    , label_accounts(new QLabel())
    , table_accounts(new QTableWidget(0, 3))
    , widget_progress_page(new QWidget())
    , label_progress_status(new QLabel("Initializing..."))
    , label_progress_game_icon(new QLabel())
    , button_progress_back(new QPushButton("back"))
    , widget_top_bar(new QWidget(this))
    , button_top_bar_home(new QPushButton("\tback", widget_top_bar))
    , button_top_bar_options(new QPushButton("", widget_top_bar))
    , widget_bottom_bar(new QWidget(this))
    , label_game_icon_placeholder(new QLabel(widget_bottom_bar))
    , button_login(new QPushButton("Login", widget_bottom_bar))
    , button_add_account(new QPushButton("Add Account", widget_bottom_bar))
    , button_remove_account(new QPushButton("Remove Account", widget_bottom_bar))
    , banners_dir(QCoreApplication::applicationDirPath() + "/banners/")
    , game_icons_dir(QCoreApplication::applicationDirPath() + "/icons/")
    , config_path{get_config_path()}
    , config(config_path.isEmpty() ? toml::parse_result{} : toml::parse_file(config_path.toStdString())) {

    //
    // login worker
    //

    auto *worker = new Login_Worker;
    worker->moveToThread(&worker_thread);

    connect(&worker_thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &Window::start_login, worker, &Login_Worker::do_login);
    connect(worker, &Login_Worker::progress_updated, this, &Window::on_login_progress_update);
    connect(worker, &Login_Worker::login_finished, this, &Window::on_login_finished);

    worker_thread.start();

    //
    // window setup
    //

    setMinimumSize(750, 450);
    setWindowTitle("a flame alighteth");

    setWindowIcon(QIcon(QCoreApplication::applicationDirPath() + "/icons/app-icon.png"));

    const auto available_geometry = QGuiApplication::primaryScreen()->availableGeometry();
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), available_geometry));

    auto *main_window_layout = new QVBoxLayout;
    main_window_layout->setContentsMargins(0, 0, 0, 0);
    main_window_layout->setSpacing(0);

    layout_menu->setSpacing(20);
    layout_menu->setContentsMargins(20, 0, 20, 0);

    setup_common_ui();

    main_window_layout->addWidget(widget_top_bar, 0, Qt::AlignTop);
    setup_home_page();
    setup_accounts_page();

    //
    // progress page ui
    //

    auto *progress_layout = new QVBoxLayout(widget_progress_page);
    label_progress_game_icon->setFixedSize(128, 128);
    label_progress_game_icon->setAlignment(Qt::AlignCenter);
    label_progress_status->setAlignment(Qt::AlignCenter);
    button_progress_back->hide();
    button_progress_back->setFixedSize(150, 30);

    progress_layout->addStretch();
    progress_layout->addWidget(label_progress_game_icon, 0, Qt::AlignCenter);
    progress_layout->addWidget(label_progress_status, 0, Qt::AlignCenter);
    progress_layout->addWidget(button_progress_back, 0, Qt::AlignCenter);
    progress_layout->addStretch();

    // for now, lets return to the home screen after login
    connect(button_progress_back, &QPushButton::clicked, this, &Window::handle_home_button_click);

    //
    // setup window widgets
    //

    widget_main_stacked->addWidget(widget_menu);
    widget_main_stacked->addWidget(widget_accounts_content);
    widget_main_stacked->addWidget(widget_progress_page);

    main_window_layout->addWidget(widget_main_stacked);
    main_window_layout->addWidget(widget_bottom_bar, 0, Qt::AlignBottom);

    auto *central_widget = new QWidget{this};
    central_widget->setLayout(main_window_layout);
    setCentralWidget(central_widget);

    widget_bottom_bar->hide();
    label_game_icon_placeholder->hide();
    button_login->hide();
    button_add_account->hide();
    button_remove_account->hide();
    widget_top_bar->show();

    updater = new Updater{this};
    updater->check_for_updates();
}

Window::~Window() {
    worker_thread.quit();
    worker_thread.wait();
}

void Window::on_login_progress_update(const QString &message) { label_progress_status->setText(message); }

void Window::on_login_finished(bool success, const QString &message) {
    label_progress_status->setText(message);
    button_progress_back->show();

    if (success) {
        label_progress_status->setStyleSheet("color: #2ecc71; font-weight: bold;");
    } else {
        label_progress_status->setStyleSheet("color: #e74c3c; font-weight: bold;");
    }
}

void Window::on_check_for_updates_clicked() { updater->check_for_updates(); }

auto Window::handle_login_button_click() -> void {
    const int row = table_accounts->currentRow();
    if (row == -1) return;

    widget_top_bar->hide();
    widget_bottom_bar->hide();

    label_progress_status->setText("Initializing...");
    label_progress_status->setStyleSheet("");
    button_progress_back->hide();

    QString icon_filename;
    switch (current_game) {
    case riot::Game::League_of_Legends: icon_filename = "league-icon.png"; break;
    case riot::Game::Valorant: icon_filename = "valorant-icon.png"; break;
    case riot::Game::Teamfight_Tactics: icon_filename = "teamfight-icon.png"; break;
    case riot::Game::Legends_of_Runeterra: icon_filename = "runeterra-icon.png"; break;
    }

    if (!icon_filename.isEmpty()) {
        const auto game_icon_pixmap = QPixmap(game_icons_dir + icon_filename);
        label_progress_game_icon->setPixmap(game_icon_pixmap.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    widget_main_stacked->setCurrentIndex(static_cast<int>(Page::Progress));

    const auto username = table_accounts->item(row, 1)->text();
    const auto password = table_accounts->item(row, 2)->data(Qt::UserRole).toString();

    reset_account_selection();
    emit start_login(current_game, username, password);
}

auto Window::create_banner_button(const QString &image_path, riot::Game game) -> QPushButton * {
    auto *button = new QPushButton("", widget_menu);
    const QPixmap original_pixmap(banners_dir + image_path);
    original_banner_pixmaps[game] = original_pixmap;
    button->setIcon(QIcon(original_pixmap));
    button->setStyleSheet("QPushButton { border: 3px solid #5a5a5a; padding: 0px; background-color: transparent; outline: none; } "
                          "QPushButton:hover { border: 3px solid #9a9a9a; }");
    return button;
}

auto Window::resizeEvent(QResizeEvent *event) -> void {
    QMainWindow::resizeEvent(event);
    layout_menu->blockSignals(true);

    const int available_content_height = height() - widget_top_bar->height() - widget_bottom_bar->height();
    const int available_content_width = width();
    if (available_content_width <= 0 || available_content_height <= 0) {
        layout_menu->blockSignals(false);
        return;
    }

    const int num_banners = layout_menu->count();
    if (num_banners == 0) {
        layout_menu->blockSignals(false);
        return;
    }

    const int horizontal_margins = layout_menu->contentsMargins().left() + layout_menu->contentsMargins().right();

    int total_spacing = layout_menu->spacing() * (num_banners - 1);
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

    const int IMAGE_BORDER_PADDING = 2;
    for (int i = 0; i < layout_menu->count(); ++i) {
        auto *button = qobject_cast<QPushButton *>(layout_menu->itemAt(i)->widget());
        if (button) {
            riot::Game current_game;
            switch (i) {
            case 0: current_game = riot::Game::League_of_Legends; break;
            case 1: current_game = riot::Game::Valorant; break;
            case 2: current_game = riot::Game::Teamfight_Tactics; break;
            case 3: current_game = riot::Game::Legends_of_Runeterra; break;
            default: continue;
            }

            const auto original_pixmap = original_banner_pixmaps[current_game];
            if (!original_pixmap.isNull()) {
                const auto banner_width = desired_banner_width;
                const auto banner_height = desired_banner_height;
                const auto scaled_pixmap =
                    original_pixmap.scaled(banner_width, banner_height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                button->setIcon(QIcon{scaled_pixmap});

                const auto image_display_size = scaled_pixmap.size();
                button->setIconSize(image_display_size);

                const auto button_width = image_display_size.width() + (2 * IMAGE_BORDER_PADDING);
                const auto button_height = image_display_size.height() + (2 * IMAGE_BORDER_PADDING);
                button->setFixedSize(QSize{button_width, button_height});
            }
        }
    }

    layout_menu->blockSignals(false);
}

auto Window::keyPressEvent(QKeyEvent *event) -> void {
    if (event->key() == Qt::Key_Escape && !table_accounts->selectedItems().isEmpty()) reset_account_selection();
    return QMainWindow::keyPressEvent(event);
}

auto Window::setup_common_ui() -> void {
    widget_top_bar->setStyleSheet("background-color: #1c1c1c; border-bottom: 1px solid #d0d0d0; padding: 5px;");
    widget_top_bar->setFixedHeight(40);

    auto *top_bar_layout = new QHBoxLayout(widget_top_bar);
    top_bar_layout->setContentsMargins(5, 0, 5, 0);
    top_bar_layout->setSpacing(5);

    button_top_bar_home->setIcon(QIcon::fromTheme("document-revert"));
    button_top_bar_home->setStyleSheet("QPushButton { border: none; padding: 2px; background-color: transparent; outline: none; } "
                                       "QPushButton:hover { background-color: #3a3a3b; }");

    button_top_bar_options->setIcon(QIcon::fromTheme("document-properties"));
    button_top_bar_options->setStyleSheet(
        "QPushButton { border: none; padding: 2px; background-color: transparent; outline: none; } QPushButton:hover { background-color: "
        "#3a3a3b; } QPushButton::menu-indicator { image: none; width: 0px; }");

    auto *options_menu = new QMenu(button_top_bar_options);
    options_menu->setStyleSheet("QMenu { background-color: #2b2b2b; color: white; border: 1px solid #555; } QMenu::item { padding: 4px "
                                "20px; } QMenu::item:selected { background-color: #404040; }");

    auto *action_check_for_updates = options_menu->addAction("check for updates");
    action_check_for_updates->setIcon(QIcon::fromTheme("emblem-synchronized"));
    connect(action_check_for_updates, &QAction::triggered, this, &Window::on_check_for_updates_clicked);

    auto *action_open_directory = options_menu->addAction("open config directory");
    action_open_directory->setIcon(QIcon::fromTheme("folder-open"));

    // FIXME this is really slow in release build for some reason? the terminal opens for 5 seconds with no background, then it opens
    connect(action_open_directory, &QAction::triggered, this, [] { std::system("explorer %appdata%\\friede"); });

    options_menu->addSeparator();

    auto *action_close = options_menu->addAction("close");

    action_close->setIcon(QIcon::fromTheme("window-close"));

    button_top_bar_options->setMenu(options_menu);

    top_bar_layout->addWidget(button_top_bar_options, 0, Qt::AlignLeft);
    top_bar_layout->addWidget(button_top_bar_home, 0, Qt::AlignLeft);
    top_bar_layout->addStretch();
    button_top_bar_home->hide();
    QObject::connect(button_top_bar_home, &QPushButton::clicked, this, [this] { handle_home_button_click(); });

    handle_home_button_click();

    widget_bottom_bar->setFixedHeight(50);

    QHBoxLayout *bottom_bar_layout = new QHBoxLayout(widget_bottom_bar);
    bottom_bar_layout->setContentsMargins(10, 0, 15, 0);
    bottom_bar_layout->setSpacing(10);
    button_login->setStyleSheet("QPushButton { border: 1px solid #5a5a5a; padding: 5px 15px; background-color: #3a3a3a; color: white; "
                                "border-radius: 5px; outline: none; } QPushButton:hover { background-color: #4a4a4a; } "
                                "QPushButton:disabled { background-color: #2a2a2a; color: #8a8a8a; border: 1px solid #4a4a4a; }");

    button_add_account->setStyleSheet("QPushButton { border: 1px solid #5a5a5a; padding: 5px 15px; background-color: #3a3a3a; color: "
                                      "white; border-radius: 5px; outline: none; } QPushButton:hover { background-color: #4a4a4a; } "
                                      "QPushButton:disabled { background-color: #2a2a2a; color: #8a8a8a; border: 1px solid #4a4a4a; }");

    bottom_bar_layout->addWidget(button_login);
    bottom_bar_layout->addWidget(button_add_account);
    button_remove_account->setStyleSheet(
        "QPushButton { border: 1px solid #5a5a5a; padding: 5px 15px; background-color: #3a3a3a; color: white; border-radius: 5px; outline: "
        "none; } QPushButton:hover { background-color: #4a4a4a; } QPushButton:pressed { background-color: #c0392b; } QPushButton:disabled "
        "{ background-color: #2a2a2a; color: #8a8a8a; border: 1px solid #4a4a4a; }");

    bottom_bar_layout->addWidget(button_login);
    bottom_bar_layout->addWidget(button_add_account);
    bottom_bar_layout->addWidget(button_remove_account);
    bottom_bar_layout->addStretch();

    label_game_icon_placeholder->setFixedSize(32, 32);
    label_game_icon_placeholder->setStyleSheet("QLabel { border: none; background-color: transparent; }");
    label_game_icon_placeholder->setContentsMargins(0, 0, 0, 0);

    auto *game_info_layout = new QHBoxLayout;
    game_info_layout->addWidget(label_game_icon_placeholder);
    game_info_layout->setContentsMargins(0, 0, 0, 0);
    game_info_layout->setSpacing(5);
    bottom_bar_layout->addLayout(game_info_layout);

    QObject::connect(button_login, &QPushButton::clicked, this, [this] { handle_login_button_click(); });
    QObject::connect(button_add_account, &QPushButton::clicked, this, [this] { handle_add_account_button_click(); });
    QObject::connect(button_remove_account, &QPushButton::clicked, this, [this] { handle_remove_account_button_click(); });
}

auto Window::setup_home_page() -> void {
    QPushButton *button_league = create_banner_button("league.jpg", riot::Game::League_of_Legends);
    QPushButton *button_valorant = create_banner_button("valorant.jpg", riot::Game::Valorant);
    QPushButton *button_teamfight = create_banner_button("tft.jpg", riot::Game::Teamfight_Tactics);
    QPushButton *button_runeterra = create_banner_button("runeterra.jpg", riot::Game::Legends_of_Runeterra);

    layout_menu->addWidget(button_league);
    layout_menu->addWidget(button_valorant);
    layout_menu->addWidget(button_teamfight);
    layout_menu->addWidget(button_runeterra);

    QObject::connect(button_league, &QPushButton::clicked, this, [this] { handle_game_banner_click(riot::Game::League_of_Legends); });
    QObject::connect(button_valorant, &QPushButton::clicked, this, [this] { handle_game_banner_click(riot::Game::Valorant); });
    QObject::connect(button_teamfight, &QPushButton::clicked, this, [this] { handle_game_banner_click(riot::Game::Teamfight_Tactics); });
    QObject::connect(button_runeterra, &QPushButton::clicked, this, [this] { handle_game_banner_click(riot::Game::Legends_of_Runeterra); });
}

auto Window::setup_accounts_page() -> void {
    if (!config) {
        QMessageBox::critical(this, "Configuration Error", "Failed to parse configuration.toml");
        return;
    }

    auto *accounts_layout = new QVBoxLayout{widget_accounts_content};
    table_accounts->setHorizontalHeaderLabels({"Note", "Username", "Password"});
    table_accounts->horizontalHeader()->setStretchLastSection(true);
    table_accounts->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table_accounts->setSelectionMode(QAbstractItemView::SingleSelection);
    table_accounts->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_accounts->setEditTriggers(QAbstractItemView::DoubleClicked);
    table_accounts->verticalHeader()->setVisible(false);

    const auto accounts = config["accounts"];
    if (accounts && accounts.is_array()) {
        for (const auto &account : *accounts.as_array()) {
            if (!account.is_table()) continue;

            const auto table = account.as_table();
            const auto note = table->get("note") ? table->get("note")->value_or(""sv) : ""sv;
            const auto username = table->get("username") ? table->get("username")->value_or(""sv) : ""sv;
            const auto password = table->get("password") ? table->get("password")->value_or(""sv) : ""sv;
            if (username.empty() || password.empty()) continue;

            const int row = table_accounts->rowCount();
            table_accounts->insertRow(row);
            table_accounts->setItem(row, 0, new QTableWidgetItem{note.data()});
            table_accounts->setItem(row, 1, new QTableWidgetItem{username.data()});

            auto *password_item = new Password_Table_Widget;
            password_item->setData(Qt::DisplayRole, QString("************"));
            password_item->setData(Qt::UserRole, QString(password.data()));

            table_accounts->setItem(row, 2, password_item);
        }
    }

    QObject::connect(table_accounts, &QTableWidget::cellChanged, this, &Window::handle_account_cell_updated);
    accounts_layout->addWidget(label_accounts);
    accounts_layout->addWidget(table_accounts);

    QObject::connect(table_accounts, &QTableWidget::itemSelectionChanged, this, &Window::handle_table_selection_changed);
    handle_table_selection_changed();
}

auto Window::handle_game_banner_click(riot::Game game) -> void {
    current_game = game;

    update_bottom_bar_content(game);
    widget_main_stacked->setCurrentIndex(static_cast<int>(Page::Accounts));

    auto *layout = qobject_cast<QHBoxLayout *>(widget_top_bar->layout());
    layout->removeWidget(button_top_bar_options);
    layout->addWidget(button_top_bar_options, 0, Qt::AlignRight);

    widget_top_bar->show();
    button_top_bar_home->show();
    widget_bottom_bar->show();
}

auto Window::handle_home_button_click() -> void {
    auto *layout = qobject_cast<QHBoxLayout *>(widget_top_bar->layout());
    layout->removeWidget(button_top_bar_options);
    layout->insertWidget(0, button_top_bar_options, 0, Qt::AlignLeft);

    button_top_bar_home->hide();
    widget_bottom_bar->hide();
    reset_account_selection();

    widget_main_stacked->setCurrentIndex(static_cast<int>(Page::Home));
    widget_top_bar->show();
}

auto Window::handle_table_selection_changed() -> void {
    const bool row_is_selected = !table_accounts->selectedItems().isEmpty();
    button_login->setEnabled(row_is_selected);
    button_remove_account->setEnabled(row_is_selected);
}

auto Window::save_config_to_file() -> bool {
    if (!config) {
        QMessageBox::critical(this, "Save Error", "Configuration object is invalid, cannot save.");
        return false;
    }

    try {
        auto ofs = std::ofstream{config_path.toStdString()};
        if (!ofs.is_open()) {
            QMessageBox::critical(this, "Save Error", "Failed to open configuration for writing: " + config_path);
            return false;
        }

        ofs << config;
        ofs.close();
    } catch (const std::exception &e) {
        QMessageBox::critical(this, "Save Error", "An exception occurred during configuration write: " + QString(e.what()));
        return false;
    }

    return true;
}

auto Window::handle_add_account_button_click() -> void {
    Add_Account_Dialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        const auto note = dialog.get_note();
        const auto username = dialog.get_username();
        const auto password = dialog.get_password();
        if (username == "" || password == "") {
            QMessageBox::critical(this, "Add Account", "Failed to add account.\nEither the username or the password was empty.");
            return;
        }

        add_account_to_config(note, username, password);
    }
}

auto Window::handle_remove_account_button_click() -> void {
    QList<QTableWidgetItem *> selected_items = table_accounts->selectedItems();
    if (selected_items.isEmpty()) {
        QMessageBox::critical(this, "Delete Account", "Please select an account to delete.");
        return;
    }

    const int current_row = selected_items.first()->row();
    auto account_to_delete_name = QString{"the selected account"};
    if (table_accounts->item(current_row, 1)) { account_to_delete_name = table_accounts->item(current_row, 1)->text(); }

    const auto reply_flags = QMessageBox::Yes | QMessageBox::No;
    const auto confirmation = QString{"Are you sure you want to delete '%1'?"}.arg(account_to_delete_name);
    const auto reply = QMessageBox::warning(this, "Confirm Deletion", confirmation, reply_flags);
    if (reply == QMessageBox::No) return;

    auto *accounts_array = config.table().get_as<toml::array>("accounts");
    if (!accounts_array) {
        QMessageBox::critical(this, "Deletion Error", "TOML config does not contain 'accounts', cannot delete.");
        return;
    }

    if (current_row < 0 || static_cast<size_t>(current_row) >= accounts_array->size()) {
        QMessageBox::critical(this, "Deletion Error", "Invalid row index for deletion in TOML: " + QString::number(current_row));
        return;
    }

    accounts_array->erase(accounts_array->begin() + current_row);
    if (!save_config_to_file()) {
        QMessageBox::critical(this, "Save Error", "Failed to save configuration after account deletion.");
        return;
    }

    table_accounts->blockSignals(true);
    table_accounts->removeRow(current_row);
    table_accounts->blockSignals(false);

    reset_account_selection();
}

auto Window::save_account_data(int row, int column, const QString &new_value) -> void {
    auto *accounts_array = config.table().get_as<toml::array>("accounts");
    if (!accounts_array) {
        QMessageBox::critical(this, "Save Error", "TOML config does not contain 'accounts'.");
        return;
    }

    if (row < 0 || static_cast<size_t>(row) >= accounts_array->size()) {
        QMessageBox::critical(this, "Save Error", "Invalid row bounds for saving: " + QString::number(row));
        return;
    }

    auto *table = accounts_array->at(row).as_table();
    if (!table) {
        QMessageBox::critical(this, "Save Error", "Account data at row " + QString::number(row) + " is not a valid table structure.");
        return;
    }

    auto key_to_update = "";
    if (column == 0) key_to_update = "note";
    if (column == 1) key_to_update = "username";
    if (column == 2) key_to_update = "password";

    auto value_to_write = new_value.toStdString();
    table->insert_or_assign(key_to_update, value_to_write);

    save_config_to_file();
}

auto Window::add_account_to_config(const QString &note, const QString &username, const QString &password) -> void {
    if (!config) {
        QMessageBox::critical(this, "Configuration Error", "TOML config is not valid, cannot add account.");
        return;
    }

    auto *accounts_array = config.table().get_as<toml::array>("accounts");
    if (!accounts_array) {
        config.table().insert("accounts", toml::array{});

        accounts_array = config.table().get_as<toml::array>("accounts");
        if (!accounts_array) {
            QMessageBox::critical(this, "Configuration Error", "Failed to create 'accounts' array in TOML config.");
            return;
        }
    }

    toml::table new_toml_table;
    new_toml_table.insert("note", note.toStdString());
    new_toml_table.insert("username", username.toStdString());
    new_toml_table.insert("password", password.toStdString());
    accounts_array->push_back(new_toml_table);

    save_config_to_file();
    table_accounts->blockSignals(true);

    const int new_qt_row = table_accounts->rowCount();
    table_accounts->insertRow(new_qt_row);

    auto *note_item = new QTableWidgetItem{note};
    table_accounts->setItem(new_qt_row, 0, note_item);

    auto *username_item = new QTableWidgetItem{username};
    table_accounts->setItem(new_qt_row, 1, username_item);

    auto *password_item = new Password_Table_Widget;
    password_item->setData(Qt::DisplayRole, QString("************"));
    password_item->setData(Qt::UserRole, password);

    table_accounts->setItem(new_qt_row, 2, password_item);
    table_accounts->blockSignals(false);
    table_accounts->setCurrentCell(new_qt_row, 0);
}

auto Window::handle_account_cell_updated(int row, int column) -> void {
    table_accounts->blockSignals(true);

    if (auto *item = table_accounts->item(row, column)) {
        const auto text = item->text();

        if (column == 2) {
            item->setData(Qt::UserRole, text);
            item->setText("************");
        }

        save_account_data(row, column, text);
    }

    table_accounts->blockSignals(false);
}

auto Window::reset_account_selection() -> void { table_accounts->setCurrentCell(-1, -1); }

auto Window::update_bottom_bar_content(riot::Game game) -> void {
    QString icon_filename;

    switch (game) {
    case riot::Game::Valorant: icon_filename = "valorant-icon.png"; break;
    case riot::Game::League_of_Legends: icon_filename = "league-icon.png"; break;
    case riot::Game::Teamfight_Tactics: icon_filename = "teamfight-icon.png"; break;
    case riot::Game::Legends_of_Runeterra: icon_filename = "runeterra-icon.png"; break;
    }

    const auto game_icon = QIcon{game_icons_dir + icon_filename};
    label_game_icon_placeholder->setPixmap(game_icon.pixmap(QSize(32, 32)));
    label_game_icon_placeholder->show();

    button_login->show();
    button_add_account->show();
    button_remove_account->show();
}

} // namespace ui
