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

#include <bullet.h>
#include <prompt.h>

#include <logindialog.h>

Settings::Settings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Settings),
    settings(new QSettings(__FILE__,"QBullet",this)),
    tray(new QSystemTrayIcon()),
    menu(new QMenu(this)),
    networkaccess(new QNetworkAccessManager(this)),
    foo(NULL),
    prompt(new Prompt(this)),
    showResult(false),
    exitClicked(false),
    devices(QMap<int,QString>()),
    proxyAuthenticationSupplied(false)
{
    ui->setupUi(this);

    connect(networkaccess, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyReceived(QNetworkReply*)));
    connect(networkaccess, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)), this,SLOT(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)));
    connect(ui->cmbProxyType,SIGNAL(currentIndexChanged(QString)),this,SLOT(proxyTypeChanged(QString)));
    connect(ui->btnTest,SIGNAL(clicked()),this,SLOT(executeTest()));

    tray->setIcon(QIcon(":icons/QBullet.png"));
    tray->show();
    setWindowIcon(QIcon(":icons/QBullet.png"));
    QApplication::setWindowIcon(QIcon(":icons/QBullet.png"));

    clipboardMenu = menu->addMenu("Clipboard to");
    connect(menu,SIGNAL(aboutToShow()),this,SLOT(renameClipboardMenu()));
    noteMenu = menu->addMenu("Note to");
    addressMenu = menu->addMenu("Address to");
    listMenu = menu->addMenu("List to");
    fileMenu = menu->addMenu("File to");
    linkMenu = menu->addMenu("Link to");

    menu->addSeparator();
    menu->addAction("&Settings",this,SLOT(show()));
    menu->addAction("&Update Devices",this,SLOT(getDevices()));
    menu->addSeparator();
    menu->addAction("&Exit",this,SLOT(exit()));

    QAction *tmpAction;
    ui->toolButton->addAction(tmpAction = new QAction("About QBullet",ui->toolButton));
    connect(tmpAction,SIGNAL(triggered()),this,SLOT(about()));
    ui->toolButton->addAction(tmpAction = new QAction("About QT",ui->toolButton));
    connect(tmpAction,SIGNAL(triggered()),qApp,SLOT(aboutQt()));
    tray->setContextMenu(menu);

    ui->cmbProxyType->addItem("System Proxy",-1);
    ui->cmbProxyType->addItem("Default",QNetworkProxy::DefaultProxy);
    //ui->cmbProxyType->addItem("FTP Caching",QNetworkProxy::FtpCachingProxy);
    ui->cmbProxyType->addItem("HTTP",QNetworkProxy::HttpProxy);
    ui->cmbProxyType->addItem("None",QNetworkProxy::NoProxy);
    ui->cmbProxyType->addItem("HTTP Caching",QNetworkProxy::HttpCachingProxy);
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
        handleError(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(),reply->errorString());
    }else
    {
        handleResponse(data);
    }

    showResult = false;
    qDebug() << "Out " << QString(__FUNCTION__);
}

void Settings::proxyAuthenticationRequired ( const QNetworkProxy & proxy, QAuthenticator * authenticator )
{
    if (proxyAuthenticationSupplied == false &&
            settings->value("proxyRememberPassword",false).toBool() &&
            settings->value("proxyUser","").toString() != "")
    {
        proxyAuthenticationSupplied = true;

        authenticator->setUser(settings->value("proxyUser","").toString());
        authenticator->setPassword(settings->value("proxyPassword","").toString());
        return;
    }

#ifdef WIN32
    show();
#endif

    LoginDialog * login = new LoginDialog(this);

    QString proxyType;
    switch (proxy.type())
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
        proxyType = "Unknown";
    }

    login->setMessage(proxy.hostName()+":"+QString::number(proxy.port()),proxyType);
    login->setUser(proxy.user());
    login->setPassword(proxy.password());
    login->setRemember(settings->value("proxyRememberPassword",false).toBool());

    if (login->exec() == LoginDialog::Accepted)
    {
        authenticator->setPassword(login->password());
        authenticator->setUser(login->user());
        settings->setValue("proxyRememberPassword",login->remember());
        settings->setValue("proxyUser",login->user());

        if (login->remember())
        {
            settings->setValue("proxyPassword",login->password());
        }
        else
        {
            settings->setValue("proxyPassword","");
        }
    }

    /**
     * TODO Define this
     */
    DELETE_IF_NOT_NULL(login);
}

