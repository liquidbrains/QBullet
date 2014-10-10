#include "filepush.h"
#include "settings.h"

FilePush::FilePush(Settings *parent, const QString &id, const QString &file, const QString &mimetype, const QByteArray &data) :
    QObject(parent)
{

}


void FilePush::fileUploadRequestDone(QNetworkReply *)
{

}

void FilePush::fileUploadDone(QNetworkReply *)
{

}
