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
#include <QHeaderView>
#include <QJsonValue>
#include <QFileDialog>
#include <QApplication>
#include <QJsonObject>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QJsonArray>
#include <QMenu>
#include <QBuffer>
#include <QMimeDatabase>
#include <QSystemTrayIcon>
#include <QJsonDocument>
#include <QClipboard>
#include <QMimeData>
#include <QMap>
#include <qbulletproxyfactory.h>

#include <bullet.h>
#include <prompt.h>

#include <logindialog.h>

Settings::Settings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Settings),
    //settings(new QSettings(__FILE__,"QBullet",this)),
    settings(new QSettings(QCoreApplication::organizationName(),QCoreApplication::applicationName(),this)),
    tray(new QSystemTrayIcon()),
    menu(new QMenu(this)),
    networkaccess(new NetworkAccessManager(this)),
    foo(NULL),
    prompt(new Prompt(this)),
    showResult(false),
    exitClicked(false),
    devices(QMap<QString,QString>()),
    proxyAuthenticationSupplied(false)
{
    ui->setupUi(this);

    connect(networkaccess, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyReceived(QNetworkReply*)));
    //connect(networkaccess, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)), this,SLOT(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)));
    connect(ui->cmbProxyType,SIGNAL(currentIndexChanged(QString)),this,SLOT(proxyTypeChanged(QString)));
    connect(ui->btnTest,SIGNAL(clicked()),this,SLOT(executeTest()));

    tray->setIcon(QIcon(":icons/QBullet.png"));

    tray->show();
    connect(tray,SIGNAL(messageClicked()),this,SLOT(show()));

    setWindowIcon(QIcon(":icons/QBullet.png"));
    QApplication::setWindowIcon(QIcon(":icons/QBullet.png"));

    (clipboardMenu = menu->addMenu("Clipboard to"))->setEnabled(false);
    connect(menu,SIGNAL(aboutToShow()),this,SLOT(renameClipboardMenu()));
    (noteMenu = menu->addMenu("Note to"))->setEnabled(false);

    (addressMenu = menu->addMenu("Address to"))->setEnabled(false);
    (listMenu = menu->addMenu("List to"))->setEnabled(false);
    (fileMenu = menu->addMenu("File to"))->setEnabled(false);
    (linkMenu = menu->addMenu("Link to"))->setEnabled(false);

    menu->addSeparator();
    menu->addAction("&Settings",this,SLOT(show()));
    menu->addAction("&Update Device List",this,SLOT(getDevices()));
    menu->addSeparator();
    menu->addAction("&Exit",this,SLOT(exit()));

    QAction *tmpAction;
    ui->toolButton->addAction(tmpAction = new QAction("About QBullet",ui->toolButton));
    connect(tmpAction,SIGNAL(triggered()),this,SLOT(about()));
    ui->toolButton->addAction(tmpAction = new QAction("About QT",ui->toolButton));
    connect(tmpAction,SIGNAL(triggered()),qApp,SLOT(aboutQt()));
    tray->setContextMenu(menu);
    connect(tray,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),this,SLOT(trayActivated(QSystemTrayIcon::ActivationReason)));

    ui->cmbProxyType->addItem("None",QNetworkProxy::NoProxy);
    //ui->cmbProxyType->addItem("Default",QNetworkProxy::DefaultProxy);
    ui->cmbProxyType->addItem("HTTP",QNetworkProxy::HttpProxy);
    //ui->cmbProxyType->addItem("HTTP Caching",QNetworkProxy::HttpCachingProxy);
    ui->cmbProxyType->addItem("Socks 5",QNetworkProxy::Socks5Proxy);
}

Settings::~Settings()
{
    delete ui;
}

void Settings::replyReceived(QNetworkReply* reply)
{
    QByteArray data = reply->readAll();
    qDebug() << "In " << QString(__FUNCTION__);
    qDebug() << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug() << reply->error() << " " << reply->errorString();
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200)
    {
        qDebug() << "Reply data: "<<data;
        handleError(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(),reply->errorString());
    }else
    {
        handleResponse(data);
    }

    showResult = false;
    qDebug() << "Out " << QString(__FUNCTION__);
}

