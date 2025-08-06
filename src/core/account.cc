// =================================================================================
// core/account.cc
// =================================================================================

#include "account.hpp"

using namespace std::literals;

namespace core {

Account_Config::Account_Config(QObject *parent)
    : Config("accounts.toml", parent)
{
}

auto Account_Config::get_accounts() const -> QVector<Account>
{
    QVector<Account> accounts_list;
    auto config = load();

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

auto Account_Config::add_account(const Account &account) -> bool
{
    toml::table config = Config::load();

    auto *accounts_array = config.get_as<toml::array>("accounts");
    if (!accounts_array) {
        config.insert_or_assign("accounts", toml::array{});
        accounts_array = config.get_as<toml::array>("accounts");
    }

    if (!accounts_array) return false;

    toml::table new_account_table;
    new_account_table.insert("note", account.note.toStdString());
    new_account_table.insert("username", account.username.toStdString());
    new_account_table.insert("password", account.password.toStdString());

    accounts_array->push_back(new_account_table);
    return save(config);
}

auto Account_Config::update_account(int index, const Account &account) -> bool
{
    auto config = load();

    auto *accounts_array = config.get_as<toml::array>("accounts");
    if (!accounts_array || index < 0 || static_cast<size_t>(index) >= accounts_array->size()) { return false; }

    auto *table_to_update = accounts_array->at(index).as_table();
    if (!table_to_update) return false;

    table_to_update->insert_or_assign("note", account.note.toStdString());
    table_to_update->insert_or_assign("username", account.username.toStdString());
    table_to_update->insert_or_assign("password", account.password.toStdString());

    return save(config);
}

auto Account_Config::remove_account(int index) -> bool
{
    auto config = load();

    auto *accounts_array = config.get_as<toml::array>("accounts");
    if (!accounts_array || index < 0 || static_cast<size_t>(index) >= accounts_array->size()) { return false; }

    accounts_array->erase(accounts_array->begin() + index);
    return save(config);
}

} // namespace core
