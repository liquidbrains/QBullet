#include "settings.h"
#include "ui_settings.h"

#include <iostream>
#include <QtNetwork/QNetworkProxy>
#include <QtNetwork/QAuthenticator>
#include <QtNetwork/QNetworkAccessManager>
#include <QMessageBox>
#include <QNetworkReply>
#include <QSettings>
#include <QCloseEvent>
#include <QJsonValue>
#include <QFileDialog>
#include <QJsonObject>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QJsonArray>
#include <QJsonDocument>
#include <bullet.h>

#include <logindialog.h>

Settings::Settings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Settings),
    settings(new QSettings(__FILE__,"QBullet",parent)),
    tray(new QSystemTrayIcon()),
    menu(new QMenu(parent)),
    networkaccess(new QNetworkAccessManager(parent)),
    foo(NULL),
    showResult(true),
    exitClicked(false)
{
    ui->setupUi(this);

    //connect(tray,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),menu,SLOT(internalDelayedPopup()));
    connect(networkaccess, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyReceived(QNetworkReply*)));
    connect(networkaccess, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)), this,SLOT(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)));
    connect(ui->cbSystemProxy,SIGNAL(stateChanged(int)),this,SLOT(systemProxyChecked(int)));
    connect(ui->btnTest,SIGNAL(clicked()),this,SLOT(executeTest()));

    tray->setIcon(QIcon(":icons/QBullet.png"));
    tray->show();
    setWindowIcon(QIcon(":icons/QBullet.png"));
    QApplication::setWindowIcon(QIcon(":icons/QBullet.png"));

    noteMenu = menu->addMenu("&Note");
    addressMenu = menu->addMenu("&Address");
    listMenu = menu->addMenu("&List");
    fileMenu = menu->addMenu("&File");
    linkMenu = menu->addMenu("L&ink");

    menu->addSeparator();
    menu->addAction("&Settings",this,SLOT(show()));
    menu->addAction("&Update Devices",this,SLOT(getDevices()));
    menu->addSeparator();
    menu->addAction("&Exit",this,SLOT(exit()));

    tray->setContextMenu(menu);

    /*if (settings->value("apiKey","").toString().isEmpty())
    {
        show();
    }else
    {
        this->getDevices();
    }*/

}

Settings::~Settings()
{
    delete ui;
}

void Settings::replyReceived(QNetworkReply* reply)
{
    QByteArray data = reply->readAll();
    if (reply->error() != 0)
    {
        show();
    }

    handleResponse(data);

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
    settings->setValue("proxyServerType",ui->cmbProxyType->currentText());
    /**
     * TODO: Save Screen values.
     * Set Proxy
     * Save Devices
     */

    hide();
    loadConfig();
}

void Settings::loadConfig()
{
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
#ifdef BLARGH
        QNetworkProxyFactory::setUseSystemConfiguration(false);
        QNetworkProxy *proxy = NULL;
        switch ("proxyServerType",settings->value("proxyServerType","No Proxy"))
        {
        case "No Proxy":
            break;
        case "Default":
        case "Http":
        case "Socks 5":
        case "Http Caching"
        default:
            QMessageBox::warning(this,"Error","Invalid proxy type selected: "+settings->value("proxyServerType","No Proxy"));
            break;

        }

        (); /// Crap... Need to have a proxy type.
#endif
    }
}

void Settings::reject()
{
    hide();
}

void Settings::exit()
{
    exitClicked = true;
    tray->setVisible(false);
    QApplication::exit();
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
    ui->cmbProxyType->setEnabled(false);
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
//    QMessageBox::information(0,"Reply Received",response);
    qDebug() << response;

    QJsonDocument jsod(QJsonDocument::fromJson(response));

    QJsonObject jsoo(jsod.object());

    processDevices(jsoo["devices"]);
    processSharedDevices(jsoo["shared_devices"]);
    processResponse(jsoo["created"]);

    qDebug() << "JSOO has " << jsoo.keys().count() << " keys.";
    if (jsoo.keys().count() > 0)
    {
        qDebug() << "Key 0: " << jsoo.keys().begin().operator *();
    }
}

bool Settings::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::Close)
    {
        if (exitClicked)
        {
            event->accept();
        }else
        {
            event->ignore();
        }

        return true;
    }

    return false;
}

void Settings::closeEvent(QCloseEvent *event)
{
    if (exitClicked)
    {
        event->accept();
    }else
    {
        reject();
        event->ignore();
    }

}

