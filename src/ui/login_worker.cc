// =================================================================================
// ui/login_worker.cc
// =================================================================================

#include "login_worker.hpp"

#include "riot/client.hpp"

#include <chrono>
#include <thread>

Login_Worker::Login_Worker(QObject *parent)
    : QObject(parent) {}

auto Login_Worker::do_login(riot::Game game, const QString &username, const QString &password) -> void {
    auto client_result = riot::Client::create();
    if (!client_result) {
        const auto error_string = QString::fromStdString(std::string(riot::client_error_as_string(client_result.error())));
        emit login_finished(false, "Error: Failed to initialize client. Reason: " + error_string);
        return;
    }

    auto client = std::move(*client_result);
    if (client.is_alive()) {
        emit progress_updated("Closing client...");

        if (const auto kill_result = client.kill(); !kill_result) {
            const auto error_string = QString::fromStdString(std::string(riot::client_error_as_string(kill_result.error())));
            emit login_finished(false, "Failed to kill existing client (Reason: " + error_string + ")");
            return;
        }

        // give windows a second to handle killing the process
        std::this_thread::sleep_for(std::chrono::milliseconds{500});
    }

    emit progress_updated("Starting client...");
    if (const auto start_result = client.start(game); !start_result) {
        const auto error_string = QString::fromStdString(std::string(riot::client_error_as_string(start_result.error())));
        emit login_finished(false, "Failed to start client (Reason: " + error_string + ")");
        return;
    }

    constexpr auto timeout = std::chrono::seconds{20};
    emit progress_updated("Waiting for client window...");
    if (!client.connect_to_window(timeout)) {
        emit login_finished(false, "Failed to find client window (Reason: Timed out)");
        return;
    }

    emit progress_updated("Setting credentials...");
    const auto login_result = client.login(username.toStdString(), password.toStdString());
    if (!login_result) {
        const auto error_message = riot::client_error_as_string(login_result.error());
        emit login_finished(false, QString::fromStdString(std::string(error_message)));
        return;
    }

    emit login_finished(true, "Login successful!");
}