void Settings::accept()
{
    if (ui->cmbProxyType->itemData(ui->cmbProxyType->currentIndex()).toInt() != -1 &&
            ui->cmbProxyType->itemData(ui->cmbProxyType->currentIndex()).toInt() != QNetworkProxy::NoProxy)
    {
        if (ui->txtProxyServer->text() == "" || ui->sbPort->value() <= 0)
        {
#ifdef WIN32
            show();
#endif
            QMessageBox::critical(this,"Invalid Proxy URL supplied!","Please supply a host name and port.\nThe default is 8080 or 3128.",QMessageBox::Ok);

        }
    }


    settings->setValue("firststart",false);
    settings->setValue("apiKey",ui->txtAPIKey->text());
    settings->setValue("proxyUser",ui->txtProxyUsername->text());
    settings->setValue("proxyPassword",ui->txtProxyPassword->text());
    settings->setValue("proxyServer",ui->txtProxyServer->text());
    settings->setValue("proxyServerPort",ui->sbPort->value());
    settings->setValue("proxyServerType",ui->cmbProxyType->itemData(ui->cmbProxyType->currentIndex()).toInt());

    /**
     * TODO: Save Screen values.
     * Set Proxy
     * Save Devices
     */

    hide();
    loadConfig();
    getDevices();
}

void Settings::loadConfig()
{
    proxyAuthenticationSupplied = false;

    if (settings->value("proxyServerType",(int)QNetworkProxy::NoProxy).toInt() == -1)
    {
        QNetworkProxyFactory::setUseSystemConfiguration(true);
        //QNetworkProxyQuery npq(QUrl("https://www.pushbullet.com/api/devices"));
        QNetworkProxyQuery npq("www.pushbullet.com",443,"https",QNetworkProxyQuery::UrlRequest);
        QList<QNetworkProxy> proxies = QNetworkProxyFactory::systemProxyForQuery(npq);
        QNetworkProxy newproxy(proxies[0]);

        if (proxies.count() > 0)
        {
            networkaccess->setProxy(newproxy);
        }
    }else
    {
        QNetworkProxyFactory::setUseSystemConfiguration(false);
        //QNetworkProxy *proxy = new QNetworkProxy();
        //networkaccess->setProxy(proxy);

        QNetworkProxy newproxy;


        QNetworkProxy::ProxyType proxyType = QNetworkProxy::NoProxy;

        switch (settings->value("proxyServerType",(int)QNetworkProxy::NoProxy).toInt())
        {
        case QNetworkProxy::NoProxy:
        case QNetworkProxy::DefaultProxy:
        case QNetworkProxy::HttpProxy:
        case QNetworkProxy::Socks5Proxy:
        case QNetworkProxy::HttpCachingProxy:
            proxyType = (QNetworkProxy::ProxyType)settings->value("proxyServerType",(int)QNetworkProxy::NoProxy).toInt();
            break;
        default:
            show();
            QMessageBox::warning(this,"Error","Invalid proxy type selected: "+settings->value("proxyServerType",(int)QNetworkProxy::NoProxy).toInt());
            break;

        }

        if (settings->value("proxyServer","").toString() == "")
        {
            proxyType = QNetworkProxy::NoProxy;
            settings->setValue("proxyServerType",proxyType);
        }

        newproxy.setType(proxyType);

        if (proxyType != QNetworkProxy::NoProxy)
        {
            newproxy.setHostName(settings->value("proxyServer","").toString());
            newproxy.setPort(settings->value("proxyServerPort",8080).toInt());
            qDebug() << "proxyServer: "<<settings->value("proxyServer","").toString()<<endl;
            qDebug() << "proxyServerPort: "<<settings->value("proxyServerPort",8080).toString()<<endl;
        }

        networkaccess->setProxy(newproxy);
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

    //ui->cbSystemProxy->setChecked(settings->value("proxyServerType",-1).toInt() == -1);
    ui->txtAPIKey->setText(settings->value("apiKey","").toString());
    ui->txtProxyServer->setEnabled(settings->value("proxyServerType",-1).toInt() != -1);
    ui->txtProxyServer->setText(settings->value("proxyServer","").toString());
    ui->sbPort->setValue(settings->value("proxyServerPort",8080).toInt());
    ui->sbPort->setEnabled(settings->value("proxyServerType",-1).toInt() != -1);

    ui->txtProxyUsername->setText(settings->value("proxyUser","").toString());
    ui->txtProxyPassword->setText(settings->value("proxyPassword","").toString());


    QString proxyType;
    switch(settings->value("proxyServerType",-1).toInt())
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
    case QNetworkProxy::NoProxy:
        proxyType = "None";
        break;
    case -1:
    default:
        proxyType = "System Proxy";
        break;
    }

    ui->cmbProxyType->setCurrentText(proxyType);

    QDialog::show();
}

