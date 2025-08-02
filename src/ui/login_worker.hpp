// =================================================================================
// ui/login_worker.hpp
// =================================================================================

#pragma once

#include <QObject>
#include <QString>

#include "riot/client.hpp"

/**
 * @class Login_Worker
 * @brief Executes the Riot Client login sequence on a background thread.
 */
class Login_Worker final : public QObject {
    Q_OBJECT

  public:
    explicit Login_Worker(QObject *parent = nullptr);

  public slots:
    /**
     * @brief Runs the login automation flow.
     * @param game The target game to launch.
     * @param username The account username.
     * @param password The account password.
     */
    auto do_login(riot::Game game, const QString &username, const QString &password) -> void;

  signals:
    /**
     * @brief Signals a change in the login status message.
     * @param message The progress message to display.
     */
    auto progress_updated(const QString &message) -> void;

    /**
     * @brief Signals the completion of the login attempt.
     * @param success True if the login succeeded, false otherwise.
     * @param message A final status or error message.
     */
    auto login_finished(bool success, const QString &message) -> void;
};
