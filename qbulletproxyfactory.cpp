#include "qbulletproxyfactory.h"


#include <QSettings>

QBulletProxyFactory::QBulletProxyFactory(const QSettings &_settings):
settings(_settings)
{

}

QList<QNetworkProxy> QBulletProxyFactory::queryProxy(const QNetworkProxyQuery & )
{
    QList<QNetworkProxy> ret;

    if (settings.value("proxyServerType",(int)QNetworkProxy::NoProxy).toInt() == -1)
    {
        QNetworkProxyFactory::setUseSystemConfiguration(true);
        //QNetworkProxyQuery npq(QUrl("https://www.pushbullet.com/api/devices"));
        QNetworkProxyQuery npq("www.pushbullet.com",443,"https",QNetworkProxyQuery::UrlRequest);
        QList<QNetworkProxy> proxies = QNetworkProxyFactory::systemProxyForQuery(npq);
        QNetworkProxy newproxy(proxies[0]);

        if (proxies.count() > 0)
        {
            ret.insert(0,newproxy);
        }

    }else
    {
        QNetworkProxyFactory::setUseSystemConfiguration(false);

        QNetworkProxy::ProxyType proxyType;

        switch (settings.value("proxyServerType",(int)QNetworkProxy::NoProxy).toInt())
        {
        case QNetworkProxy::NoProxy:
        case QNetworkProxy::DefaultProxy:
        case QNetworkProxy::HttpProxy:
        case QNetworkProxy::Socks5Proxy:
        case QNetworkProxy::HttpCachingProxy:
            proxyType = (QNetworkProxy::ProxyType)settings.value("proxyServerType",(int)QNetworkProxy::NoProxy).toInt();
            break;
        default:
            //show();
            //QMessageBox::warning(this,"Error","Invalid proxy type selected: "+settings.value("proxyServerType",(int)QNetworkProxy::NoProxy).toInt());
            return ret;

        }

        ret.insert(0,QNetworkProxy (proxyType,settings.value("proxyServer","").toString(),settings.value("proxyServerPort",8080).toInt()));

    }
    return ret;

}


QBulletProxyFactory::~QBulletProxyFactory()
{

}
