#include <talon/talon.hpp>

#include <format>
#include <string_view>

namespace fs = std::filesystem;
using namespace std::literals;

auto generate_moc_files(std::initializer_list<std::string_view> relative_paths) -> void
{
    fs::create_directories("moc");

    for (const auto &path : relative_paths) {
        const auto input_path = fs::path{path};

        fs::path output_filename("moc_" + input_path.filename().string());
        output_filename.replace_extension(".cpp");

        const auto relative_output_path = fs::path{"moc"} / output_filename;
        const fs::path include_prefix = input_path.parent_path();

        constexpr auto qt_include_path = "C:\\dev\\qt-build\\qtbase\\include"sv;
        const auto command =
            std::format(R"(moc -p {} -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB -I{} -Isrc src\{} > {})", include_prefix.generic_string(),
                        qt_include_path, std::string(path), relative_output_path.generic_string());

        std::printf("%s -> %s\n", path.data(), relative_output_path.string().data());
        std::system(command.data());
    }
}

auto copy_resources_to_build(talon::workspace &workspace) -> void
{
    const auto build = workspace.root / "build";
    fs::create_directories(build);

    const auto banners_dst = build / "banners";
    const auto banners_src = workspace.root / "resources" / "banners";
    if (fs::exists(banners_src)) fs::copy(banners_src, banners_dst, fs::copy_options::recursive | fs::copy_options::overwrite_existing);

    const auto icons_dst = build / "icons";
    const auto icons_src = workspace.root / "resources" / "icons";
    if (fs::exists(icons_src)) fs::copy(icons_src, icons_dst, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
}

auto maybe_deploy_qt_deps(const bool needs_deployment) -> void
{
    if (!needs_deployment) return;

    // TODO why are dont hardcoded :D
    constexpr const char *output_path = "build\\friede.exe";
    std::system(std::format("windeployqt {} --release", output_path).data());
}

auto add_build_dependencies(const talon::arguments &args, talon::workspace *workspace) -> void
{
    workspace->add_includes("src");
    workspace->add_source_directories("src", "moc");
    workspace->add_libraries("ole32", "comsuppw");
    workspace->set_windows_resource_file("resources/app.rc");

    //
    // thirdparty
    //

    workspace->add_includes("thirdparty/include");
    workspace->add_library_includes("thirdparty/lib");

    workspace->add_libraries("tomlplusplus");

    //
    // qt (pls move this to thirdparty if possible :D)
    //

    workspace->add_includes("C:\\dev\\qt-build\\qtbase\\include");
    workspace->add_includes("C:\\dev\\qt-build\\qtbase\\include\\QtCore");
    workspace->add_includes("C:\\dev\\qt-build\\qtbase\\include\\QtWidgets");
    workspace->add_includes("C:\\dev\\qt-build\\qtbase\\include\\QtGui");

    workspace->add_libraries("Qt6Widgets", "Qt6Core", "Qt6Gui", "Qt6Network");
    workspace->add_library_includes("C:\\dev\\qt-build\\qtbase\\lib");
}

auto set_build_options(const talon::arguments &args, talon::workspace *workspace) -> void
{
    auto *opts = &workspace->options;

    opts->compiler = talon::msvc;
    opts->cpp_version = talon::std_23;

    opts->warnings_are_errors = true;
    opts->enable_recommended_warnings();

    opts->print_build_script = args.contains("print");

    if (args.contains("release")) {
        opts->optimization = talon::speed;
        workspace->add_linker_flags("/SUBSYSTEM:WINDOWS", "/ENTRY:mainCRTStartup");
    } else {
        opts->debug_symbols = true;
        workspace->add_linker_flags("/DEBUG");
    }
}

auto build(talon::arguments args) -> void
{
    auto workspace = talon::workspace{};

    add_build_dependencies(args, &workspace);
    set_build_options(args, &workspace);

    generate_moc_files({"ui/window.hpp", "ui/updater.hpp", "ui/login_worker.hpp", "ui/add_account_dialog.hpp", "ui/theme_editor.hpp"});

    // FIXME yeah this doesnt really work if the build folder is there lol
    const bool needs_qt_deps = !fs::exists(workspace.root / "build");
    copy_resources_to_build(workspace);

    workspace.build();
    maybe_deploy_qt_deps(needs_qt_deps);
}