void Settings::proxyTypeChanged(const QString &type)
{
    ui->txtProxyServer->setEnabled(type != "System Proxy" && type != "None");
    ui->sbPort->setEnabled(type != "System Proxy" && type != "None");
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

void Settings::renameClipboardMenu()
{
    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();


    clipboardMenu->setEnabled(true);
    QStringList formats = mimeData->formats();

    for (int i = 0; i < formats.count(); ++i)
    {
        qDebug() << formats[i];
    }
    if (mimeData->hasImage())
    {
        clipboardMenu->setTitle("Clipboard image to");
    } else if (mimeData->hasHtml())
    {
        clipboardMenu->setTitle("Clipboard note to");
    } else if (mimeData->hasUrls())
    {
        if ((mimeData->text().startsWith("https://maps.google") || mimeData->text().startsWith("http://goo.gl/maps")))
        {
            clipboardMenu->setTitle("Clipboard address to");
        }
        else if (mimeData->text().startsWith("file://"))
        {
            clipboardMenu->setEnabled(false);
            clipboardMenu->setTitle("Clipboard file to");
        }
    }else if (mimeData->hasText())
    {
        clipboardMenu->setTitle("Clipboard note to");
    }
    else{


        clipboardMenu->setEnabled(false);
    }
}

void Settings::processDevices(const QJsonValue &response)
{
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
    ui->tblDevicesList->clear();

    ui->tblDevicesList->setRowCount(devices.count());
    for (int i = 0; i < devices.count(); ++i)
    {
        QJsonObject device(devices[i].toObject());
        ui->tblDevicesList->setItem(i,0,new QTableWidgetItem(QString::number((int)device["id"].toDouble())));
        ui->tblDevicesList->setItem(i,1,new QTableWidgetItem("Yours"));
        ui->tblDevicesList->setItem(i,2,new QTableWidgetItem(device["extras"].toObject()["manufacturer"].toString()));
        ui->tblDevicesList->setItem(i,3,new QTableWidgetItem(device["extras"].toObject()["model"].toString()));
        ui->tblDevicesList->setItem(i,4,new QTableWidgetItem(device["extras"].toObject()["android_version"].toString()));

        QString deviceDescription("Your "+device["extras"].toObject()["model"].toString()+" ("+QString::number((int)device["id"].toDouble())+")");
        this->devices.insert((int)device["id"].toDouble(),deviceDescription);

        Bullet *bullet = new Bullet(deviceDescription,(int)device["id"].toDouble(),foo);
        connect(bullet,SIGNAL(sendAddress(QString,int)),this,SLOT(sendAddress(QString,int)));
        connect(bullet,SIGNAL(sendNote(QString,int)),this,SLOT(sendNote(QString,int)));
        connect(bullet,SIGNAL(sendList(QString,int)),this,SLOT(sendList(QString,int)));
        connect(bullet,SIGNAL(sendLink(QString,int)),this,SLOT(sendLink(QString,int)));
        connect(bullet,SIGNAL(sendFile(QString,int)),this,SLOT(sendFile(QString,int)));
        connect(bullet,SIGNAL(sendClipboard(QString,int)),this,SLOT(sendClipboard(QString,int)));

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
    if (response.isArray() == false|| response.toArray().size() == 0)
        return;

    QJsonArray devices(response.toArray());

    clipboardMenu->addSeparator();
    noteMenu->addSeparator();
    addressMenu->addSeparator();
    listMenu->addSeparator();
    fileMenu->addSeparator();
    linkMenu->addSeparator();

    int starting_row = ui->tblDevicesList->rowCount();
    ui->tblDevicesList->setRowCount(starting_row+devices.count());
    for (int i = 0; i < devices.count(); ++i)
    {
        QJsonObject device(devices[i].toObject());

        ui->tblDevicesList->setItem(starting_row+i,0,new QTableWidgetItem(QString::number((int)device["id"].toDouble())));
        ui->tblDevicesList->setItem(starting_row+i,1,new QTableWidgetItem(device["owner_name"].toString()));
        ui->tblDevicesList->setItem(starting_row+i,2,new QTableWidgetItem(device["extras"].toObject()["manufacturer"].toString()));
        ui->tblDevicesList->setItem(starting_row+i,3,new QTableWidgetItem(device["extras"].toObject()["model"].toString()));
        ui->tblDevicesList->setItem(starting_row+i,4,new QTableWidgetItem(device["extras"].toObject()["android_version"].toString()));

        QString deviceDescription(device["owner_name"].toString()+"'s "+device["extras"].toObject()["model"].toString()+" ("+QString::number((int)device["id"].toDouble())+")");
        this->devices.insert((int)device["id"].toDouble(),deviceDescription);

        Bullet *bullet = new Bullet(deviceDescription,(int)device["id"].toDouble(),foo);
        connect(bullet,SIGNAL(sendAddress(QString,int)),this,SLOT(sendAddress(QString,int)));
        connect(bullet,SIGNAL(sendNote(QString,int)),this,SLOT(sendNote(QString,int)));
        connect(bullet,SIGNAL(sendList(QString,int)),this,SLOT(sendList(QString,int)));
        connect(bullet,SIGNAL(sendLink(QString,int)),this,SLOT(sendLink(QString,int)));
        connect(bullet,SIGNAL(sendFile(QString,int)),this,SLOT(sendFile(QString,int)));
        connect(bullet,SIGNAL(sendClipboard(QString,int)),this,SLOT(sendClipboard(QString,int)));

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
    if (response["created"].isNull())
        return;

    QString description;
    if (devices.contains((int)response["device_id"].toDouble()))
    {
        description = devices[(int)response["device_id"].toDouble()];
    }
    else
    {
        description = "Unknown device "+QString::number(response["device_id"].toDouble());
    }

    tray->showMessage("Sending successfull","Your "+response["data"].toObject()["type"].toString()+ " has been successfully sent to "+description);
}
void Settings::sendNote(QString deviceDescription, int id)
{
#ifdef WIN32
    show();
#endif

    if (prompt->showPrompt("Send note to "+deviceDescription,"Title") != QDialog::Accepted)
    {
        hide();

        return;
    }

    hide();
    sendText(id,"note",prompt->getTitle(),"body",prompt->getText());
}

void Settings::sendAddress(QString deviceDescription, int id)
{
#ifdef WIN32
    show();
#endif

    if (prompt->showPrompt("Send address to "+deviceDescription,"Name") != QDialog::Accepted)
    {
        hide();

        return;
    }

    hide();
    sendText(id,"address",prompt->getTitle(),"address",prompt->getText());
}

void Settings::sendList(QString deviceDescription, int id)
{
    qDebug() << "In " << QString(__FUNCTION__);

#ifdef WIN32
    show();
#endif

    if (prompt->showPrompt("Send list to "+deviceDescription,"Title") != QDialog::Accepted)
    {
        hide();

        return;
    }

    hide();
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart devicePart;
    devicePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"device_id\""));
    devicePart.setBody(QString::number(id).toLatin1());
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
        contentPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"items["+QString::number(i-list.begin())+"]\""));
        QString item(*i);
        contentPart.setBody(item.toUtf8());
        multiPart->append(contentPart);
    }

    QNetworkRequest request(QUrl("https://www.pushbullet.com/api/pushes"));
    addAuthentication(request);

    QNetworkReply *reply = networkaccess->post(request, multiPart);
    multiPart->setParent(reply); // delete the multiPart with the reply

    qDebug() << "Out " << QString(__FUNCTION__);
}