void Settings::trayActivated(QSystemTrayIcon::ActivationReason reason)
{
    qDebug() << "In " << QString(__FUNCTION__);
    if (reason == QSystemTrayIcon::DoubleClick)
    {
        show();
    }
}

void Settings::accept()
{
    qDebug() << "In " << QString(__FUNCTION__);
    if (ui->cmbProxyType->itemData(ui->cmbProxyType->currentIndex()).toInt() != -1 &&
            ui->cmbProxyType->itemData(ui->cmbProxyType->currentIndex()).toInt() != QNetworkProxy::NoProxy)
    {
        if (ui->txtProxyServer->text() == "" || ui->sbPort->value() <= 0)
        {
            QMessageBox::critical(this,"Invalid Proxy URL supplied!","Please supply a host name and port.\nThe default is 8080 or 3128.",QMessageBox::Ok);
            return;
        }
    }


    settings->setValue("firststart",false);
    settings->setValue("apiKey",ui->txtAPIKey->text());
    settings->beginGroup("proxy");
    settings->setValue("userName",ui->txtProxyUsername->text());
    settings->setValue("password",ui->txtProxyPassword->text());
    settings->setValue("hostName",ui->txtProxyServer->text());
    settings->setValue("port",ui->sbPort->value());
    settings->setValue("type",ui->cmbProxyType->itemData(ui->cmbProxyType->currentIndex()).toInt());
    settings->endGroup();

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
    qDebug() << "In " << QString(__FUNCTION__);

    if (settings->value("firststart",false).toBool() == false && settings->value("apiKey","").toString()!= "")
    {
        networkaccess->loadSettings();
        getDevices();
    }else
    {
        show();
        QMessageBox::information(this,"Welcome to QBulet","Welcome to QBullet.  Please provide your API Key that you can retrieve from your accounts page on PushBullet.com",QMessageBox::Ok);
    }
}

void Settings::reject()
{
    qDebug() << "In " << QString(__FUNCTION__);
    hide();
}

void Settings::exit()
{
    qDebug() << "In " << QString(__FUNCTION__);
    exitClicked = true;
    tray->setVisible(false);
    QApplication::exit();
}

void Settings::uploadProgress(qint64, qint64)
{

}


void Settings::show()
{
    qDebug() << "In " << QString(__FUNCTION__);
    /**
     * TODO Populate GUI.
     */

    ui->txtAPIKey->setText(settings->value("apiKey","").toString());

    settings->beginGroup(QLatin1String("proxy"));
    ui->txtProxyServer->setEnabled(settings->value("type",QNetworkProxy::NoProxy).toInt() != QNetworkProxy::NoProxy);
    ui->txtProxyServer->setText(settings->value("hostName","").toString());
    ui->sbPort->setValue(settings->value("port",8080).toInt());
    ui->sbPort->setEnabled(settings->value("type",QNetworkProxy::NoProxy).toInt() != QNetworkProxy::NoProxy);

    ui->txtProxyUsername->setText(settings->value("userName","").toString());
    ui->txtProxyPassword->setText(settings->value("password","").toString());



    QString proxyType;
    switch(settings->value("type",QNetworkProxy::NoProxy).toInt())
    {
    case QNetworkProxy::HttpProxy:
        proxyType = "HTTP";
        break;
    case QNetworkProxy::DefaultProxy:
        proxyType = "Default";
        break;
    case QNetworkProxy::Socks5Proxy:
        proxyType = "Socks 5";
        break;
    case QNetworkProxy::HttpCachingProxy:
        proxyType = "HTTP Caching";
        break;
    case QNetworkProxy::FtpCachingProxy:
        proxyType = "FTP Caching";
        break;
    default:
        proxyType = "None";
        break;
    }

    settings->endGroup();

    ui->cmbProxyType->setCurrentText(proxyType);

    QDialog::show();
}

void Settings::proxyTypeChanged(const QString &type)
{
    qDebug() << "In " << QString(__FUNCTION__);
    ui->txtProxyServer->setEnabled(type != "System Proxy" && type != "None");
    ui->sbPort->setEnabled(type != "System Proxy" && type != "None");
}

