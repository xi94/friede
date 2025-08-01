// =================================================================================
// add_account_dialog.hpp
// =================================================================================

#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QObject>

namespace ui {

/**
 * @class Add_Account_Dialog
 * @brief A modal dialog for adding a new user account.
 */
class Add_Account_Dialog final : public QDialog {
    Q_OBJECT

  public:
    explicit Add_Account_Dialog(QWidget *parent = nullptr);

    /// @brief Returns the text from the note input field.
    auto get_note() const -> QString;

    /// @brief Returns the text from the username input field.
    auto get_username() const -> QString;

    /// @brief Returns the text from the password input field.
    auto get_password() const -> QString;

  private:
    QLineEdit *line_edit_note;
    QLineEdit *line_edit_username;
    QLineEdit *line_edit_password;
};

} // namespace ui