void Settings::sendLink(QString deviceDescription, int id)
{
#ifdef WIN32
    show();
#endif

    if (prompt->showPrompt("Send link to "+deviceDescription,"Title") != QDialog::Accepted)
    {
        hide();

        return;
    }

    hide();
    sendText(id,"link",prompt->getTitle(),"url",prompt->getText());
}

void Settings::sendText(int id, QString type, const QString title, QString contentType, const QString content)
{
    qDebug() << "In " << QString(__FUNCTION__);
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart devicePart;
    devicePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"device_id\""));
    devicePart.setBody(QString::number(id).toLatin1());
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

    QNetworkRequest request(QUrl("https://www.pushbullet.com/api/pushes"));
    addAuthentication(request);

    QNetworkReply *reply = networkaccess->post(request, multiPart);
    multiPart->setParent(reply); // delete the multiPart with the reply
}

void Settings::sendFile(QString deviceDescription, int id)
{
#ifdef WIN32
    show();
#endif

    QString fileName(QFileDialog::getOpenFileName(0,"Select file to be sent to: "+deviceDescription));

    if (fileName.isEmpty())
    {
        tray->showMessage("No File selected","You did not select a file.  Please try again.",QSystemTrayIcon::Warning);
        hide();
        return;
    }

    QFile *file = new QFile(fileName);
    if (file->size() > 10*1024*1024)
    {
        tray->showMessage("File to big",fileName+" is to big (max 10MB)",QSystemTrayIcon::Critical);
        delete file;
        hide();
        return;
    }

    hide();

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart devicePart;
    devicePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"device_id\""));
    devicePart.setBody(QString::number(id).toLatin1());

    QHttpPart typePart;
    typePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"type\""));
    typePart.setBody("file");

    QHttpPart filePart;
    QMimeDatabase mdb;
    QString fileNamePart(QFileInfo(fileName).fileName());
    qDebug() << "Mime type: "<< mdb.mimeTypeForFile(fileName).name();
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(mdb.mimeTypeForFile(fileName).name()));
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(QString("form-data; name=\"file\" filename=\"")+fileNamePart+"\""));

    file->open(QIODevice::ReadOnly);
    filePart.setBodyDevice(file);
    file->setParent(multiPart); // we cannot delete the file now, so delete it with the multiPart

    multiPart->append(devicePart);
    multiPart->append(typePart);
    multiPart->append(filePart);

    QNetworkRequest request(QUrl("https://www.pushbullet.com/api/pushes"));
    addAuthentication(request);

    QNetworkReply *reply = networkaccess->post(request, multiPart);
    multiPart->setParent(reply); // delete the multiPart with the reply
}

