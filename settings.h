#ifndef SETTINGS_H
#define SETTINGS_H

#include <QDialog>

class QNetworkAccessManager;
class QNetworkProxy;
class QNetworkReply;
class QAuthenticator;

namespace Ui {
class Settings;
}

class Settings : public QDialog
{
    Q_OBJECT
    
public:
    explicit Settings(QWidget *parent = 0);
    ~Settings();
    
private:
    QSystemTrayIcon *tray;
    QMenu *menu;
    Ui::Settings *ui;
    QNetworkAccessManager *networkaccess;
protected:
    void replyReceived(QNetworkReply*);
    void proxyAuthenticationRequired ( const QNetworkProxy & proxy, QAuthenticator * authenticator );
};

#endif // SETTINGS_H
