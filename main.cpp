#include <QtWidgets>

auto main(int argc, char *argv[]) -> int
{
    QApplication app(argc, argv);
    QWidget window;
    window.show();
    window.setWindowTitle(QApplication::translate("helloworld", "Hello, World"));
    return QApplication::exec();
}
