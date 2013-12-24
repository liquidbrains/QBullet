#include "bullet.h"
#include <QJsonValue>
#include <QDebug>

Bullet::Bullet(QString _deviceDescription, int _id,QObject *parent) :
    QObject(parent),deviceDescription(_deviceDescription),id(_id)
{
}

Bullet::~Bullet()
{
}

void Bullet::sendClipboard()
{
    emit sendClipboard(deviceDescription,id);
}

void Bullet::sendNote()
{
    emit sendNote(deviceDescription,id);
}

void Bullet::sendAddress()
{
    emit sendAddress(deviceDescription,id);
}

void Bullet::sendList()
{
    emit sendList(deviceDescription,id);

}

void Bullet::sendLink()
{
    emit sendLink(deviceDescription,id);
}

void Bullet::sendFile()
{
    emit sendFile(deviceDescription,id);
}
