#include "ui/window.hpp"
#include <iostream>

// TODO make the login control buttons the same size
// TODO make it more obvious what game ur logging into
// TODO make uia multi threaded. IMPORTANT uia needs to be initialized once per thread
// FIXME theres a strange bug that occurs rarely, where if you delete an account, and i think its if you close the program instantly, the
// file actually doesnt save, and the account remains

// TODO create installer that will unzip and install the files, winsparkle will be monitoring for this, and then downloading the installer
// from a github release and running it
#include <winsparkle/winsparkle.h>

auto main(int argc, char *argv[]) -> int {
    auto app = QApplication{argc, argv};

    app.setApplicationName("friede");
    app.setApplicationVersion("1.0.0");

    win_sparkle_set_appcast_url("https://raw.githubusercontent.com/xi94/friede/main/appcast.xml");
    win_sparkle_init();

    // TODO pls make the deps here, and then pass them to window after they are verified, they are giga hidden right now
    auto window = ui::Window{};
    window.show();

    const int exec = app.exec();
    win_sparkle_cleanup();

    return exec;
}
