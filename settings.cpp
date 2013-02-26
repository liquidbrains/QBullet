#include "settings.h"
#include "ui_settings.h"

#include <QtNetwork/QNetworkProxy>
#include <QtNetwork/QAuthenticator>
#include <QtNetwork/QNetworkAccessManager>
#include <QMessageBox>
#include <QNetworkReply>
#include <QSettings>
#include <QJsonValue>
#include <logindialog.h>

Settings::Settings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Settings),
    tray(new QSystemTrayIcon(this)),
    menu(new QMenu(this)),
    networkaccess(new QNetworkAccessManager(this)),
    settings(new QSettings(__FILE__,"QBullet",this)),
    settingsAction(new QAction("&Settings",this)),
    exitAction(new QAction("&Exit",this))
{
    ui->setupUi(this);


    QNetworkProxyFactory::setUseSystemConfiguration(true);
    QNetworkProxyQuery npq(QUrl("https://www.pushbullet.com/api/pushes"));
    QList<QNetworkProxy> proxies = QNetworkProxyFactory::systemProxyForQuery(npq);

    if (proxies.count() > 0)
    {
        networkaccess->setProxy(proxies[0]);
    }

    QAction *test = new QAction ("&Test",this);

    connect(tray,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),menu,SLOT(internalDelayedPopup()));
    connect(networkaccess, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyReceived(QNetworkReply*)));
    connect(networkaccess, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)), this,SLOT(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)));
    connect(ui->cbSystemProxy,SIGNAL(stateChanged(int)),this,SLOT(systemProxyChecked(int)));
    connect(ui->btnTest,SIGNAL(clicked()),this,SLOT(executeTest()));

    tray->setIcon(QIcon(":icons/QBullet.png"));
    tray->show();
    setWindowIcon(QIcon(":icons/QBullet.png"));


    menu->addAction(settingsAction);
    connect(settingsAction,SIGNAL(triggered()),this,SLOT(show()));
    menu->addAction(exitAction);
    connect(exitAction,SIGNAL(triggered()),this,SLOT(close()));
    menu->addAction(test);
    connect(test,SIGNAL(triggered()),this,SLOT(test()));
    tray->setContextMenu(menu);

    if (settings->value("apiKey","").toString().isEmpty())
    {
        show();
    }else
    {
        this->test();
    }
}

Settings::~Settings()
{
    delete ui;
}

void Settings::replyReceived(QNetworkReply* reply)
{
    qDebug()<<"Response code: " << reply->error();
    QByteArray data = reply->readAll();

    qDebug()<<"Response text: " << data;
    /*for (QList<QByteArray>::const_iterator i = reply->rawHeaderList().begin(); i != reply->rawHeaderList().end(); ++i)
    {
        qDebug()<<"Raw Headers: "<< (QByteArray)(*i);
    }*/

    if (showResult)
    {
        /**
         * TODO Handle Responses.
         */
        handleResponse(data);

    }else
        if (reply->error() != 200)
        {
            show();
            QMessageBox::warning(this,"Error received","The following error was received from pushbullet: "+data);
        }else
        {
            tray->showMessage("Action Successfull.","Your request has succeeded.");
        }

    showResult = false;
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

void Settings::accept()
{
    settings->setValue("firststart",false);
    settings->setValue("apiKey",ui->txtAPIKey->text());

    /**
     * TODO: Save Screen values.
     * Set Proxy
     * Save Devices
     */

    hide();
}

void Settings::reject()
{
    hide();
}

void Settings::show()
{
    /**
     * TODO Populate GUI.
     */

    ui->cbSystemProxy->setChecked(settings->value("systemProxy",true).toBool());
    ui->txtAPIKey->setText(settings->value("apiKey","").toString());
    ui->txtProxyServer->setEnabled(settings->value("systemProxy",true).toBool() == false);
    ui->txtProxyUsername->setEnabled(settings->value("systemProxy",true).toBool() == false);
    ui->txtProxyPassword->setEnabled(settings->value("systemProxy",true).toBool() == false);

    QDialog::show();
}

void Settings::systemProxyChecked(int state)
{

    ui->txtProxyServer->setEnabled(state == Qt::Unchecked);
    ui->txtProxyUsername->setEnabled(state == Qt::Unchecked);
    ui->txtProxyPassword->setEnabled(state == Qt::Unchecked);
}

void Settings::executeTest()
{
    showResult = true;
    settings->setValue("apiKey",ui->txtAPIKey->text());
    test();
}

void Settings::test()
{
#ifndef QT_NO_SSL
    QUrl url("https://www.pushbullet.com/api/devices");
    QNetworkRequest qnr(url);
    QString header = "Basic ";
    header+=(settings->value("apiKey","").toString()+":").toLatin1().toBase64();
    qnr.setRawHeader(QString("Authorization").toLatin1(),header.toLatin1());
//    url.setUserName(settings->value("apiKey","").toString());
    networkaccess->get(qnr);

#else
    qDebug() << "SHITSHITSHIT No SSL!"
#endif
}


void Settings::handleResponse(QByteArray &response)
{
    QMessageBox::information(this,"Reply Received",response);
    QJsonValue jsov()
}
