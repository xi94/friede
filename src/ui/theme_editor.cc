// =================================================================================
// ui/theme_editor.cc
// =================================================================================

#include "theme_editor.hpp"
#include "core/theme.hpp"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QVBoxLayout>

namespace ui {

Theme_Editor::Theme_Editor(core::Theme &theme, QWidget *parent)
    : QDialog{parent}
    , current_theme_{theme}
{
    setWindowTitle("Customize Theme");
    setMinimumSize(600, 400);
    this->setObjectName("theme_editor_dialog");

    auto *main_h_layout = new QHBoxLayout{};
    auto *form_widget = new QWidget{};
    form_layout_ = new QFormLayout{form_widget};
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

    preview_group_ = new QGroupBox{"Preview"};
    preview_group_->setObjectName("preview_group");
    auto *preview_layout = new QVBoxLayout{};
    preview_layout->setSpacing(10);

    preview_label_ = new QLabel{"Sample Text"};
    preview_line_edit_ = new QLineEdit{"Sample Input"};
    preview_table_ = new QTableWidget{1, 2};
    preview_table_->setHorizontalHeaderLabels({"Header 1", "Header 2"});
    preview_table_->setItem(0, 0, new QTableWidgetItem{"Item A"});
    preview_table_->setItem(0, 1, new QTableWidgetItem{"Item B"});
    preview_table_->setFixedHeight(80);
    preview_table_->horizontalHeader()->setStretchLastSection(true);

    preview_button_ = new QPushButton{"Sample Button"};
    preview_disabled_button_ = new QPushButton{"Disabled Button"};
    preview_disabled_button_->setEnabled(false);
    preview_disabled_label_ = new QLabel{"Disabled Text"};

    preview_layout->addWidget(preview_label_);
    preview_layout->addWidget(preview_line_edit_);
    preview_layout->addWidget(preview_table_);
    preview_layout->addWidget(preview_button_);
    preview_layout->addWidget(preview_disabled_button_);
    preview_layout->addWidget(preview_disabled_label_);
    preview_group_->setLayout(preview_layout);

    main_h_layout->addWidget(form_widget);
    main_h_layout->addWidget(preview_group_);

    auto *button_layout = new QHBoxLayout{};
    auto *save_button = new QPushButton{"Save"};
    auto *cancel_button = new QPushButton{"Cancel"};
    button_layout->addStretch();
    button_layout->addWidget(save_button);
    button_layout->addWidget(cancel_button);

    auto *root_layout = new QVBoxLayout{this};
    root_layout->addLayout(main_h_layout);
    root_layout->addLayout(button_layout);

    connect(save_button, &QPushButton::clicked, this, &Theme_Editor::on_save_button_clicked);
    connect(cancel_button, &QPushButton::clicked, this, &Theme_Editor::on_cancel_button_clicked);

    update_previews();
}

auto Theme_Editor::create_color_picker(const QString &label, QColor &color_ref) -> void
{
    auto *button = new QPushButton{};
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

    const auto preview_stylesheet =
        QString{"QGroupBox#preview_group {"
                "    background-color: %1;"
                "    border: 1px solid %2;"
                "    margin-top: 4px;"
                "}"
                "QGroupBox#preview_group::title {"
                "    color: %3;"
                "    subcontrol-origin: margin;"
                "    subcontrol-position: top center;"
                "    padding: 0 5px;"
                "}"
                "QGroupBox#preview_group QLabel {"
                "    color: %3;"
                "    background-color: transparent;"
                "    border: none;"
                "}"
                "QGroupBox#preview_group QPushButton {"
                "    background-color: %4;"
                "    color: %3;"
                "    border: 1px solid %2;"
                "}"
                "QGroupBox#preview_group QPushButton:hover {"
                "    background-color: %5;"
                "}"
                "QGroupBox#preview_group QPushButton:disabled {"
                "    background-color: %6;"
                "    color: %7;"
                "}"
                "QGroupBox#preview_group QLineEdit, QGroupBox#preview_group QTableWidget {"
                "    background-color: %8;"
                "    color: %3;"
                "    border: 1px solid %2;"
                "}"
                "QGroupBox#preview_group QTableWidget::item {"
                "    color: %3;"
                "}"
                "QGroupBox#preview_group QHeaderView::section {"
                "    background-color: %8;"
                "    color: %3;"
                "    border: 1px solid %2;"
                "}"}
            .arg(current_theme_.background_dark.name(), current_theme_.border.name(), current_theme_.text_primary.name(),
                 current_theme_.button_primary.name(), current_theme_.button_hover.name(), current_theme_.button_disabled.name(),
                 current_theme_.text_disabled.name(), current_theme_.background_light.name());

    preview_group_->setStyleSheet(preview_stylesheet);
    preview_disabled_label_->setStyleSheet(
        QString{"color: %1; background-color: transparent; border: none;"}.arg(current_theme_.text_disabled.name()));
}

auto Theme_Editor::on_save_button_clicked() -> void
{
    QDialog::accept();
}

auto Theme_Editor::on_cancel_button_clicked() -> void
{
    QDialog::reject();
}

} // namespace ui
