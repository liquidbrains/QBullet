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
class QSystemTrayIcon;
class QMenu;

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
    virtual void loadConfig();
    virtual bool eventFilter(QObject *, QEvent *event);
    virtual void exit();
    virtual void closeEvent(QCloseEvent *);
    virtual void shitfuckshit();
private:
    Ui::Settings *ui;
    QSettings *settings;
    QMenu *clipboardMenu;
    QMenu *noteMenu;
    QMenu *addressMenu;
    QMenu *listMenu;
    QMenu *fileMenu;
    QMenu *linkMenu;
    QSystemTrayIcon *tray;
    QMenu *menu;
    QNetworkAccessManager *networkaccess;
    QObject *foo;
    bool showResult;
    bool exitClicked;
    void addAuthentication(QNetworkRequest &request);
    void processDevices(const QJsonValue &);
    void processSharedDevices(const QJsonValue &);
    void processResponse(const QJsonObject &);
protected slots:
    void replyReceived(QNetworkReply* reply);
    void proxyAuthenticationRequired ( const QNetworkProxy & proxy, QAuthenticator * authenticator );
    void sendNote(QString deviceDescription, int id);
    void sendClipboard(QString deviceDescription, int id);
    void sendAddress(QString deviceDescription, int id);
    void sendList(QString deviceDescription, int id);
    void sendLink(QString deviceDescription, int id);
    void sendFile(QString deviceDescription, int id);
    void handleError(int errorCode, QString serverMessage);
    void about();

};

#endif // SETTINGS_H
