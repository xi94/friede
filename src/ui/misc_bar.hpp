// =================================================================================
// ui/misc_bar.hpp
// =================================================================================

#pragma once

#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

namespace ui {

class Misc_Bar final : public QWidget {
    Q_OBJECT

  public:
    explicit Misc_Bar(QWidget *parent = nullptr);
    ~Misc_Bar() override = default;

  signals:
    auto customize_theme_requested() -> void;
    auto check_for_updates_requested() -> void;
    auto open_config_directory_requested() -> void;

  private:
    auto setup_ui() -> void;

    QPushButton *options_button_;
    QMenu *options_menu_;
};

} // namespace ui
