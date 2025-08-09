#include "ui/window.hpp"

#include <QApplication>
#include <QColor>
#include <QPalette>
#include <QStyleFactory>

auto force_dark_mode(QApplication *app) -> void
{
    app->setStyle(QStyleFactory::create("Fusion"));

    auto palette = QPalette{};
    palette.setColor(QPalette::Window, QColor(53, 53, 53));
    palette.setColor(QPalette::WindowText, Qt::white);
    palette.setColor(QPalette::Base, QColor(25, 25, 25));
    palette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    palette.setColor(QPalette::ToolTipBase, Qt::white);
    palette.setColor(QPalette::ToolTipText, Qt::white);
    palette.setColor(QPalette::Text, Qt::white);
    palette.setColor(QPalette::Button, QColor(53, 53, 53));
    palette.setColor(QPalette::ButtonText, Qt::white);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(127, 127, 127));
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Link, QColor(42, 130, 218));
    palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    palette.setColor(QPalette::HighlightedText, Qt::black);

    app->setPalette(palette);
}

auto main(int argc, char *argv[]) -> int
{
    auto app = QApplication{argc, argv};
    force_dark_mode(&app);

    app.setApplicationName("friede");
    app.setApplicationVersion("1.3.1");

    auto window = ui::Window{};
    window.show();

    return app.exec();
}
