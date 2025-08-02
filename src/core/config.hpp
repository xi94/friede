// =================================================================================
// core/config.hpp
// =================================================================================

#pragma once

#include "account.hpp"

#include <QObject>
#include <QString>
#include <QVector>

#define TOML_EXCEPTIONS 0
#define TOML_HEADER_ONLY 1
#include <toml++/toml.hpp>

namespace core {

/**
 * @class Config
 * @brief Manages reading from and writing to the TOML configuration file.
 */
class Config final : public QObject {
    Q_OBJECT

  public:
    explicit Config(QObject *parent = nullptr);

    /// @brief Returns a list of all accounts from the configuration file.
    auto get_accounts() const -> QVector<Account>;

    /// @brief Adds a new account to the configuration file.
    /// @return True on success, false on failure.
    auto add_account(const Account &account) -> bool;

    /// @brief Updates an existing account at a specific index.
    /// @return True on success, false on failure.
    auto update_account(int index, const Account &account) -> bool;

    /// @brief Removes an account at a specific index.
    /// @return True on success, false on failure.
    auto remove_account(int index) -> bool;

    /// @brief Returns the full path to the configuration directory.
    auto get_config_directory_path() const -> QString;

  private:
    /// @brief Gets the path to the config file, creating it if it doesn't exist.
    auto initialize_config_path() -> QString;

    /// @brief Reads the TOML file from disk into a toml::table.
    auto load_config_from_file() const -> toml::table;

    /// @brief Writes a toml::table to the configuration file on disk.
    auto save_config_to_file(const toml::table &config) -> bool;

    QString config_path_;
};

} // namespace core
