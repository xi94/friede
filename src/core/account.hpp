// =================================================================================
// core/account.hpp
// =================================================================================

#pragma once

#include <QString>
#include <QVector>

#include "config.hpp"

namespace core {

// TODO create a serializer and then a 'struct serializer<Account> {};' impl (same for theme)
struct Account {
    QString note;
    QString username;
    QString password;
};

/**
 * @class Account_Config
 * @brief Manages account-specific data in the configuration file.
 */
class Account_Config final : public Config {
  public:
    Account_Config();

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
};

} // namespace core
