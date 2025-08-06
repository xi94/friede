// =================================================================================
// core/config.hpp
// =================================================================================

#pragma once

#include <QObject>
#include <QString>

#define TOML_EXCEPTIONS 0
#define TOML_HEADER_ONLY 1
#include <toml++/toml.hpp>

namespace core {

/**
 * @class Config
 * @brief Base class for managing reading from and writing to TOML configuration files.
 */
class Config : public QObject {
    Q_OBJECT

  public:
    virtual ~Config() = default;

    /// @brief Returns the full path to the configuration file.
    auto path() const -> const QString &;

    /// @brief Returns the full path to the configuration directory.
    auto get_config_directory_path() const -> QString;

  protected:
    explicit Config(const QString file_name, QObject *parent = nullptr);

    /// @brief Reads the TOML file from disk into a toml::table.
    auto load() const -> toml::table;

    /// @brief Writes a toml::table to the configuration file on disk.
    auto save(const toml::table &config) -> bool;

  private:
    /// @brief Gets the path to the config file, creating it if it doesn't exist.
    auto initialize_config_path(const QString file_name) -> QString;

    QString config_path_;
};

} // namespace core
