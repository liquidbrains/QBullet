#include "settings.h"
#include "ui_settings.h"

#include <QtNetwork/QNetworkProxy>
#include <QtNetwork/QAuthenticator>
#include <QtNetwork/QNetworkAccessManager>
#include <QMessageBox>
#include <QNetworkReply>
#include <QSettings>
#include <QJsonValue>
#include <QJsonDocument>
#include <logindialog.h>

Settings::Settings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Settings),
    tray(new QSystemTrayIcon(this)),
    menu(new QMenu(this)),
    networkaccess(new QNetworkAccessManager(this)),
    settings(new QSettings(__FILE__,"QBullet",this)),
    settingsAction(new QAction("&Settings",this)),
    exitAction(new QAction("&Exit",this)),
    showResult(true)
{
    ui->setupUi(this);

    if (settings->value("systemProxy",true).toBool())
    {
        QNetworkProxyFactory::setUseSystemConfiguration(true);
        QNetworkProxyQuery npq(QUrl("https://www.pushbullet.com/api/pushes"));
        QList<QNetworkProxy> proxies = QNetworkProxyFactory::systemProxyForQuery(npq);

        if (proxies.count() > 0)
        {
            networkaccess->setProxy(proxies[0]);
        }
    }else
    {
        QNetworkProxyFactory::setUseSystemConfiguration(false);
        //QNetworkProxy proxy() /// Crap... Need to have a proxy type.
    }

    QAction *test = new QAction ("&Update Devices",this);

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
    connect(test,SIGNAL(triggered()),this,SLOT(getDevices()));
    tray->setContextMenu(menu);

    if (settings->value("apiKey","").toString().isEmpty())
    {
        show();
    }else
    {
        this->getDevices();
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
        if (reply->error() != 0)
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
    DELETE_IF_NOT_NULL(login);
}

void Settings::accept()
{
    settings->setValue("firststart",false);
    settings->setValue("apiKey",ui->txtAPIKey->text());
    settings->setValue("systemProxy",ui->cbSystemProxy->checkState()== Qt::Checked);
    settings->setValue("proxyUser",ui->txtProxyUsername->text());
    settings->setValue("proxyPassword",ui->txtProxyPassword->text());
    settings->setValue("proxyServer",ui->txtProxyServer->text());
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
    ui->txtProxyServer->setText(settings->value("proxyServer","").toString());
    ui->txtProxyUsername->setEnabled(settings->value("systemProxy",true).toBool() == false);
    ui->txtProxyUsername->setText(settings->value("proxyUser","").toString());
    ui->txtProxyPassword->setEnabled(settings->value("systemProxy",true).toBool() == false);
    ui->txtProxyPassword->setText(settings->value("proxyPassword","").toString());

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
    getDevices();
}

void Settings::getDevices()
{
    QUrl url("https://www.pushbullet.com/api/devices");
    QNetworkRequest qnr(url);
    addAuthentication(qnr);
    networkaccess->get(qnr);
}

void Settings::addAuthentication(QNetworkRequest &request)
{
    QString header = "Basic ";
    header+=(settings->value("apiKey","").toString()+":").toLatin1().toBase64();
    request.setRawHeader(QString("Authorization").toLatin1(),header.toLatin1());
}

void Settings::handleResponse(QByteArray &response)
{
    QMessageBox::information(this,"Reply Received",response);
    /*QJsonDocument &jsod = QJsonDocument::fromJson(response);

    if (jsod.array()
*/
}
