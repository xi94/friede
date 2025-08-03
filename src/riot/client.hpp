// =================================================================================
// riot/client.hpp
// =================================================================================

#pragma once

#include "platform/automation.hpp"
#include <chrono>
#include <expected>
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>

#include <tlhelp32.h>
#include <windows.h>

namespace riot {

/// @brief Represents the specific Riot Games that can be launched.
enum class Game { League_of_Legends = 0, Valorant = 1, Teamfight_Tactics = 2, Legends_of_Runeterra = 3 };

/// @brief A helper to ensure that an int32 index is within range of the enum
constexpr inline auto is_game_index_out_of_range(const int index) -> bool
{
    using E = Game;

    switch (static_cast<Game>(index)) {
    case E::League_of_Legends:
    case E::Valorant:
    case E::Teamfight_Tactics:
    case E::Legends_of_Runeterra: return false;
    default: return true;
    }
}

/// @brief Defines error codes for Client operations.
enum class Client_Error {
    None,
    Env_Var_Not_Found,
    Installs_Json_Not_Found,
    JSON_Parse_Failed,
    RC_Default_Key_Not_Found,
    Snapshot_Creation_Failed,
    Process_Creation_Failed,
    Process_Termination_Failed,
    Automation_Failed,
};

/// @brief Converts a Client_Error enum to a user-readable string.
constexpr inline auto client_error_as_string(Client_Error error) -> std::string_view
{
    using E = Client_Error;
    using namespace std::string_view_literals;

    switch (error) {
    case E::Env_Var_Not_Found: return "Could not find the PROGRAMDATA environment variable."sv;
    case E::Installs_Json_Not_Found: return "Could not find Riot's installation config file (RiotClientInstalls.json)."sv;
    case E::JSON_Parse_Failed: return "Failed to parse Riot's installation JSON file."sv;
    case E::RC_Default_Key_Not_Found: return "Could not find the 'rc_default' path in Riot's config."sv;
    case E::Snapshot_Creation_Failed: return "Failed to create a system process snapshot."sv;
    case E::Process_Creation_Failed: return "Failed to start the Riot Client process."sv;
    case E::Process_Termination_Failed: return "An existing Riot Client process could not be terminated."sv;
    case E::Automation_Failed: return "A UI automation step failed. The client may have updated or is not responding."sv;
    case E::None: return "No error."sv;
    default: return "An unknown client error occurred."sv;
    }
}

template <typename T>
using Result = std::expected<T, Client_Error>;

/**
 * @class Client
 * @brief Provides an interface for automating the Riot Games client.
 */
class Client {
  public:
    /// @brief Creates a Client instance by locating the Riot Client installation.
    [[nodiscard]] static auto create() -> Result<Client>;

    /// @brief Attaches to the main Riot Client window for UI automation.
    auto connect_to_window(std::chrono::seconds timeout = std::chrono::seconds(20)) -> bool;

    /// @brief Launches the Riot Client process for a specific game.
    auto start(Game game) -> Result<void>;

    /// @brief Terminates all running Riot Client processes.
    auto kill() -> Result<void>;

    /// @brief Executes the UI login sequence using the provided credentials.
    auto login(std::string_view username, std::string_view password, bool remember_me = false) -> Result<void>; // <-- Corrected return type

    /// @brief Checks if any Riot Client processes are currently running.
    [[nodiscard]] auto is_alive() const -> bool;

    /// @brief Checks if the client is attached to a window and ready for automation.
    [[nodiscard]] auto is_ready() const -> bool;

  private:
    explicit Client(std::string client_path);

    [[nodiscard]] static auto find_client_path() -> Result<std::string>;
    [[nodiscard]] static auto get_game_parameter_id(Game game) -> std::string_view;
    auto for_each_riot_process(const std::function<bool(const PROCESSENTRY32W &)> &callback) const -> Result<void>;

    std::string path_;
    std::unique_ptr<platform::UIA_Application> uia_;
};

} // namespace riot