void Settings::executeTest()
{
    qDebug() << "In " << QString(__FUNCTION__);
    showResult = true;
    settings->setValue("apiKey",ui->txtAPIKey->text());

    loadConfig();
}

void Settings::getDevices()
{
    qDebug() << "In " << QString(__FUNCTION__);
    QUrl url("https://api.pushbullet.com/api/devices");
    QNetworkRequest qnr(url);
    addAuthentication(qnr);
    networkaccess->get(qnr);
}

void Settings::addAuthentication(QNetworkRequest &request)
{
    qDebug() << "In " << QString(__FUNCTION__);
    QString header = "Basic ";
    header+=(settings->value("apiKey","").toString()+":").toLatin1().toBase64();
    request.setRawHeader(QString("Authorization").toLatin1(),header.toLatin1());
#define QT_DECRYPT_SSL_TRAFFIC
}

void Settings::handleResponse(QByteArray &response)
{
    qDebug() << "In " << QString(__FUNCTION__);
//    QMessageBox::information(0,"Reply Received",response);
    qDebug() << response;

    QJsonDocument jsod(QJsonDocument::fromJson(response));

    QJsonObject jsoo(jsod.object());

    if (jsoo.contains("devices"))
    {
        devices.clear();
        processDevices(jsoo["devices"]);
        processSharedDevices(jsoo["shared_devices"]);
        if (showResult)
            tray->showMessage("Device list updated","The device list has been updated.",QSystemTrayIcon::Information,5000);

        QTableWidgetItem *h = new QTableWidgetItem("ID");
        ui->tblDevicesList->setHorizontalHeaderItem(0,h);
        h = new QTableWidgetItem("Owner");
        ui->tblDevicesList->setHorizontalHeaderItem(1,h);
        h = new QTableWidgetItem("Manufacturer");
        ui->tblDevicesList->setHorizontalHeaderItem(2,h);
        h = new QTableWidgetItem("Model");
        ui->tblDevicesList->setHorizontalHeaderItem(3,h);
        h = new QTableWidgetItem("Android Version");
        ui->tblDevicesList->setHorizontalHeaderItem(4,h);
        ui->tblDevicesList->doItemsLayout();
        ui->tblDevicesList->resizeColumnsToContents();
    }else
    {
        processResponse(jsoo);
    }
    qDebug() << "Out " << QString(__FUNCTION__);
}

bool Settings::eventFilter(QObject *, QEvent *event)
{
    qDebug() << "In " << QString(__FUNCTION__);
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
    qDebug() << "In " << QString(__FUNCTION__);
    if (exitClicked)
    {
        event->accept();
    }else
    {
        reject();
        event->ignore();
    }
}

const QString Settings::detectClipboardContents(const QMimeData &mimeData)
{
    qDebug() << "In " << QString(__FUNCTION__);
    if (mimeData.hasImage())
    {
        return "image";
    } else if (mimeData.hasUrls())
    {
        if ((mimeData.text().startsWith("http://maps.google") || mimeData.text().startsWith("http://goo.gl/maps")))
        {
            return "address";
        }
        else if (mimeData.urls()[0].isLocalFile())
        {
            return "file";
        }else if (mimeData.text().startsWith("http") || mimeData.text().startsWith("ftp") || mimeData.text().startsWith("email"))
        {
            return "link";
        }else
            return "note";
    } else if (mimeData.hasHtml() || mimeData.hasText())
    {
        if (mimeData.text().startsWith("mailto"))
            return "e-mail";

        return "note";
    }
    else{
        return "";
    }
}

void Settings::renameClipboardMenu()
{
    qDebug() << "In " << QString(__FUNCTION__);

    if (settings->value("apiKey","").toString() == "" || clipboardMenu->isEmpty())
    {
        showResult = false;
        this->loadConfig();
        //tray->contextMenu()->
        return;
    }


    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();


    clipboardMenu->setEnabled(false);
    QStringList formats = mimeData->formats();

    for (int i = 0; i < formats.count(); ++i)
    {
        qDebug() << formats[i];
    }

    QString content(detectClipboardContents(*mimeData));

    if (!content.isEmpty()){
        clipboardMenu->setTitle("Clipboard "+content+" to");

        //if(content != "file")
            clipboardMenu->setEnabled(true);
    }
}