void Settings::processDevices(const QJsonValue &response)
{
    if (response.isArray() == false|| response.toArray().size() == 0)
        return;

    QJsonArray devices(response.toArray());

    DELETE_IF_NOT_NULL(foo);
    foo = new QObject(this);

    noteMenu->clear();
    addressMenu->clear();
    listMenu->clear();
    fileMenu->clear();
    linkMenu->clear();

    for (int i = 0; i < devices.count(); ++i)
    {
        QJsonObject device(devices[i].toObject());

        QString deviceDescription("Your "+device["extras"].toObject()["model"].toString()+"("+QString::number((int)device["id"].toDouble())+")");
        Bullet *bullet = new Bullet(deviceDescription,(int)device["id"].toDouble(),foo);
        connect(bullet,SIGNAL(sendAddress(QString,int)),this,SLOT(sendAddress(QString,int)));
        connect(bullet,SIGNAL(sendNote(QString,int)),this,SLOT(sendNote(QString,int)));
        connect(bullet,SIGNAL(sendList(QString,int)),this,SLOT(sendList(QString,int)));
        connect(bullet,SIGNAL(sendLink(QString,int)),this,SLOT(sendLink(QString,int)));
        connect(bullet,SIGNAL(sendFile(QString,int)),this,SLOT(sendFile(QString,int)));

        noteMenu->addAction(deviceDescription,bullet,SLOT(sendNote()));
        addressMenu->addAction(deviceDescription,bullet,SLOT(sendAddress()));
        listMenu->addAction(deviceDescription,bullet,SLOT(sendList()));
        fileMenu->addAction(deviceDescription,bullet,SLOT(sendFile()));
        linkMenu->addAction(deviceDescription,bullet,SLOT(sendLink()));
    }
}

void Settings::processSharedDevices(const QJsonValue &response)
{
    if (response.isArray() == false|| response.toArray().size() == 0)
        return;

    QJsonArray devices(response.toArray());

    noteMenu->addSeparator();
    addressMenu->addSeparator();
    listMenu->addSeparator();
    fileMenu->addSeparator();
    linkMenu->addSeparator();

    for (int i = 0; i < devices.count(); ++i)
    {
        QJsonObject device(devices[i].toObject());

        QString deviceDescription(device["owner_name"].toString()+"'s "+device["extras"].toObject()["model"].toString()+"("+QString::number((int)device["id"].toDouble())+")");
        Bullet *bullet = new Bullet(deviceDescription,(int)device["id"].toDouble(),foo);
        connect(bullet,SIGNAL(sendAddress(QString,int)),this,SLOT(sendAddress(QString,int)));
        connect(bullet,SIGNAL(sendNote(QString,int)),this,SLOT(sendNote(QString,int)));
        connect(bullet,SIGNAL(sendList(QString,int)),this,SLOT(sendList(QString,int)));
        connect(bullet,SIGNAL(sendLink(QString,int)),this,SLOT(sendLink(QString,int)));
        connect(bullet,SIGNAL(sendFile(QString,int)),this,SLOT(sendFile(QString,int)));

        noteMenu->addAction(deviceDescription,bullet,SLOT(sendNote()));
        addressMenu->addAction(deviceDescription,bullet,SLOT(sendAddress()));
        listMenu->addAction(deviceDescription,bullet,SLOT(sendList()));
        fileMenu->addAction(deviceDescription,bullet,SLOT(sendFile()));
        linkMenu->addAction(deviceDescription,bullet,SLOT(sendLink()));
    }
}

void Settings::processResponse(const QJsonValue &response)
{
    if (response.isNull())
        return;

}
void Settings::sendNote(QString /*deviceDescription*/, int /*id*/)
{

}

void Settings::sendAddress(QString /*deviceDescription*/, int /*id*/)
{

}

void Settings::sendList(QString /*deviceDescription*/, int /*id*/)
{

}

void Settings::sendLink(QString /*deviceDescription*/, int /*id*/)
{

}

void Settings::sendFile(QString deviceDescription, int id)
{
    try{
    QString fileName(QFileDialog::getOpenFileName(menu,"Select file to be sent to: "+deviceDescription));

    if (fileName.isEmpty())
    {
        tray->showMessage("No File selected","You did not select a file.  Please try again.",QSystemTrayIcon::Warning);
        return;
    }

    QFile *file = new QFile(fileName);
    if (file->size() > 10*1024*1024)
    {
        tray->showMessage("File to big",fileName+" is to big (max 10MB)",QSystemTrayIcon::Critical);
        delete file;
        return;
    }

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart textPart;
    textPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"device\""));
    textPart.setBody(QString::number(id).toLatin1());

    QHttpPart typePart;
    typePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"type\""));
    typePart.setBody("file");

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpeg"));
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"file\""));

    file->open(QIODevice::ReadOnly);
    filePart.setBodyDevice(file);
    file->setParent(multiPart); // we cannot delete the file now, so delete it with the multiPart

    multiPart->append(textPart);
    multiPart->append(typePart);
    multiPart->append(filePart);

    QNetworkRequest request(QUrl("https://www.pushbullet.com/api/pushes"));
    addAuthentication(request);

    QNetworkAccessManager manager;
    QNetworkReply *reply = manager.post(request, multiPart);
    multiPart->setParent(reply); // delete the multiPart with the reply
    if(reply->waitForReadyRead(-1))
    {
        qDebug()<< reply->error();
        qDebug()<<reply->errorString();
        qDebug()<< reply->readAll();
    }
    qDebug()<< reply->error();
    qDebug()<<reply->errorString();
    }catch (std::exception ex)
    {
        qDebug() << ex.what();
    }
    catch (...)
    {
        std::cerr << "well fuck."<<std::endl;
    }
    return;
}
