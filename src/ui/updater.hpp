#pragma once

#include <QObject>

// Forward declarations
class QNetworkAccessManager;
class QNetworkReply;
class QUrl;
class QWidget;

// TODO yeah this is ai generated im tired, fix this tomorrow :D

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
    void checkForUpdates();

  private slots:
    void onUpdateCheckFinished(QNetworkReply *reply);
    void onInstallerDownloadFinished();

  private:
    void downloadAndRunInstaller(const QUrl &url);

    QNetworkAccessManager *network_manager_;
    QNetworkReply *installer_download_reply_ = nullptr;
    QWidget *parent_widget_;
};

} // namespace updater
