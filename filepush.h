#ifndef FILEPUSH_H
#define FILEPUSH_H

#include <QObject>

class QNetworkReply;
class Settings;

class FilePush : public QObject
{
    Q_OBJECT

    const QString device;
    const QString file;
    const QString mime;
    const QByteArray image;
public:
    explicit FilePush(Settings *parent, const QString &id, const QString &file, const QString & mimetype, const QByteArray &data = NULL);

signals:

public slots:
    void fileUploadRequestDone(QNetworkReply *);
    void fileUploadDone(QNetworkReply *);
};

#endif // FILEPUSH_H