void Settings::processDevices(const QJsonValue &response)
{
    qDebug() << "In " << QString(__FUNCTION__);

    clipboardMenu->setEnabled(false);
    noteMenu->setEnabled(false);
    addressMenu->setEnabled(false);
    listMenu->setEnabled(false);
    fileMenu->setEnabled(false);
    linkMenu->setEnabled(false);

    if (response.isArray() == false|| response.toArray().size() == 0)
        return;

    QJsonArray devices(response.toArray());

    DELETE_IF_NOT_NULL(foo);
    foo = new QObject(this);

    clipboardMenu->clear();
    noteMenu->clear();
    addressMenu->clear();
    listMenu->clear();
    fileMenu->clear();
    linkMenu->clear();
    clipboardMenu->setEnabled(true);
    noteMenu->setEnabled(true);
    addressMenu->setEnabled(true);
    listMenu->setEnabled(true);
    fileMenu->setEnabled(true);
    linkMenu->setEnabled(true);

    ui->tblDevicesList->clear();

    ui->tblDevicesList->setRowCount(devices.count());
    for (int i = 0; i < devices.count(); ++i)
    {
        QJsonObject device(devices[i].toObject());
        qDebug() << device["id"].toString() << ":"<<device["id"].toInt();
        ui->tblDevicesList->setItem(i,0,new QTableWidgetItem(device["id"].toString()));
        ui->tblDevicesList->setItem(i,1,new QTableWidgetItem("Yours"));
        ui->tblDevicesList->setItem(i,2,new QTableWidgetItem(device["extras"].toObject()["manufacturer"].toString()));
        ui->tblDevicesList->setItem(i,3,new QTableWidgetItem(device["extras"].toObject()["model"].toString()));
        ui->tblDevicesList->setItem(i,4,new QTableWidgetItem(device["extras"].toObject()["android_version"].toString()));

        QString deviceDescription("Your "+device["extras"].toObject()["model"].toString()+" ("+device["id"].toString()+")");
        this->devices.insert(device["id"].toString(),deviceDescription);

        Bullet *bullet = new Bullet(deviceDescription,device["id"].toString(),foo);
        connect(bullet,SIGNAL(sendAddress(QString,const QString & )),this,SLOT(sendAddress(QString,const QString &)));
        connect(bullet,SIGNAL(sendNote(QString,const QString & )),this,SLOT(sendNote(QString,const QString & )));
        connect(bullet,SIGNAL(sendList(QString,const QString & )),this,SLOT(sendList(QString,const QString & )));
        connect(bullet,SIGNAL(sendLink(QString,const QString & )),this,SLOT(sendLink(QString,const QString & )));
        connect(bullet,SIGNAL(sendFile(QString,const QString & )),this,SLOT(sendFile(QString,const QString & )));
        connect(bullet,SIGNAL(sendClipboard(QString,const QString & )),this,SLOT(sendClipboard(QString,const QString & )));

        noteMenu->addAction(deviceDescription,bullet,SLOT(sendNote()));
        addressMenu->addAction(deviceDescription,bullet,SLOT(sendAddress()));
        listMenu->addAction(deviceDescription,bullet,SLOT(sendList()));
        fileMenu->addAction(deviceDescription,bullet,SLOT(sendFile()));
        linkMenu->addAction(deviceDescription,bullet,SLOT(sendLink()));
        clipboardMenu->addAction(deviceDescription,bullet,SLOT(sendClipboard()));
    }
}

