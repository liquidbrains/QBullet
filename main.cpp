#include "settings.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("QBullet");
    a.setApplicationDisplayName("QBullet PushBullet client");
    a.setApplicationVersion("0.1");
    a.setOrganizationName("Wian Potgieter");
    Settings w;
//    w.show();
    
    return a.exec();
}
