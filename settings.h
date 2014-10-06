#ifndef SETTINGS_H
#define SETTINGS_H

#define DELETE_IF_NOT_NULL(x) if (x != NULL){delete x;x = NULL;}

#include <QDialog>
#include <QSystemTrayIcon>
#include <networkaccessmanager.h>

class QNetworkAccessManager;
class QNetworkProxy;
class QNetworkReply;
class QAuthenticator;
class QNetworkRequest;
class QSettings;



class QMenu;
class Prompt;
class QMimeData;

namespace Ui {
class Settings;
}

#include <QMap>

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
    virtual void proxyTypeChanged(const QString &type);
    virtual void getDevices();
    virtual void executeTest();
    virtual void showPushes();
    virtual void handleResponse(QByteArray &response);
    virtual void loadConfig();
    virtual bool eventFilter(QObject *, QEvent *event);
    virtual void exit();
    virtual void closeEvent(QCloseEvent *);
    virtual void trayActivated(QSystemTrayIcon::ActivationReason);
    virtual void uploadProgress(qint64, qint64);
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
    NetworkAccessManager *networkaccess;
    QObject *foo;
    Prompt *prompt;
    bool showResult;
    bool exitClicked;
    QMap<QString,QString> devices;
    QString uploadFilePath;
    void addAuthentication(QNetworkRequest &request);
    void processDevices(const QJsonValue &);
    void processSharedDevices(const QJsonValue &);
    void processResponse(const QJsonObject &);
    const QString detectClipboardContents(const QMimeData &mimeData);
    void sendFilePrivate(const QString &  id, const QString fileName);
    bool proxyAuthenticationSupplied;
protected slots:
    void replyReceived(QNetworkReply* reply);
    void sendNote(QString deviceDescription, const QString & id);
    void sendClipboard(QString deviceDescription, const QString & id);
    void sendAddress(QString deviceDescription, const QString &  id);
    void sendList(QString deviceDescription, const QString &  id);
    void childKilled(QObject *child);
    void sendLink(QString deviceDescription, const QString &  id);
    void sendFile(QString deviceDescription, const QString &  id);
    void sendFilePartTwo(const QString &uploadURL, const QJsonObject &data, const QString &fileName);
    void sendText(const QString &  id, QString type, const QString title, QString contentType, const QString content);
    void renameClipboardMenu();
    void handleError(int errorCode, QString serverMessage);
    void about();

};

#endif // SETTINGS_H
