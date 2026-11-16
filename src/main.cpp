#include "MainWindow.h"
#include "Theme.h"

#include <QApplication>
#include <QPalette>
#include <QStyleFactory>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("KernelMem");
    app.setStyle(QStyleFactory::create("Fusion"));

    QPalette pal;
    pal.setColor(QPalette::Window, Theme::bg);
    pal.setColor(QPalette::WindowText, Theme::ink);
    pal.setColor(QPalette::Base, Theme::surface);
    pal.setColor(QPalette::AlternateBase, Theme::bg);
    pal.setColor(QPalette::Text, Theme::ink);
    pal.setColor(QPalette::Button, Theme::surface);
    pal.setColor(QPalette::ButtonText, Theme::ink);
    pal.setColor(QPalette::Highlight, Theme::accent);
    pal.setColor(QPalette::HighlightedText, Qt::white);
    pal.setColor(QPalette::ToolTipBase, Theme::pcb);
    pal.setColor(QPalette::ToolTipText, Qt::white);
    app.setPalette(pal);
    app.setFont(Theme::ui(13));
    app.setStyleSheet(Theme::styleSheet());

    MainWindow w;
    w.show();
    return app.exec();
}
