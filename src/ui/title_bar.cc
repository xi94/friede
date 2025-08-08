// =================================================================================
// ui/title_bar.cc
// =================================================================================

#include "ui/title_bar.hpp"

#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMainWindow>

namespace ui {

Title_Bar::Title_Bar(QWidget *parent, const QString &title)
    : QWidget{parent} //    , title_label_{new QLabel{title, this}}
    , home_button_{new QPushButton{"", this}}
    , minimize_button_{new QPushButton{"", this}}
    , maximize_button_{new QPushButton{"", this}}
    , close_button_{new QPushButton{"", this}}
    , mouse_click_position_{}
    , window_position_{}
{
    setObjectName("title_bar");
    setFixedHeight(40);

    setAttribute(Qt::WA_StyledBackground);
    setup_ui();
}

void Title_Bar::set_home_button_visible(bool visible)
{
    home_button_->setVisible(visible);
}

void Title_Bar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        window_position_ = QWidget::window()->pos();
        mouse_click_position_ = event->globalPosition();
    }
}

void Title_Bar::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        const QPointF delta = event->globalPosition() - mouse_click_position_;
        QWidget::window()->move(window_position_ + delta.toPoint());
    }
}

void Title_Bar::setup_ui()
{
    auto *layout = new QHBoxLayout{this};
    layout->setContentsMargins(5, 0, 5, 0);
    layout->setSpacing(5);

    constexpr std::string_view control_button_stylesheet = "QPushButton {"
                                                           "  background-color: transparent;"
                                                           "  border-color: transparent;"
                                                           "}"
                                                           "QPushButton#maximize_button:hover,"
                                                           "QPushButton#minimize_button:hover {"
                                                           "  background-color: rgba(200, 200, 200, 30);"
                                                           "}"
                                                           "QPushButton#close_button:hover {"
                                                           "  background-color: rgba(240, 0, 0, 200);"
                                                           "}"
                                                           "QPushButton#maximize_button:pressed,"
                                                           "QPushButton#minimize_button:pressed {"
                                                           "  background-color: rgba(100, 100, 100, 30);"
                                                           "}"
                                                           "QPushButton#close_button:pressed {"
                                                           "  background-color: rgba(150, 0, 0, 200);"
                                                           "}";

    home_button_->setObjectName("home_button");
    home_button_->setText("\tback");
    home_button_->setIcon(QIcon::fromTheme("document-revert"));
    home_button_->hide();

    minimize_button_->setObjectName("minimize_button");
    minimize_button_->setIcon(QIcon::fromTheme("list-remove"));
    minimize_button_->setFixedSize(30, 30);
    minimize_button_->setIconSize({12, 12});
    minimize_button_->setStyleSheet(control_button_stylesheet.data());

    maximize_button_->setObjectName("maximize_button");
    maximize_button_->setIcon(QIcon::fromTheme("view-fullscreen"));
    maximize_button_->setFixedSize(30, 30);
    maximize_button_->setIconSize({14, 14});
    maximize_button_->setStyleSheet(control_button_stylesheet.data());

    close_button_->setObjectName("close_button");
    close_button_->setIcon(QIcon::fromTheme("window-close"));
    close_button_->setFixedSize(30, 30);
    close_button_->setIconSize({10, 10});
    close_button_->setStyleSheet(control_button_stylesheet.data());

    layout->addWidget(home_button_);
    //    layout->addWidget(title_label_);
    layout->addStretch();
    layout->addWidget(minimize_button_);
    layout->addWidget(maximize_button_);
    layout->addWidget(close_button_);

    connect(home_button_, &QPushButton::clicked, this, &Title_Bar::home_button_clicked);

    connect(minimize_button_, &QPushButton::clicked, QWidget::window(), &QMainWindow::showMinimized);
    connect(close_button_, &QPushButton::clicked, QWidget::window(), &QMainWindow::close);
    connect(maximize_button_, &QPushButton::clicked, this, [parent = QWidget::window()] {
        if (parent->isMaximized()) {
            parent->showNormal();
        } else {
            parent->showMaximized();
        }
    });
}

} // namespace ui
