#ifndef QBULLETPROXYFACTORY_H
#define QBULLETPROXYFACTORY_H

#include <QNetworkProxyFactory>
class QSettings;

class QBulletProxyFactory : public QNetworkProxyFactory
{
public:
    QBulletProxyFactory(const QSettings &);
    virtual	~QBulletProxyFactory();
    virtual QList<QNetworkProxy>	queryProxy(const QNetworkProxyQuery & query = QNetworkProxyQuery());

 private:
    const QSettings &settings;
};

#endif // QBULLETPROXYFACTORY_H