void Settings::sendClipboard(QString , int id)
{
    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();

    if (mimeData->hasImage())
    {
        QImage image = qvariant_cast<QImage>(mimeData->imageData());

        QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

        QHttpPart devicePart;
        devicePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"device_id\""));
        devicePart.setBody(QString::number(id).toLatin1());

        QHttpPart typePart;
        typePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"type\""));
        typePart.setBody("file");

        QHttpPart filePart;
        QMimeDatabase mdb;
        QString fileNamePart("Clipboard Image.png");

        filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(mdb.mimeTypeForFile(fileNamePart).name()));
        filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(QString("form-data; name=\"file\" filename=\"")+fileNamePart+"\""));

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

        QNetworkRequest request(QUrl("https://www.pushbullet.com/api/pushes"));
        addAuthentication(request);

        QNetworkReply *reply = networkaccess->post(request, multiPart);
        multiPart->setParent(reply); // delete the multiPart with the reply

        //QVariant image
    } else if (mimeData->hasUrls() && (mimeData->text().startsWith("https://maps.google") || mimeData->text().startsWith("http://goo.gl/maps")))
    {
        sendText(id,"address","Address from Clipboard","address",mimeData->text());
    }else if (mimeData->hasUrls())
    {
        sendText(id,"link","Link from clipboard","url",mimeData->text());
    }else if (mimeData->hasText())
    {
        sendText(id,"note","Text from clipboard","body",mimeData->text());
    }
}

void Settings::handleError(int errorCode, QString serverMessage)
{

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
        error = "Something went wrong on PushBullet's side.  Error "+QString::number(errorCode)+": \n"+serverMessage;
    }
#ifdef WIN32
    show();
#endif

    QMessageBox::critical(this,"An error occurred",error);

}

void Settings::about()
{
    QMessageBox::about(this,"About "+qApp->applicationName(),qApp->applicationDisplayName()+". Developed by "+qApp->organizationName()+".\r\nVersion: "+qApp->applicationVersion());
}
