// =================================================================================
// ui/updater.hpp
// =================================================================================

#pragma once

#include <QObject>

class QNetworkAccessManager;
class QNetworkReply;
class QUrl;
class QWidget;

namespace ui {

/**
 * @class Updater
 * @brief Manages the application update process.
 *
 * This class checks for new versions by downloading an appcast file,
 * prompts the user, and handles the download and execution of the installer.
 */
class Updater final : public QObject {
    Q_OBJECT

  public:
    /**
     * @brief Constructs the updater.
     * @param parent The parent widget, used for displaying dialogs.
     */
    explicit Updater(QWidget *parent = nullptr);

    /**
     * @brief Begins the process of checking for a new version.
     */
    auto check_for_updates() -> void;

  private slots:
    /**
     * @brief Handles the network reply after checking for updates.
     * @param reply The network reply containing the appcast data or an error.
     * @note This function parses the XML, compares versions, and may prompt the user.
     */
    auto on_update_check_finished(QNetworkReply *reply) -> void;

    /**
     * @brief Handles the completion of the installer download.
     * @note Saves the installer to a temporary file and attempts to execute it.
     */
    auto on_installer_download_finished() -> void;

  private:
    /**
     * @brief Downloads the installer from the given URL.
     * @param url The URL of the installer executable.
     */
    auto download_and_run_installer(const QUrl &url) -> void;

    /// @brief Manages all network communication.
    QNetworkAccessManager *network_manager_;

    /// @brief Holds the network reply for the installer download.
    QNetworkReply *installer_download_reply_ = nullptr;

    /// @brief Parent widget for displaying message boxes and dialogs.
    QWidget *parent_widget_;
};

} // namespace ui
