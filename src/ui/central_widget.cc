#include "centraL_widget.hpp"

namespace ui {

Central_Widget::Central_Widget(core::Theme_Config *theme_config, QWidget *parent)
    : QWidget(parent)
    , theme_config_{theme_config}
{
}

auto Central_Widget::paintEvent(QPaintEvent *event) -> void
{
    constexpr int radius = 2;
    auto painter = QPainter{this};

    // FIXME the anti aliasing is kind of awful, but it also is sort of a must
    //    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    path.addRoundedRect(QWidget::rect().adjusted(1, 1, -1, -1), radius, radius);

    auto pen = QPen{QColor{"#616161"}, 1};
    painter.setPen(pen);

    auto brush = QBrush{theme_config_->load().background_dark};
    painter.setBrush(brush);

    painter.drawPath(path);
}

} // namespace ui
