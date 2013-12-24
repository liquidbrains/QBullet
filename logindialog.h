#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit LoginDialog(QWidget *parent = 0);
    QString user() ;
    QString password();
    void setUser(QString user) ;
    void setPassword(QString password);
    void setRemember(bool checked);
    bool remember();
    void setMessage(const QString proxy, const QString type = "");
    ~LoginDialog();
signals:
    void AcceptLogin(QString domain, QString username, QString password);
public slots:
    void Accept();
    void Decline();

private:
    Ui::LoginDialog *ui;
};

#endif // LOGINDIALOG_H
