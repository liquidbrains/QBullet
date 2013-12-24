#ifndef BULLET_H
#define BULLET_H

#include <QObject>


class Bullet : public QObject
{
    Q_OBJECT

    QString deviceDescription;
    int id;
public:
    explicit Bullet(QString deviceDescription, int id,QObject *parent = 0);
    virtual ~Bullet();
signals:
    void sendClipboard(QString deviceDescription, int id);
    void sendNote(QString deviceDescription, int id);
    void sendAddress(QString deviceDescription, int id);
    void sendList(QString deviceDescription, int id);
    void sendLink(QString deviceDescription, int id);
    void sendFile(QString deviceDescription, int id);
public slots:
    void sendClipboard();
    void sendNote();
    void sendAddress();
    void sendList();
    void sendLink();
    void sendFile();
};

#endif // BULLET_H
