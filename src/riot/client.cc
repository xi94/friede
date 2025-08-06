// =================================================================================
// riot/client.cc
// =================================================================================

#include "client.hpp"

#include <array>
#include <expected>
#include <fstream>
#include <functional>
#include <memory>
#include <processthreadsapi.h>
#include <tlhelp32.h>
#include <windows.h>

using json = nlohmann::json;
using namespace std::string_view_literals;

namespace riot {

using Scoped_Handle = std::unique_ptr<void, decltype(&::CloseHandle)>;
static constexpr std::array<std::wstring_view, 6> RIOT_PROCESS_NAMES = {L"Riot Client.exe"sv,  L"RiotClientServices.exe"sv,
                                                                        L"RiotClientUx.exe"sv, L"RiotClientUxRender.exe"sv,
                                                                        L"LeagueClient.exe"sv, L"LeagueClientUx.exe"sv};

Client::Client(std::string client_path)
    : path_{std::move(client_path)}
{
}

auto Client::create() -> Result<Client>
{
    auto client_path_result = find_client_path();
    if (!client_path_result) { return std::unexpected(client_path_result.error()); }

    return Client(*client_path_result);
}

auto Client::connect_to_window(const std::chrono::seconds timeout) -> bool
{
    uia_ = std::make_unique<platform::UIA_Application>(L"Riot Client", timeout);
    return uia_->is_ready();
}

auto Client::start(Game game) -> Result<void>
{
    std::string command = "\"" + path_ + "\" --launch-product=" + std::string(get_game_parameter_id(game)) + " --launch-patchline=live";

    auto si = STARTUPINFOA{};
    auto pi = PROCESS_INFORMATION{};
    si.cb = sizeof(si);

    if (!CreateProcessA(nullptr, command.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
        return std::unexpected(Client_Error::Process_Creation_Failed);
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return {};
}

auto Client::kill() -> Result<void>
{
    bool process_killed = false;

    auto kill_result = for_each_riot_process([&process_killed](const PROCESSENTRY32W &process_entry) {
        auto process = Scoped_Handle{OpenProcess(PROCESS_TERMINATE, FALSE, process_entry.th32ProcessID), &::CloseHandle};
        if (!process) return true;

        if (TerminateProcess(process.get(), 1)) process_killed = true;
        return true;
    });

    if (!kill_result) return std::unexpected(kill_result.error());

    if (process_killed) {
        if (uia_) uia_.reset();
        return {};
    }

    return std::unexpected(Client_Error::Process_Termination_Failed);
}

auto Client::login(const std::string_view username, const std::string_view password, bool remember_me) -> Result<void>
{
    if (!is_ready()) { return std::unexpected(Client_Error::Automation_Failed); }

    const auto wide_username = std::wstring{username.begin(), username.end()};
    const auto wide_password = std::wstring{password.begin(), password.end()};

    if (!uia_->set_text_in_field(L"username", wide_username)) return std::unexpected(Client_Error::Automation_Failed);
    if (!uia_->set_text_in_field(L"password", wide_password)) return std::unexpected(Client_Error::Automation_Failed);
    if (!uia_->toggle_checkbox(L"remember-me", remember_me)) return std::unexpected(Client_Error::Automation_Failed);
    if (!uia_->set_focus_to_element(L"password")) return std::unexpected(Client_Error::Automation_Failed);
    if (!uia_->send_key_to_window(VK_RETURN)) return std::unexpected(Client_Error::Automation_Failed);

    return {};
}

auto Client::is_alive() const -> bool
{
    bool found = false;

    std::ignore = for_each_riot_process([&found](const PROCESSENTRY32W &) {
        found = true;
        return false;
    });

    return found;
}

auto Client::is_ready() const -> bool
{
    return uia_ && uia_->is_ready();
}

auto Client::find_client_path() -> Result<std::string>
{
    char *program_data_path = nullptr;
    size_t len = 0;
    if (_dupenv_s(&program_data_path, &len, "PROGRAMDATA") != 0 || !program_data_path) {
        return std::unexpected(Client_Error::Env_Var_Not_Found);
    }

    auto program_data_ptr = std::unique_ptr<char, decltype(&free)>(program_data_path, &free);

    const auto settings_path = std::string(program_data_path) + "\\Riot Games\\RiotClientInstalls.json";
    auto settings_file = std::ifstream(settings_path);
    if (!settings_file.is_open()) { return std::unexpected(Client_Error::Installs_Json_Not_Found); }

    try {
        auto data = json::parse(settings_file);
        if (data.contains("rc_default") && data["rc_default"].is_string()) { return data["rc_default"].get<std::string>(); }

    } catch (const json::parse_error &) {
        return std::unexpected(Client_Error::JSON_Parse_Failed);
    }

    return std::unexpected(Client_Error::RC_Default_Key_Not_Found);
}

auto Client::get_game_parameter_id(Game game) -> std::string_view
{
    switch (game) {
    case Game::Valorant: return "valorant"sv;
    case Game::Legends_of_Runeterra: return "bacon"sv;

    case Game::Teamfight_Tactics:
    case Game::League_of_Legends: return "league_of_legends"sv;
    }

    return ""sv;
}

auto Client::for_each_riot_process(const std::function<bool(const PROCESSENTRY32W &)> &callback) const -> Result<void>
{
    auto snapshot = Scoped_Handle{CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0), &::CloseHandle};
    if (snapshot.get() == INVALID_HANDLE_VALUE) return std::unexpected(Client_Error::Snapshot_Creation_Failed);

    PROCESSENTRY32W process_entry{};
    process_entry.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(snapshot.get(), &process_entry)) {
        do {
            for (const auto target_name : RIOT_PROCESS_NAMES) {
                if (target_name == process_entry.szExeFile) {
                    if (!callback(process_entry)) return {};
                    break;
                }
            }

        } while (Process32NextW(snapshot.get(), &process_entry));
    }

    return {};
}

} // namespace riot