void Settings::processSharedDevices(const QJsonValue &response)
{
    qDebug() << "In " << QString(__FUNCTION__);
    if (response.isArray() == false|| response.toArray().size() == 0)
        return;

    QJsonArray devices(response.toArray());

    int starting_row = ui->tblDevicesList->rowCount();

    if (starting_row > 0)
    {
        clipboardMenu->addSeparator();
        noteMenu->addSeparator();
        addressMenu->addSeparator();
        listMenu->addSeparator();
        fileMenu->addSeparator();
        linkMenu->addSeparator();
    }

    clipboardMenu->setEnabled(true);
    noteMenu->setEnabled(true);
    addressMenu->setEnabled(true);
    listMenu->setEnabled(true);
    fileMenu->setEnabled(true);
    linkMenu->setEnabled(true);

    ui->tblDevicesList->setRowCount(starting_row+devices.count());
    for (int i = 0; i < devices.count(); ++i)
    {
        QJsonObject device(devices[i].toObject());

        ui->tblDevicesList->setItem(starting_row+i,0,new QTableWidgetItem(device["id"].toString()));
        ui->tblDevicesList->setItem(starting_row+i,1,new QTableWidgetItem(device["owner_name"].toString()));
        ui->tblDevicesList->setItem(starting_row+i,2,new QTableWidgetItem(device["extras"].toObject()["manufacturer"].toString()));
        ui->tblDevicesList->setItem(starting_row+i,3,new QTableWidgetItem(device["extras"].toObject()["model"].toString()));
        ui->tblDevicesList->setItem(starting_row+i,4,new QTableWidgetItem(device["extras"].toObject()["android_version"].toString()));

        QString deviceDescription(device["owner_name"].toString()+"'s "+device["extras"].toObject()["model"].toString()+" ("+device["id"].toString()+")");
        this->devices.insert(device["id"].toString(),deviceDescription);

        Bullet *bullet = new Bullet(deviceDescription,device["id"].toString(),foo);
        connect(bullet,SIGNAL(sendAddress(QString,int)),this,SLOT(sendAddress(QString,const QString & )));
        connect(bullet,SIGNAL(sendNote(QString,const QString & )),this,SLOT(sendNote(QString,const QString & )));
        connect(bullet,SIGNAL(sendList(QString,const QString & )),this,SLOT(sendList(QString,const QString & )));
        connect(bullet,SIGNAL(sendLink(QString,const QString & )),this,SLOT(sendLink(QString,const QString & )));
        connect(bullet,SIGNAL(sendFile(QString,const QString & )),this,SLOT(sendFile(QString,const QString & )));
        connect(bullet,SIGNAL(sendClipboard(QString,int)),this,SLOT(sendClipboard(QString,const QString & )));

        noteMenu->addAction(deviceDescription,bullet,SLOT(sendNote()));
        addressMenu->addAction(deviceDescription,bullet,SLOT(sendAddress()));
        listMenu->addAction(deviceDescription,bullet,SLOT(sendList()));
        fileMenu->addAction(deviceDescription,bullet,SLOT(sendFile()));
        linkMenu->addAction(deviceDescription,bullet,SLOT(sendLink()));
        clipboardMenu->addAction(deviceDescription,bullet,SLOT(sendClipboard()));
    }
}

void Settings::processResponse(const QJsonObject &response)
{
    qDebug() << "In " << QString(__FUNCTION__);
    if (response["created"].isNull())
        return;

    QString description;
    if (devices.contains(response["device_id"].toString()))
    {
        description = devices[response["device_id"].toString()];
    }
    else
    {
        description = "Unknown device "+response["device_id"].toString();
    }

    tray->showMessage("Sending successfull","Your "+response["data"].toObject()["type"].toString()+ " has been successfully sent to "+description);
}
void Settings::sendNote(QString deviceDescription, const QString &id)
{
    qDebug() << "In " << QString(__FUNCTION__);
    if (prompt->showPrompt("Send note to "+deviceDescription,"Title") != QDialog::Accepted)
    {
        return;
    }

    sendText(id,"note",prompt->getTitle(),"body",prompt->getText());
}

void Settings::sendAddress(QString deviceDescription, const QString &id)
{
qDebug() << "In " << QString(__FUNCTION__);
    if (prompt->showPrompt("Send address to "+deviceDescription,"Name") != QDialog::Accepted)
    {
        return;
    }

    sendText(id,"address",prompt->getTitle(),"address",prompt->getText());
}

