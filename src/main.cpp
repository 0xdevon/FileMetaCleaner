#include <QApplication>
#include <QIcon>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("FileMeta Cleaner");
    QApplication::setApplicationDisplayName("FileMeta Cleaner");
    QApplication::setOrganizationName("devonchan");
    app.setWindowIcon(QIcon(":/icons/AppIcon.png"));

    MainWindow window;
    window.setWindowIcon(QIcon(":/icons/AppIcon.png"));
    window.resize(600, 400);
    window.show();

    return app.exec();
}
