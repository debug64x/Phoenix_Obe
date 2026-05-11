#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    app.setApplicationName("Phoenix Obe");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("Obe Office");

    MainWindow window;

    const auto args = QApplication::arguments();
    if (args.size() > 1)
        window.openPath(args[1]);

    window.show();
    return app.exec();
}