void Settings::sendList(QString deviceDescription, const QString &id)
{
    qDebug() << "In " << QString(__FUNCTION__);

    if (prompt->showPrompt("Send list to "+deviceDescription,"Title") != QDialog::Accepted)
    {
        return;
    }

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart devicePart;
    devicePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"device_id\""));
    devicePart.setBody(id.toLatin1());
    multiPart->append(devicePart);

    QHttpPart typePart;
    typePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"type\""));
    typePart.setBody("list");
    multiPart->append(typePart);

    QHttpPart titlePart;
    titlePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"title\""));

    titlePart.setBody(prompt->getTitle().toUtf8());
    multiPart->append(titlePart);

    QStringList list(prompt->getText().split(QRegExp("[\r\n]"),QString::SkipEmptyParts));

    for (QStringList::const_iterator i = list.begin(); i != list.end(); ++i)
    {
        QHttpPart contentPart;
        contentPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"items\""));
        QString item(*i);
        contentPart.setBody(item.toUtf8());
        multiPart->append(contentPart);
    }

    QNetworkRequest request(QUrl("https://api.pushbullet.com/api/pushes"));
    addAuthentication(request);

    QNetworkReply *reply = networkaccess->post(request, multiPart);
    multiPart->setParent(reply); // delete the multiPart with the reply

    qDebug() << "Out " << QString(__FUNCTION__);
}

void Settings::sendLink(QString deviceDescription, const QString &id)
{
    if (prompt->showPrompt("Send link to "+deviceDescription,"Title") != QDialog::Accepted)
    {
        return;
    }

    sendText(id,"link",prompt->getTitle(),"url",prompt->getText());
}

void Settings::sendText(const QString &id, QString type, const QString title, QString contentType, const QString content)
{
    qDebug() << "In " << QString(__FUNCTION__);
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart devicePart;
    devicePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"device_id\""));
    devicePart.setBody(id.toLatin1());
    multiPart->append(devicePart);

    QHttpPart typePart;
    typePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"type\""));
    typePart.setBody(type.toLatin1());
    multiPart->append(typePart);

    QHttpPart titlePart;
    titlePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\""+QString(contentType=="address"?"name":"title")+"\""));

    titlePart.setBody(title.toUtf8());
    multiPart->append(titlePart);

    QHttpPart contentPart;
    contentPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\""+contentType.toUtf8()+"\""));
    contentPart.setBody(content.toUtf8());
    multiPart->append(contentPart);

    QNetworkRequest request(QUrl("https://api.pushbullet.com/api/pushes"));
    addAuthentication(request);

    QNetworkReply *reply = networkaccess->post(request, multiPart);
    multiPart->setParent(reply); // delete the multiPart with the reply
}

void Settings::sendFile(QString deviceDescription, const QString &id)
{
    qDebug() << "In " << QString(__FUNCTION__);
    QString fileName(QFileDialog::getOpenFileName(0,"Select file to be sent to: "+deviceDescription));

    if (fileName.isEmpty())
    {
        tray->showMessage("No File selected","You did not select a file.  Please try again.",QSystemTrayIcon::Warning);
        return;
    }

    sendFilePrivate(id, fileName);
}

void Settings::sendFilePrivate(const QString &id, const QString fileName)
{

    QFile *file = new QFile(fileName);
    if (file->size() > 10*1024*1024)
    {
        tray->showMessage("File to big",fileName+" is to big (max 10MB)",QSystemTrayIcon::Critical);
        delete file;
        return;
    }

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart devicePart;
    devicePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"device_id\""));
    devicePart.setBody(id.toLatin1());

    QHttpPart typePart;
    typePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"type\""));
    typePart.setBody("file");

    QHttpPart filePart;
    QMimeDatabase mdb;
    QString fileNamePart(QFileInfo(fileName).fileName());
    qDebug() << "Mime type: "<< mdb.mimeTypeForFile(fileName).name();
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(QString("form-data; name=\"file\"; filename=\"")+fileNamePart+"\""));
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(mdb.mimeTypeForFile(fileName).name()));

    file->open(QIODevice::ReadOnly);
    filePart.setBodyDevice(file);
    file->setParent(multiPart); // we cannot delete the file now, so delete it with the multiPart

    multiPart->append(devicePart);
    multiPart->append(typePart);
    multiPart->append(filePart);

    QNetworkRequest request(QUrl("https://api.pushbullet.com/api/pushes"));
    addAuthentication(request);

    QNetworkReply *post = networkaccess->post(request, multiPart);
    connect(post,SIGNAL(uploadProgress(qint64,qint64)),this,SLOT(uploadProgress(int, int)));
    multiPart->setParent(post); // delete the multiPart with the reply
}

