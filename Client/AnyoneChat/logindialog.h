#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QTimeLine>
#include "global.h"
#include "notification.h"

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();
private:
    void initHead();
    void initHttpHandlers();
    void showTip(QString str);
    bool checkEmailValid();
    bool checkPwdValid();
    Ui::LoginDialog *ui;
    QMap<ReqId, std::function<void(const QJsonObject&)>> _handlers;
    bool enableOperation(bool);
    int _uid;
    QString _token;
    std::shared_ptr<ServerInfo> _si;
    Notification* notification = Q_NULLPTR;
    QTimeLine* animation = Q_NULLPTR;
private slots:
    void slot_forget_pwd();
    void on_login_btn_clicked();
    void slot_login_mod_finish(ReqId id, QString res, ErrorCodes err);
    void slot_tcp_con_finish(bool bsuccess);
    void slot_login_failed(int);
    void slot_res_con_finish(bool bsuccess);
signals:
    void switchRegister();
    void switchReset();
    void sig_connect_tcp(std::shared_ptr<ServerInfo>);
    void sig_connect_res_server(std::shared_ptr<ServerInfo>);
};

#endif // LOGINDIALOG_H
