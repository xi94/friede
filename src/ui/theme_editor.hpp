// =================================================================================
// ui/theme_editor.hpp
// =================================================================================

#pragma once

#include "core/theme.hpp"

#include <QColorDialog>
#include <QDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>

namespace ui {

/// @class Theme_Editor
/// @brief A dialog for editing and previewing application themes.
class Theme_Editor final : public QDialog {
    Q_OBJECT

  public:
    /// @brief Constructs the theme editor dialog.
    /// @param theme A reference to the theme object to be modified.
    /// @param parent The parent widget.
    explicit Theme_Editor(core::Theme &theme, QWidget *parent = nullptr);

  private slots:
    /// @brief Opens a color dialog when a color swatch button is clicked.
    auto on_color_button_clicked() -> void;

    /// @brief Accepts the dialog, signaling that changes should be saved.
    auto on_save_button_clicked() -> void;

    /// @brief Rejects the dialog, discarding any changes.
    auto on_cancel_button_clicked() -> void;

  private:
    /// @brief Creates a color picker widget and adds it to the form.
    /// @param label The text label for the color picker.
    /// @param color_ref A reference to the QColor object to be modified.
    auto create_color_picker(const QString &label, QColor &color_ref) -> void;

    /// @brief Updates the preview widgets with the current theme colors.
    auto update_previews() -> void;

  private:
    QFormLayout *form_layout_;
    QGroupBox *preview_group_;
    QLabel *preview_label_;
    QLineEdit *preview_line_edit_;
    QTableWidget *preview_table_;
    QPushButton *preview_button_;
    QPushButton *preview_disabled_button_;
    QLabel *preview_disabled_label_;

    /// @brief A reference to the theme object being actively modified by the editor.
    core::Theme &current_theme_;

    /// @brief Maps each color picker button to its corresponding QColor in the theme.
    /// @note This allows a single slot to handle clicks from any color button.
    QMap<QPushButton *, QColor *> color_map_;
};

} // namespace ui
