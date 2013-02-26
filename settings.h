#ifndef SETTINGS_H
#define SETTINGS_H

#define DELETE_IF_NOT_NULL(x) if (x != NULL){delete x;x = NULL;}

#include <QDialog>

class QNetworkAccessManager;
class QNetworkProxy;
class QNetworkReply;
class QAuthenticator;
class QNetworkRequest;
class QSettings;

namespace Ui {
class Settings;
}

class Settings : public QDialog
{
    Q_OBJECT
    
public:
    explicit Settings(QWidget *parent = 0);
    ~Settings();
public slots:
    virtual void accept();
    virtual void reject();
    virtual void show();
    virtual void systemProxyChecked(int);
    virtual void getDevices();
    virtual void executeTest();
    virtual void handleResponse(QByteArray &response);
private:
    Ui::Settings *ui;
    QSettings *settings;
    QAction *settingsAction;
    QAction *exitAction;
    QSystemTrayIcon *tray;
    QMenu *menu;
    QNetworkAccessManager *networkaccess;
    bool showResult;
    void addAuthentication(QNetworkRequest &request);
protected slots:
    void replyReceived(QNetworkReply* reply);
    void proxyAuthenticationRequired ( const QNetworkProxy & proxy, QAuthenticator * authenticator );
};

#endif // SETTINGS_H
