// =================================================================================
// ui/control_bar.cc
// =================================================================================

#include "ui/control_bar.hpp"

#include <QHBoxLayout>
#include <QIcon>

namespace ui {

Control_Bar::Control_Bar(QWidget *parent)
    : QWidget{parent}
    , game_icon_label_{new QLabel{this}}
    , login_button_{new QPushButton{"Login", this}}
    , add_account_button_{new QPushButton{"Add Account", this}}
    , remove_account_button_{new QPushButton{"Remove Account", this}}
{
    QWidget::setObjectName("bottom_bar_widget");
    QWidget::setFixedHeight(50);
    QWidget::setup_ui();
}

auto Control_Bar::set_controls_enabled(bool enabled) -> void
{
    login_button_->setEnabled(enabled);
    remove_account_button_->setEnabled(enabled);
}

auto Control_Bar::update_game_context(riot::Game game, const QString &icon_path) -> void
{
    const auto game_icon = QIcon{icon_path};
    game_icon_label_->setPixmap(game_icon.pixmap(QSize{32, 32}));
    game_icon_label_->show();

    login_button_->show();
    add_account_button_->show();
    remove_account_button_->show();
}

auto Control_Bar::setup_ui() -> void
{
    auto *layout = new QHBoxLayout{this};
    layout->setContentsMargins(10, 0, 15, 0);
    layout->setSpacing(10);

    layout->addWidget(login_button_);
    layout->addWidget(add_account_button_);
    layout->addWidget(remove_account_button_);
    layout->addStretch();

    game_icon_label_->setObjectName("game_icon_placeholder");
    game_icon_label_->setFixedSize(32, 32);
    layout->addWidget(game_icon_label_);

    QWidget::connect(login_button_, &QPushButton::clicked, this, &Control_Bar::login_clicked);
    QWidget::connect(add_account_button_, &QPushButton::clicked, this, &Control_Bar::add_account_clicked);
    QWidget::connect(remove_account_button_, &QPushButton::clicked, this, &Control_Bar::remove_account_clicked);
}

} // namespace ui
