// =================================================================================
// ui/updater.cc
// =================================================================================

#include "updater.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QProcess>
#include <QProgressDialog>
#include <QUrl>
#include <QVersionNumber>
#include <QXmlStreamReader>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include <QDebug>

namespace ui {

Updater::Updater(QWidget *parent)
    : QObject{parent}
    , parent_widget_{parent}
    , network_manager_{new QNetworkAccessManager{this}} {}

auto Updater::check_for_updates() -> void {
    const auto url = QUrl{"https://raw.githubusercontent.com/xi94/friede/main/appcast.xml"};
    const auto request = QNetworkRequest{url};

    QNetworkReply *reply = network_manager_->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply] { on_update_check_finished(reply); });
}

auto Updater::on_update_check_finished(QNetworkReply *reply) -> void {
    const bool reply_has_error = reply->error() != QNetworkReply::NoError;
    if (reply_has_error) {
        QMessageBox::critical(parent_widget_, "Updater", "Failed to check for updates: " + reply->errorString());
        reply->deleteLater();
        return;
    }

    //
    // parse installer url and latest version from the appcast xml
    //

    QUrl installer_url;
    QString latest_version;

    auto xml = QXmlStreamReader{reply->readAll()};
    while (!xml.atEnd() && !xml.hasError()) {
        xml.readNext();

        const bool current_element_is_enclosure = (xml.isStartElement() && xml.name().toString() == "enclosure");
        if (current_element_is_enclosure) {
            installer_url = QUrl{xml.attributes().value("url").toString()};
            latest_version = xml.attributes().value("sparkle:version").toString();
            break;
        }
    }

    reply->deleteLater();

    //
    // compare versions and prompt user to update if newer
    //

    if (latest_version.isEmpty()) {
        QMessageBox::critical(parent_widget_, "Updater", "Failed to get latest version");
        return;
    }

    const auto current_version = QVersionNumber::fromString(QCoreApplication::applicationVersion());
    if (current_version < QVersionNumber::fromString(latest_version)) {
        const auto flags = QMessageBox::Yes | QMessageBox::No;
        const auto message = QString{"A new version (%1) is available, would you like to download an install it?"}.arg(latest_version);
        const QMessageBox::StandardButton response = QMessageBox::information(parent_widget_, "Update Available", message, flags);

        const bool wants_to_update = response == QMessageBox::Yes;
        if (wants_to_update) download_and_run_installer(installer_url);
    }
}

auto Updater::download_and_run_installer(const QUrl &url) -> void {
    const auto request = QNetworkRequest{url};
    installer_download_reply_ = network_manager_->get(request);

    auto *progress_dialog = new QProgressDialog{"Downloading update...", "Cancel", 0, 100, parent_widget_};
    progress_dialog->setAttribute(Qt::WA_DeleteOnClose);
    progress_dialog->setWindowModality(Qt::WindowModal);
    progress_dialog->show();

    connect(installer_download_reply_, &QNetworkReply::downloadProgress, this, [=](qint64 bytes_received, qint64 bytes_total) -> void {
        if (bytes_total <= 0) return;

        const qint64 download_percentage = (bytes_received * 100) / bytes_total;
        progress_dialog->setValue(static_cast<int>(download_percentage));
    });

    connect(installer_download_reply_, &QNetworkReply::finished, this, &Updater::on_installer_download_finished);
    connect(progress_dialog, &QProgressDialog::canceled, installer_download_reply_, &QNetworkReply::abort);
}

auto Updater::on_installer_download_finished() -> void {
    const bool download_reply_has_error = installer_download_reply_->error() != QNetworkReply::NoError;
    if (download_reply_has_error) {
        const bool user_cancelled_download = installer_download_reply_->error() == QNetworkReply::OperationCanceledError;
        if (!user_cancelled_download) {
            const auto message = QString{"Failed to download the update: %1"}.arg(installer_download_reply_->errorString());
            QMessageBox::critical(parent_widget_, "Updater", message);
        }

        installer_download_reply_->deleteLater();
        return;
    }

    //
    // attempt to run the downloaded installer
    //

    const QString filename = QFileInfo{installer_download_reply_->url().path()}.fileName();
    const QString temporary_installer_path = QDir::tempPath() + "/" + filename;
    if (auto file = QFile{temporary_installer_path}; file.open(QIODevice::WriteOnly)) {
        file.write(installer_download_reply_->readAll());
        file.close();

        QStringList args;
        args << "/SILENT" << "/DIR=" + QDir::toNativeSeparators(QCoreApplication::applicationDirPath());

        // here we run the installer and quit our application, so that it can get replaced and launched by the installer
        QProcess::startDetached(temporary_installer_path, args);
        QCoreApplication::quit();
    } else {
        QMessageBox::critical(parent_widget_, "Updater", "Error: could not save the downloaded installer");
    }

    installer_download_reply_->deleteLater();
}

} // namespace ui
