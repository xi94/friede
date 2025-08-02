// =================================================================================
// ui/password_table_widget.hpp
// =================================================================================

#pragma once

#include <QTableWidget>

namespace ui {

/**
 * @class Password_Table_Widget
 * @brief A QTableWidgetItem that masks password text for display.
 *
 * Displays asterisks for the Qt::DisplayRole while storing the actual password
 * in the Qt::UserRole. When edited, it reveals the real password.
 */
class Password_Table_Widget final : public QTableWidgetItem {
  public:
    /**
     * @brief Returns the raw password for editing and the masked version for display.
     * @param role The data role to retrieve.
     * @return The data associated with the specified role.
     */
    auto data(int role) const -> QVariant override {
        if (role == Qt::EditRole) { return QTableWidgetItem::data(Qt::UserRole); }
        return QTableWidgetItem::data(role);
    }
};

} // namespace ui
