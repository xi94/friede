#pragma once

#include <QPainter>
#include <QPainterPath>
#include <QWidget>

#include "core/theme.hpp"

namespace ui {

class Central_Widget : public QWidget {
  public:
    explicit Central_Widget(core::Theme_Config *theme_config, QWidget *parent = nullptr);

  protected:
    auto paintEvent(QPaintEvent *event) -> void override;

  private:
    core::Theme_Config *theme_config_;
};

} // namespace ui
