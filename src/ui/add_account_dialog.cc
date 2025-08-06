// =================================================================================
// ui/add_account_dialog.cc
// =================================================================================

#include "add_account_dialog.hpp"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpacerItem>
#include <QVBoxLayout>

namespace ui {

Add_Account_Dialog::Add_Account_Dialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Add New Account");

    constexpr int dialog_width = 250;
    constexpr int dialog_height = 280;
    setFixedSize(dialog_width, dialog_height);

    constexpr int content_margin = 20;
    constexpr int widget_spacing = 10;

    auto *main_layout = new QVBoxLayout{this};
    main_layout->setContentsMargins(content_margin, content_margin, content_margin, content_margin);
    main_layout->setSpacing(widget_spacing);

    auto *label_note = new QLabel{"Note", this};
    line_edit_note_ = new QLineEdit{this};
    line_edit_note_->setPlaceholderText("Enter a note (e.g., 'alt account')");

    main_layout->addWidget(label_note);
    main_layout->addWidget(line_edit_note_);

    auto *label_username = new QLabel{"Username", this};
    line_edit_username_ = new QLineEdit{this};
    line_edit_username_->setPlaceholderText("Enter your username");

    main_layout->addWidget(label_username);
    main_layout->addWidget(line_edit_username_);

    auto *label_password = new QLabel{"Password", this};
    line_edit_password_ = new QLineEdit{this};
    line_edit_password_->setEchoMode(QLineEdit::Password);
    line_edit_password_->setPlaceholderText("Enter your password");

    main_layout->addWidget(label_password);
    main_layout->addWidget(line_edit_password_);
    main_layout->addSpacerItem(new QSpacerItem{20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding});

    auto *button_box = new QDialogButtonBox{QDialogButtonBox::Ok | QDialogButtonBox::Cancel};

    constexpr int usable_width = dialog_width - (content_margin * 2);
    constexpr int button_gap = 5;
    constexpr int num_buttons = 2;

    constexpr int button_width = (usable_width - button_gap) / num_buttons;

    button_box->setFixedWidth(usable_width);
    button_box->setContentsMargins(0, 0, 0, 0);
    button_box->setLayoutDirection(Qt::LeftToRight);

    button_box->button(QDialogButtonBox::Ok)->setFixedWidth(button_width);
    button_box->button(QDialogButtonBox::Cancel)->setFixedWidth(button_width);

    auto *button_layout = new QHBoxLayout{};
    button_layout->addStretch();
    button_layout->addWidget(button_box);
    main_layout->addLayout(button_layout);

    connect(button_box, &QDialogButtonBox::accepted, this, &Add_Account_Dialog::accept);
    connect(button_box, &QDialogButtonBox::rejected, this, &Add_Account_Dialog::reject);
}

auto Add_Account_Dialog::get_note() const -> QString
{
    return line_edit_note_->text();
}

auto Add_Account_Dialog::get_username() const -> QString
{

    return line_edit_username_->text();
}

auto Add_Account_Dialog::get_password() const -> QString
{
    return line_edit_password_->text();
}

} // namespace ui
