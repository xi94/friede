// =================================================================================
// ui/control_bar.hpp
// =================================================================================

#pragma once

#include "riot/client.hpp"
#include <QLabel>
#include <QPushButton>
#include <QWidget>

namespace ui {

class Control_Bar final : public QWidget {
    Q_OBJECT

  public:
    explicit Control_Bar(QWidget *parent = nullptr);
    ~Control_Bar() override = default;

    auto set_controls_enabled(bool enabled) -> void;
    auto update_game_context(riot::Game game, const QString &icon_path) -> void;

  signals:
    auto login_clicked() -> void;
    auto add_account_clicked() -> void;
    auto remove_account_clicked() -> void;

  private:
    auto setup_ui() -> void;

    QLabel *game_icon_label_;
    QPushButton *login_button_;
    QPushButton *add_account_button_;
    QPushButton *remove_account_button_;
};

} // namespace ui