void Settings::sendClipboard(QString , const QString & id)
{
    qDebug() << "In " << QString(__FUNCTION__);
    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();

    QString content(detectClipboardContents(*mimeData));
    qDebug() << "It's a "<< content;


    if (content == "file" )
    {
        //File:

        sendFilePrivate(id, mimeData->urls()[0].toLocalFile());

    }else if (content == "image" )
    {
        QImage image(qvariant_cast<QImage>(mimeData->imageData()));

        QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

        QHttpPart devicePart;
        devicePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"device_id\""));
        devicePart.setBody(id.toLatin1());

        QHttpPart typePart;
        typePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"type\""));
        typePart.setBody("file");

        QHttpPart filePart;
        QMimeDatabase mdb;
        QString fileNamePart("Clipboard Image.png");

        filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(mdb.mimeTypeForFile(fileNamePart).name()));
        filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(QString("form-data; name=\"file\"; filename=\"")+fileNamePart+"\""));

        QByteArray *ba = new QByteArray();
        QBuffer *buffer = new QBuffer(ba);
        buffer->open(QIODevice::ReadWrite);
        image.save(buffer, "PNG");
        buffer->reset();

        filePart.setBodyDevice(buffer);
        buffer->setParent(multiPart); // we cannot delete the file now, so delete it with the multiPart

        multiPart->append(devicePart);
        multiPart->append(typePart);
        multiPart->append(filePart);

        QNetworkRequest request(QUrl("https://api.pushbullet.com/api/pushes"));
        addAuthentication(request);

        QNetworkReply *reply = networkaccess->post(request, multiPart);
        multiPart->setParent(reply); // delete the multiPart with the reply

        //QVariant image
    } else if (content == "address")
    {
        sendText(id,"address","Address from Clipboard","address",mimeData->text());
    }else if (content == "link")
    {
        sendText(id,"link","Link from clipboard","url",mimeData->text());
    }else if (content == "note")
    {
        sendText(id,"note","Text from clipboard","body",mimeData->text());
    }else if (content == "e-mail")
    {
        sendText(id,"link","Email address from clipboard","url",mimeData->text().replace("mailto:","email://"));
    }
}

void Settings::handleError(int errorCode, QString serverMessage)
{
qDebug() << "In " << QString(__FUNCTION__);
    QString error;
    switch (errorCode)
    {
    case 400:
        error = "Missing Parameter.  Please check for an updated version of this software.";
        break;
    case 401:
        show();
        ui->txtAPIKey->selectAll();
        ui->txtAPIKey->setFocus();
        error = "No Valid API key provided";
        break;
    case 402:
        error = "Request failed.  Please check your device and try again.";
        break;
    case 403:
        show();
        ui->txtAPIKey->selectAll();
        ui->txtAPIKey->setFocus();
        error = "Your API key is not valid for this request or to this device.";
        break;
    case 404:
        error = "The requested item doesn't exist.  Please check for an updated version of this software.";
        break;
    default:
        if (errorCode >= 500)
            error = "Something went wrong on PushBullet's side.\nHTTP error "+QString::number(errorCode)+": "+serverMessage;
        else
            error = "Something went wrong on with QBullet.\nHTTP error "+QString::number(errorCode)+": "+serverMessage;
    }

    if (isVisible() || ! tray->supportsMessages())
    {
        setFocus();
        QMessageBox::critical(this,"An error occurred",error);
    }else
    {
        tray->showMessage("An error occurred",error,QSystemTrayIcon::Critical,0);
    }


}

void Settings::about()
{
    qDebug() << "In " << QString(__FUNCTION__);
    QMessageBox::about(this,"About "+qApp->applicationName(),qApp->applicationDisplayName()+". Developed by "+qApp->organizationName()+".\r\nVersion: "+qApp->applicationVersion()+"\r\nIcon from https://inkedicon.deviantart.com/art/Bullet-Game-Icon-322005720");
}
