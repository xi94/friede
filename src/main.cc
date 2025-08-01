#include "ui/window.hpp"
#include <QApplication>
#include <QTimer>

#include <QDebug>
#include <iostream>

auto main(int argc, char *argv[]) -> int {
    auto app = QApplication{argc, argv};

    app.setApplicationName("friede");
    app.setApplicationVersion("1.1.1");

    auto window = ui::Window{};
    window.show();

    const int exec = app.exec();

    return exec;
}
