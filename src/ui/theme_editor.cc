// =================================================================================
// ui/theme_editor.cc
// =================================================================================

#include "theme_editor.hpp"
#include "core/theme.hpp"

#include <QGroupBox>
#include <QVBoxLayout>

namespace ui {

Theme_Editor::Theme_Editor(core::Theme &theme, QWidget *parent)
    : QDialog{parent}
    , current_theme_{theme}
{
    setWindowTitle("Customize Theme");
    setMinimumWidth(400);

    auto *main_layout = new QVBoxLayout{this};
    form_layout_ = new QFormLayout;
    form_layout_->setSpacing(10);

    create_color_picker("Background Dark", current_theme_.background_dark);
    create_color_picker("Background Light", current_theme_.background_light);
    create_color_picker("Text Primary", current_theme_.text_primary);
    create_color_picker("Border", current_theme_.border);
    create_color_picker("Accent", current_theme_.accent);
    create_color_picker("Accent Hover", current_theme_.accent_hover);
    create_color_picker("Button Primary", current_theme_.button_primary);
    create_color_picker("Button Hover", current_theme_.button_hover);
    create_color_picker("Button Disabled", current_theme_.button_disabled);
    create_color_picker("Text Disabled", current_theme_.text_disabled);

    auto *preview_group = new QGroupBox{"Preview"};
    auto *preview_layout = new QVBoxLayout;
    preview_label_ = new QLabel{"Sample Text"};
    preview_button_ = new QPushButton{"Sample Button"};
    preview_layout->addWidget(preview_label_);
    preview_layout->addWidget(preview_button_);
    preview_group->setLayout(preview_layout);

    auto *button_layout = new QHBoxLayout;
    auto *save_button = new QPushButton{"Save"};
    auto *cancel_button = new QPushButton{"Cancel"};
    button_layout->addStretch();
    button_layout->addWidget(save_button);
    button_layout->addWidget(cancel_button);

    main_layout->addLayout(form_layout_);
    main_layout->addWidget(preview_group);
    main_layout->addLayout(button_layout);

    connect(save_button, &QPushButton::clicked, this, &Theme_Editor::on_save_button_clicked);
    connect(cancel_button, &QPushButton::clicked, this, &Theme_Editor::on_cancel_button_clicked);

    update_previews();
}

auto Theme_Editor::create_color_picker(const QString &label, QColor &color_ref) -> void
{
    auto *button = new QPushButton;
    button->setFixedSize(24, 24);

    form_layout_->addRow(label, button);
    color_map_[button] = &color_ref;

    connect(button, &QPushButton::clicked, this, &Theme_Editor::on_color_button_clicked);
}

auto Theme_Editor::on_color_button_clicked() -> void
{
    auto *button = qobject_cast<QPushButton *>(sender());
    if (!button || !color_map_.contains(button)) return;

    QColor &current_color = *color_map_[button];
    const QColor new_color = QColorDialog::getColor(current_color, this, "Select Color");

    if (new_color.isValid()) {
        current_color = new_color;
        update_previews();
    }
}

auto Theme_Editor::update_previews() -> void
{
    for (auto it = color_map_.begin(); it != color_map_.end(); ++it) {
        it.key()->setStyleSheet(QString{"background-color: %1;"}.arg(it.value()->name()));
    }

    preview_label_->setStyleSheet(QString{"color: %1;"}.arg(current_theme_.text_primary.name()));
    preview_button_->setStyleSheet(QString{"background-color: %1; color: %2; border: 1px solid %3;"}.arg(
        current_theme_.button_primary.name(), current_theme_.text_primary.name(), current_theme_.border.name()));

    this->setStyleSheet(QString{"background-color: %1;"}.arg(current_theme_.background_dark.name()));
}

auto Theme_Editor::on_save_button_clicked() -> void
{
    accept();
}

auto Theme_Editor::on_cancel_button_clicked() -> void
{
    reject();
}

} // namespace ui
