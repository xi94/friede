// =================================================================================
// ui/control_bar.cc
// =================================================================================

#include "ui/control_bar.hpp"

#include <QCoreApplication>
#include <QHBoxLayout>

#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QSvgRenderer>
#include <QWidget>

namespace ui {

// TODO move this out of here or something, could be useful in other places
auto create_colorized_pixmap(const QString &path, const QColor &color, const QSize &size) -> QPixmap
{
    auto renderer = QSvgRenderer{path};
    if (!renderer.isValid()) return QPixmap{};

    auto pixmap = QPixmap{size};
    pixmap.fill(Qt::transparent);

    auto painter = QPainter{&pixmap};
    renderer.render(&painter);

    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(pixmap.rect(), color);
    painter.end();

    return pixmap;
}

Control_Bar::Control_Bar(QWidget *parent)
    : QWidget{parent}
    , game_icon_label_{new QLabel{this}}
    , login_button_{new QPushButton{"", this}}
    , add_account_button_{new QPushButton{"", this}}
    , remove_account_button_{new QPushButton{"", this}}
{
    QWidget::setObjectName("bottom_bar_widget");
    QWidget::setFixedHeight(50);

    login_button_->setObjectName("login_button");
    add_account_button_->setObjectName("add_account_button");
    remove_account_button_->setObjectName("remove_account_button");

    const QString icons_path = QCoreApplication::applicationDirPath() + "/icons";

    auto create_control_icon = [&](const char *path) -> QIcon {
        auto icon = QIcon{};

        constexpr auto icon_size = QSize{36, 36};
        const QColor icon_color = palette().color(QPalette::ButtonText);
        const QColor disabled_color = palette().color(QPalette::Disabled, QPalette::ButtonText);

        const QPixmap normal = create_colorized_pixmap(icons_path + path, icon_color, icon_size);
        const QPixmap disabled = create_colorized_pixmap(icons_path + path, disabled_color, icon_size);

        icon.addPixmap(normal, QIcon::Normal);
        icon.addPixmap(disabled, QIcon::Disabled);

        return icon;
    };

    const QIcon login_icon = create_control_icon("/log-in.svg");
    login_button_->setIcon(login_icon);

    const QIcon add_account_icon = create_control_icon("/add-file.svg");
    add_account_button_->setIcon(add_account_icon);

    const QIcon remove_account_icon = create_control_icon("/erase.svg");
    remove_account_button_->setIcon(remove_account_icon);

    setup_ui();
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

    constexpr auto icon_size = QSize{20, 20};
    constexpr auto button_size = QSize{40, 30};

    login_button_->setIconSize(icon_size);
    login_button_->setFixedSize(button_size);

    add_account_button_->setIconSize(icon_size);
    add_account_button_->setFixedSize(button_size);

    remove_account_button_->setIconSize(icon_size);
    remove_account_button_->setFixedSize(button_size);

    layout->addWidget(login_button_);

    auto *separator = new QFrame{};
    separator->setFrameShape(QFrame::VLine);
    separator->setFrameShadow(QFrame::Plain);

    separator->setFixedWidth(1);
    separator->setFixedHeight(25);

    layout->addWidget(separator);

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
