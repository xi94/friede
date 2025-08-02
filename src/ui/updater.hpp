#pragma once

#include <QObject>

class QNetworkAccessManager;
class QNetworkReply;
class QUrl;
class QWidget;

namespace updater {

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
    auto on_update_check_finished(QNetworkReply *reply) -> void;
    auto on_installer_download_finished() -> void;

  private:
    auto download_and_run_installer(const QUrl &url) -> void;

    QNetworkAccessManager *network_manager_;
    QNetworkReply *installer_download_reply_ = nullptr;
    QWidget *parent_widget_;
};

} // namespace updater
