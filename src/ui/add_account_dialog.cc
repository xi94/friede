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

Add_Account_Dialog::Add_Account_Dialog(QWidget *parent) : QDialog(parent) {
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
    line_edit_note = new QLineEdit{this};
    line_edit_note->setPlaceholderText("Enter a note (e.g., 'alt account')");
    line_edit_note->setStyleSheet("QLineEdit { border: none; background-color: #333333; color: white; padding: 5px; border-radius: 3px; }");

    main_layout->addWidget(label_note);
    main_layout->addWidget(line_edit_note);

    auto *label_username = new QLabel{"Username", this};
    line_edit_username = new QLineEdit{this};
    line_edit_username->setPlaceholderText("Enter your username");
    line_edit_username->setStyleSheet("QLineEdit { border: none; background-color: #333333; color: white; padding: 5px; border-radius: 3px; }");

    main_layout->addWidget(label_username);
    main_layout->addWidget(line_edit_username);

    auto *label_password = new QLabel{"Password", this};
    line_edit_password = new QLineEdit{this};
    line_edit_password->setEchoMode(QLineEdit::Password);
    line_edit_password->setPlaceholderText("Enter your password");
    line_edit_password->setStyleSheet("QLineEdit { border: none; background-color: #333333; color: white; padding: 5px; border-radius: 3px; }");

    main_layout->addWidget(label_password);
    main_layout->addWidget(line_edit_password);
    main_layout->addSpacerItem(new QSpacerItem{20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding});

    auto *button_box = new QDialogButtonBox{QDialogButtonBox::Ok | QDialogButtonBox::Cancel};

    constexpr int usable_width = dialog_width - (content_margin * 2);
    constexpr int button_gap = 5;
    constexpr int num_buttons = 2;

    constexpr int button_width = (usable_width - button_gap) / num_buttons;
    constexpr int button_height = 10;

    button_box->setFixedWidth(usable_width);
    button_box->setContentsMargins(0, 0, 0, 0);
    button_box->setLayoutDirection(Qt::LeftToRight);

    button_box->button(QDialogButtonBox::Ok)->setFixedWidth(button_width);
    button_box->button(QDialogButtonBox::Cancel)->setFixedWidth(button_width);

    button_box->setStyleSheet(R"(
        QPushButton {
            background-color: #555555;
            color: white;
            border: 1px solid #666666;
            border-radius: 4px;
            padding: 8px 0px;
            height: )" +
                            QString::number(button_height) + R"(px;
        }
        QPushButton:hover {
            background-color: #666666;
        }
        QPushButton:pressed {
            background-color: #444444;
        }
        QPushButton:disabled {
            background-color: #333333;
            color: #aaaaaa;
            border: 1px solid #444444;
        }
    )");

    auto *button_layout = new QHBoxLayout;
    button_layout->addStretch();
    button_layout->addWidget(button_box);
    main_layout->addLayout(button_layout);

    connect(button_box, &QDialogButtonBox::accepted, this, &Add_Account_Dialog::accept);
    connect(button_box, &QDialogButtonBox::rejected, this, &Add_Account_Dialog::reject);
}

auto Add_Account_Dialog::get_note() const -> QString {
    return line_edit_note->text();
}

auto Add_Account_Dialog::get_username() const -> QString {
    return line_edit_username->text();
}

auto Add_Account_Dialog::get_password() const -> QString {
    return line_edit_password->text();
}

} // namespace ui
