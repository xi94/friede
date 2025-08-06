// =================================================================================
// core/account.hpp
// =================================================================================

#pragma once

#include "config.hpp"
#include <QString>
#include <QVector>

namespace core {

/// @struct Account
/// @brief Represents a single user account with credentials and a note.
struct Account {
    QString note;
    QString username;
    QString password;
};

/// @class Account_Config
/// @brief Manages account-specific data in the configuration file.
class Account_Config final : public Config {
  public:
    /// @brief Constructs the account configuration manager.
    Account_Config();

    /// @brief Returns a list of all accounts from the configuration file.
    auto get_accounts() const -> QVector<Account>;

    /// @brief Adds a new account to the configuration file.
    auto add_account(const Account &account) -> bool;

    /// @brief Updates an existing account at a specific index.
    auto update_account(int index, const Account &account) -> bool;

    /// @brief Removes an account at a specific index.
    auto remove_account(int index) -> bool;
};

} // namespace core
