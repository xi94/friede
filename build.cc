#include <format>
#include <string_view>
#include <talon/talon.hpp>
#include <vector>

// Use the std::filesystem library for robust path manipulation
namespace fs = std::filesystem;
using namespace std::literals;

/**
 * @brief Runs Qt's Meta-Object Compiler (MOC) on a target header file.
 *
 * This function constructs and executes the command to generate a moc_*.cpp file
 * from a given header. The output is placed in the 'moc/' directory.
 *
 * @param target_file The path to the header file, relative to the 'src/' directory.
 * Example: "ui/window.hpp".
 */
void generate_moc_file(const std::string_view target_file) {
    const fs::path input_path(target_file);

    fs::path output_filename("moc_" + input_path.filename().string());
    output_filename.replace_extension(".cpp");

    const fs::path relative_output_path = fs::path("moc") / output_filename;
    const fs::path include_prefix = input_path.parent_path();

    constexpr auto qt_include_path = "C:\\dev\\qt-build\\qtbase\\include"sv;
    const auto command = std::format(R"(moc -p {} -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB -I{} -Isrc src\{} > {})",
                                     include_prefix.generic_string(), qt_include_path, target_file, relative_output_path.generic_string());

    std::printf("%s -> %s\n", std::string(target_file).c_str(), relative_output_path.string().c_str());
    std::system(command.c_str());
}

auto add_build_dependencies(const talon::arguments &args, talon::workspace *workspace) -> void {
    workspace->add_includes("src");
    workspace->add_all_build_files("src");
    workspace->add_all_build_files("moc");
    workspace->add_library_files("ole32", "comsuppw");

    //
    // thirdparty
    //

    workspace->add_includes("thirdparty/include");
    workspace->add_library_includes("thirdparty/lib");

    workspace->add_library_files("tomlplusplus", "winsparkle");

    //
    // qt (pls move this to thirdparty if possible :D)
    //

    workspace->add_includes("C:\\dev\\qt-build\\qtbase\\include");
    workspace->add_includes("C:\\dev\\qt-build\\qtbase\\include\\QtCore");
    workspace->add_includes("C:\\dev\\qt-build\\qtbase\\include\\QtWidgets");
    workspace->add_includes("C:\\dev\\qt-build\\qtbase\\include\\QtGui");

    workspace->add_library_files("Qt6Widgets", "Qt6Core", "Qt6Gui");
    workspace->add_library_includes("C:\\dev\\qt-build\\qtbase\\lib");
}

auto set_build_options(const talon::arguments &args, talon::workspace *workspace) -> void {
    auto *opts = &workspace->options;

    opts->compiler = talon::msvc;
    opts->cpp_version = talon::std_23;

    opts->warnings_are_errors = true;
    opts->enable_recommended_warnings();

    if (args.contains("release")) {
        opts->optimization = talon::speed;

        // build release without the debug console
        workspace->additional_linker_flags.push_back("/SUBSYSTEM:WINDOWS");
        workspace->additional_linker_flags.push_back("/ENTRY:mainCRTStartup");
    }
}

auto copy_resources_to_build(talon::workspace &workspace) -> void {
    const auto build = workspace.root / "build";
    fs::create_directories(build);

    const auto banners_dst = build / "banners";
    const auto banners_src = workspace.root / "resources" / "banners";
    if (fs::exists(banners_src)) fs::copy(banners_src, banners_dst, fs::copy_options::recursive | fs::copy_options::overwrite_existing);

    const auto icons_dst = build / "icons";
    const auto icons_src = workspace.root / "resources" / "icons";
    if (fs::exists(icons_src)) fs::copy(icons_src, icons_dst, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
}

auto maybe_deploy_qt_deps(const bool needs_deployment) -> void {
    if (!needs_deployment) return;

    // TODO why are dont hardcoded :D
    constexpr const char *output_path = "build\\friede.exe";
    std::system(std::format("windeployqt {} --release", output_path).data());
}

auto setup_moc_files() -> void {
    fs::create_directories("moc");
    generate_moc_file("ui/window.hpp");
    generate_moc_file("ui/login_worker.hpp");
    generate_moc_file("ui/add_account_dialog.hpp");
}

auto build(talon::arguments args) -> void {
    auto workspace = talon::workspace{};

    add_build_dependencies(args, &workspace);
    set_build_options(args, &workspace);

    // this needs to be done each time we compile
    setup_moc_files();

    // FIXME yeah this doesnt really work if the build folder is there lol
    const bool needs_qt_deps = !fs::exists(workspace.root / "build");
    copy_resources_to_build(workspace);

    workspace.build();
    maybe_deploy_qt_deps(needs_qt_deps);
}
