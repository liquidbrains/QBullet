#include "settings.h"
#include "ui_settings.h"

#include <QtNetwork/QNetworkProxy>
#include <QtNetwork/QAuthenticator>
#include <QtNetwork/QNetworkAccessManager>
#include <logindialog.h>

Settings::Settings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Settings),
    tray(new QSystemTrayIcon(this)),
    menu(new QMenu(this)),
    networkaccess(new QNetworkAccessManager(this))
{
    ui->setupUi(this);

    connect(tray,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),menu,SLOT(internalDelayedPopup()));
    connect(networkaccess, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyReceived(QNetworkReply*)));
    connect(networkaccess, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)), this,SLOT(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)));
    tray->show();
    tray->setIcon(QIcon(":/QBullet.png"));
    setWindowIcon(QIcon(":/QBullet.png"));


    menu->addAction("ONE");
    menu->addAction("Two");
    tray->setContextMenu(menu);
}

Settings::~Settings()
{
    delete ui;
}

void Settings::replyReceived(QNetworkReply*)
{

}

void Settings::proxyAuthenticationRequired ( const QNetworkProxy & proxy, QAuthenticator * authenticator )
{
    LoginDialog * login = new LoginDialog((QWidget *)QObject::parent());

    login->setMessage(proxy.hostName());

    if (login->exec() == LoginDialog::Accepted)
    {
        authenticator->setPassword(login->password());

        authenticator->setUser(login->user());
    }
    /**
     * TODO Define this
     */
//    DELETE_IF_NOT_NULL(login);
}
