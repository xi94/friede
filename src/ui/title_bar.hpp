// =================================================================================
// ui/title_bar.hpp
// =================================================================================

#pragma once

#include <QMouseEvent>
#include <QPushButton>
#include <QString>
#include <QWidget>

class QLabel;

namespace ui {

class Title_Bar final : public QWidget {
    Q_OBJECT

  public:
    explicit Title_Bar(QWidget *parent, const QString &title = "");
    ~Title_Bar() override = default;

    /// @brief Sets the visibility of the home/back button.
    auto set_home_button_visible(bool visible) -> void;

  signals:
    /// @brief Emitted when the home/back button is clicked.
    auto home_button_clicked() -> void;

  protected:
    /// @brief Handles mouse press events to initiate window dragging.
    auto mousePressEvent(QMouseEvent *event) -> void override;

    /// @brief Handles mouse move events to drag the window.
    auto mouseMoveEvent(QMouseEvent *event) -> void override;

  private:
    /// @brief Sets up the widgets, layout, and connections for the title bar.
    auto setup_ui() -> void;

  private:
    QLabel *title_label_;

    QPushButton *home_button_;
    QPushButton *minimize_button_;
    QPushButton *maximize_button_;
    QPushButton *close_button_;

    // used for window dragging
    QPoint window_position_;
    QPointF mouse_click_position_;
};

} // namespace ui
