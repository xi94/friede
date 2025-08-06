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
#include <string_view>

using namespace std::literals;

namespace core {

Config::Config(const QString file_name)
    : config_path_{initialize_config_path(file_name)}
{
}

auto Config::path() const -> const QString &
{
    return config_path_;
}

auto Config::get_config_directory_path() const -> QString
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

auto Config::initialize_config_path(const QString file_name) -> QString
{
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

    const QString config_file_path = appdata_dir.absoluteFilePath(file_name);
    if (QFile::exists(config_file_path)) { return config_file_path; }

    auto config_file = QFile{config_file_path};
    if (!config_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(nullptr, "Config Error", "Failed to create config: " + config_file_path);
        return {};
    }

    return config_file_path;
}

auto Config::load() const -> toml::table
{
    if (config_path_.isEmpty()) return {};

    auto result = toml::parse_file(config_path_.toStdString());
    if (!result) {
        if (std::string_view{result.error().description()}.find("The file was empty"sv) != std::string_view::npos) { return {}; }

        const auto message =
            QString{"Failed to parse configuration.toml:\n"} + QString::fromStdString(std::string{result.error().description()});
        QMessageBox::critical(nullptr, "Config Error", message);
        return {};
    }

    return std::move(result).table();
}

auto Config::save(const toml::table &config) -> bool
{
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

} // namespace core
