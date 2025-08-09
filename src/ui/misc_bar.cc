// =================================================================================
// ui/misc_bar.cc
// =================================================================================

#include "ui/misc_bar.hpp"

#include <QIcon>

namespace ui {

Misc_Bar::Misc_Bar(QWidget *parent)
    : QWidget{parent}
    , options_button_{new QPushButton{this}}
    , options_menu_{new QMenu{options_button_}}
{
    setObjectName("left_bar_widget");
    setFixedWidth(30);

    setAttribute(Qt::WA_StyledBackground);
    setup_ui();
}

auto Misc_Bar::setup_ui() -> void
{
    auto *layout = new QVBoxLayout{this};
    layout->setContentsMargins(5, 10, 5, 10);
    layout->setSpacing(10);

    options_button_->setObjectName("options_button");
    options_button_->setIcon(QIcon::fromTheme("document-properties"));
    options_button_->setFixedSize(20, 20);

    auto *action_customize_theme = options_menu_->addAction("customize theme");
    action_customize_theme->setIcon(QIcon::fromTheme("weather-clear"));
    connect(action_customize_theme, &QAction::triggered, this, &Misc_Bar::customize_theme_requested);

    options_menu_->addSeparator();

    auto *action_check_for_updates = options_menu_->addAction("check for updates");
    action_check_for_updates->setIcon(QIcon::fromTheme("emblem-synchronized"));
    connect(action_check_for_updates, &QAction::triggered, this, &Misc_Bar::check_for_updates_requested);

    auto *action_open_directory = options_menu_->addAction("open config directory");
    action_open_directory->setIcon(QIcon::fromTheme("folder-open"));
    connect(action_open_directory, &QAction::triggered, this, &Misc_Bar::open_config_directory_requested);

    options_menu_->addSeparator();

    auto *action_close = options_menu_->addAction("close");
    action_close->setIcon(QIcon::fromTheme("window-close"));
    connect(action_close, &QAction::triggered, this, &QWidget::close);

    options_button_->setMenu(options_menu_);

    layout->addWidget(options_button_);
    layout->addStretch();
}

} // namespace ui
