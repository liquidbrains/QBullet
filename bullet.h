#ifndef BULLET_H
#define BULLET_H

#include <QObject>


class Bullet : public QObject
{
    Q_OBJECT

    QString deviceDescription;
    QString id;
public:
    explicit Bullet(QString deviceDescription, QString id,QObject *parent = 0);
    virtual ~Bullet();
signals:
    void sendClipboard(QString deviceDescription, const QString &);
    void sendNote(QString deviceDescription, const QString &);
    void sendAddress(QString deviceDescription,const QString &);
    void sendList(QString deviceDescription, const QString &);
    void sendLink(QString deviceDescription, const QString &);
    void sendFile(QString deviceDescription, const QString &);
public slots:
    void sendClipboard();
    void sendNote();
    void sendAddress();
    void sendList();
    void sendLink();
    void sendFile();
};

#endif // BULLET_H
