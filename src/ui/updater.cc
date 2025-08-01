#include "updater.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QProcess>
#include <QProgressDialog>
#include <QUrl>
#include <QXmlStreamReader>

#include <QDebug>

namespace updater {

Updater::Updater(QWidget *parent) : QObject(parent), parent_widget_(parent) {
    network_manager_ = new QNetworkAccessManager(this);
}

void Updater::checkForUpdates() {
    const QUrl url("https://raw.githubusercontent.com/xi94/friede/main/appcast.xml");
    QNetworkRequest request(url);
    QNetworkReply *reply = network_manager_->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onUpdateCheckFinished(reply); });
}

void Updater::onUpdateCheckFinished(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Update check failed:" << reply->errorString();
        reply->deleteLater();
        return;
    }

    QString latest_version;
    QUrl installer_url;

    QXmlStreamReader xml(reply->readAll());
    while (!xml.atEnd() && !xml.hasError()) {
        xml.readNext();
        if (xml.isStartElement() && xml.name().toString() == "enclosure") {
            latest_version = xml.attributes().value("sparkle:version").toString();
            installer_url = QUrl(xml.attributes().value("url").toString());
            break;
        }
    }

    reply->deleteLater();

    if (!latest_version.isEmpty() && latest_version > QCoreApplication::applicationVersion()) {
        QMessageBox::StandardButton answer = QMessageBox::information(
            parent_widget_, "Update Available",
            "A new version (" + latest_version + ") is available. Would you like to download and install it?",
            QMessageBox::Yes | QMessageBox::No);

        if (answer == QMessageBox::Yes) {
            downloadAndRunInstaller(installer_url);
        }
    } else {
        // This is a good place to inform the user they are up-to-date
        // if they triggered the check manually from a menu.
        qDebug() << "Application is up to date.";
    }
}

void Updater::downloadAndRunInstaller(const QUrl &url) {
    QNetworkRequest request(url);
    installer_download_reply_ = network_manager_->get(request);

    auto *progress_dialog = new QProgressDialog("Downloading update...", "Cancel", 0, 100, parent_widget_);
    progress_dialog->setWindowModality(Qt::WindowModal);
    progress_dialog->show();

    connect(installer_download_reply_, &QNetworkReply::downloadProgress, this,
            [=](qint64 bytesReceived, qint64 bytesTotal) {
                if (bytesTotal > 0) {
                    progress_dialog->setValue(static_cast<int>((bytesReceived * 100) / bytesTotal));
                }
            });

    connect(installer_download_reply_, &QNetworkReply::finished, this, &Updater::onInstallerDownloadFinished);
    connect(progress_dialog, &QProgressDialog::canceled, installer_download_reply_, &QNetworkReply::abort);
}

void Updater::onInstallerDownloadFinished() {
    if (installer_download_reply_->error() != QNetworkReply::NoError) {
        // Don't show an error if the user canceled the download.
        if (installer_download_reply_->error() != QNetworkReply::OperationCanceledError) {
            QMessageBox::critical(parent_widget_, "Download Failed",
                                  "Failed to download the update: " + installer_download_reply_->errorString());
        }
        installer_download_reply_->deleteLater();
        return;
    }


    QString temp_path = QDir::tempPath() + "/friede-setup.exe";
    QFile temp_file(temp_path);
    if (temp_file.open(QIODevice::WriteOnly)) {
        temp_file.write(installer_download_reply_->readAll());
        temp_file.close();

        QStringList args;
        args << "/SILENT" << "/DIR=" + QDir::toNativeSeparators(QCoreApplication::applicationDirPath());

        // Start the installer with the arguments and close this application.
        QProcess::startDetached(temp_path, args);
        QCoreApplication::quit();
    } else {
        QMessageBox::critical(parent_widget_, "Error", "Could not save the downloaded installer.");
    }

    installer_download_reply_->deleteLater();
}

} // namespace updater
