#include "ui/window.hpp"

#include <QApplication>

auto main(int argc, char *argv[]) -> int
{
    auto app = QApplication{argc, argv};

    app.setApplicationName("friede");
    app.setApplicationVersion("1.3.0");

    auto window = ui::Window{};
    window.show();

    return app.exec();
}
