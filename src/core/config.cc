// =================================================================================
// core/config.cc
// =================================================================================

#include "config.hpp"

#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QStandardPaths>
#include <QTextStream>

#include <fstream>

using namespace std::literals;

namespace core {

Config::Config(QObject *parent)
    : QObject{parent}
    , config_path_{initialize_config_path()} {}

auto Config::get_config_directory_path() const -> QString { return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation); }

auto Config::initialize_config_path() -> QString {
    const QString appdata_path = get_config_directory_path();
    if (appdata_path.isEmpty()) {
        QMessageBox::critical(nullptr, "Config Error", "Failed to get AppData directory path.");
        return {};
    }

    const auto appdata_dir = QDir{appdata_path};
    if (!appdata_dir.exists() && !appdata_dir.mkpath(".")) {
        QMessageBox::critical(nullptr, "Config Error", "Failed to create config directory: " + appdata_path);
        return {};
    }

    const QString config_file_path = appdata_dir.absoluteFilePath("configuration.toml");
    if (QFile::exists(config_file_path)) { return config_file_path; }

    // create a default config file if it doesnt exist
    auto config_file = QFile{config_file_path};
    if (!config_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(nullptr, "Config Error", "Failed to create config: " + config_file_path);
        return {};
    }

    auto stream = QTextStream{&config_file};
    stream << "# Friede Configuration File\n";
    stream << "[accounts]\n";
    stream << "# Example: [[accounts]]\n";
    stream << "# note = \"Main Account\"\n";
    stream << "# username = \"your_username\"\n";
    stream << "# password = \"your_password\"\n";
    config_file.close();

    return config_file_path;
}

auto Config::load_config_from_file() const -> toml::table {
    if (config_path_.isEmpty()) return {};

    auto result = toml::parse_file(config_path_.toStdString());
    if (!result) {
        const auto message = QString{"Failed to parse configuration.toml:\n"}.arg(result.error().description());
        QMessageBox::critical(nullptr, "Config Error", message);
        return {};
    }

    return std::move(result).table();
}

auto Config::save_config_to_file(const toml::table &config) -> bool {
    if (config_path_.isEmpty()) return false;

    try {
        auto ofs = std::ofstream{config_path_.toStdString()};
        if (!ofs.is_open()) {
            QMessageBox::critical(nullptr, "Save Error", "Failed to open config for writing.");
            return false;
        }

        ofs << config;
        ofs.close();
    } catch (const std::exception &e) {
        QMessageBox::critical(nullptr, "Save Error", "An exception occurred while saving: " + QString(e.what()));
        return false;
    }

    return true;
}

auto Config::get_accounts() const -> QVector<Account> {
    QVector<Account> accounts_list;
    auto config = load_config_from_file();

    const auto accounts_node = config["accounts"];
    if (!accounts_node.is_array_of_tables()) return accounts_list;

    for (const auto &table : *accounts_node.as_array()) {
        if (!table.is_table()) continue;
        const auto *tbl_ptr = table.as_table();

        Account acc;
        acc.note = QString::fromStdString(tbl_ptr->get("note")->value_or(""s));
        acc.username = QString::fromStdString(tbl_ptr->get("username")->value_or(""s));
        acc.password = QString::fromStdString(tbl_ptr->get("password")->value_or(""s));

        if (!acc.username.isEmpty() && !acc.password.isEmpty()) accounts_list.append(acc);
    }

    return accounts_list;
}

auto Config::add_account(const Account &account) -> bool {
    auto config = load_config_from_file();
    if (config.empty()) return false;

    auto *accounts_array = config.get_as<toml::array>("accounts");
    if (!accounts_array) {
        config.insert("accounts", toml::array{});
        accounts_array = config.get_as<toml::array>("accounts");
    }

    toml::table new_account_table;
    new_account_table.insert("note", account.note.toStdString());
    new_account_table.insert("username", account.username.toStdString());
    new_account_table.insert("password", account.password.toStdString());

    accounts_array->push_back(new_account_table);
    return save_config_to_file(config);
}

auto Config::update_account(int index, const Account &account) -> bool {
    auto config = load_config_from_file();
    if (config.empty()) return false;

    auto *accounts_array = config.get_as<toml::array>("accounts");
    if (!accounts_array || index < 0 || static_cast<size_t>(index) >= accounts_array->size()) { return false; }

    auto *table_to_update = accounts_array->at(index).as_table();
    if (!table_to_update) return false;

    table_to_update->insert_or_assign("note", account.note.toStdString());
    table_to_update->insert_or_assign("username", account.username.toStdString());
    table_to_update->insert_or_assign("password", account.password.toStdString());

    return save_config_to_file(config);
}

auto Config::remove_account(int index) -> bool {
    auto config = load_config_from_file();
    if (config.empty()) return false;

    auto *accounts_array = config.get_as<toml::array>("accounts");
    if (!accounts_array || index < 0 || static_cast<size_t>(index) >= accounts_array->size()) { return false; }

    accounts_array->erase(accounts_array->begin() + index);
    return save_config_to_file(config);
}

} // namespace core
