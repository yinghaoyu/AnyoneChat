#include "logindialog.h"
#include "ui_logindialog.h"
#include <QDebug>
#include "httpmgr.h"
#include "tcpmgr.h"
#include <QRegExp>
#include <QRegularExpression>
#include <QPainter>
#include <QPainterPath>
#include <QPicture>
#include <QColor>
#include <QTimer>
#include "filetcpmgr.h"

LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);

    ui->reg_label->SetState("normal","hover","","selected","selected_hover","");
    ui->reg_label->setCursor(Qt::PointingHandCursor);
    connect(ui->reg_label, &ClickedLabel::clicked, this, &LoginDialog::switchRegister);

    ui->forget_label->SetState("normal","hover","","selected","selected_hover","");
    ui->forget_label->setCursor(Qt::PointingHandCursor);
    connect(ui->forget_label, &ClickedLabel::clicked, this, &LoginDialog::slot_forget_pwd);
    initHttpHandlers();
    //连接登录回包信号
    connect(HttpMgr::GetInstance().get(), &HttpMgr::sig_login_mod_finish, this,
            &LoginDialog::slot_login_mod_finish);

    //连接tcp连接请求的信号和槽函数
    connect(this, &LoginDialog::sig_connect_tcp, TcpMgr::GetInstance().get(), &TcpMgr::slot_tcp_connect);
    //连接tcp管理者发出的连接成功信号
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_con_success, this, &LoginDialog::slot_tcp_con_finish);
    //连接tcp管理者发出的登陆失败信号
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_login_failed, this, &LoginDialog::slot_login_failed);

    //连接tcp连接资源服务器请求的信号和槽函数
    connect(this, &LoginDialog::sig_connect_res_server,
            FileTcpMgr::GetInstance().get(), &FileTcpMgr::slot_tcp_connect);

    //连接资源管理tcp发出的连接成功信号
    connect(FileTcpMgr::GetInstance().get(), &FileTcpMgr::sig_con_success, this, &LoginDialog::slot_res_con_finish);

    QPixmap email_pm(QString(":/res/email.png"));
    // 将图片缩放到label的大小
    ui->email_lb->setPixmap(email_pm.scaled( ui->email_lb->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->email_lb->setScaledContents(true);

    QPixmap pass_pm(QString(":/res/passwd.png"));
    // 将图片缩放到label的大小
    ui->pass_label->setPixmap(pass_pm.scaled( ui->pass_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->pass_label->setScaledContents(true);

    ui->login_btn->setNormalColor(QColor(0,120,212));
    ui->login_btn->setHoverColor(QColor(0,120,212).lighter(110));
    ui->login_btn->setDisableColor(QColor(199,199,199));
    ui->login_btn->setTextColor(QColor(255,255,255));
    ui->login_btn->setEnabled(true);

    this->notification = new Notification(this);
    this->notification->setGeometry(QRect(QPoint(this->rect().center().x() - (this->notification->width() / 2), this->rect().top() - (this->notification->height())), QSize(this->notification->size())));
    this->notification->hide();

    this->animation = new QTimeLine(500, this);
    this->animation->setUpdateInterval(0);
    this->animation->setFrameRange(this->rect().top() - (this->notification->height()), 20);

    connect(this->animation, &QTimeLine::frameChanged, this, [=](int frame) {
        this->notification->move(this->notification->geometry().x(), frame);
    });

    initHead();
}

LoginDialog::~LoginDialog()
{
    qDebug()<<"destruct LoginDlg";
    delete ui;
}

void LoginDialog::initHead()
{
    // 加载图片
    QPixmap originalPixmap(":/res/head_1.jpg");
      // 设置图片自动缩放
    qDebug()<< originalPixmap.size() << ui->head_label->size();
    originalPixmap = originalPixmap.scaled(ui->head_label->size(),
            Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // 创建一个和原始图片相同大小的QPixmap，用于绘制圆角图片
    QPixmap roundedPixmap(originalPixmap.size());
    roundedPixmap.fill(Qt::transparent); // 用透明色填充

    QPainter painter(&roundedPixmap);
    painter.setRenderHint(QPainter::Antialiasing); // 设置抗锯齿，使圆角更平滑
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // 使用QPainterPath设置圆角
    QPainterPath path;
    path.addRoundedRect(0, 0, originalPixmap.width(), originalPixmap.height(), 10, 10); // 最后两个参数分别是x和y方向的圆角半径
    painter.setClipPath(path);

    // 将原始图片绘制到roundedPixmap上
    painter.drawPixmap(0, 0, originalPixmap);

    // 设置绘制好的圆角图片到QLabel上
    ui->head_label->setPixmap(roundedPixmap);

}

void LoginDialog::initHttpHandlers()
{
    //注册获取登录回包逻辑
    _handlers.insert(ReqId::ID_LOGIN_USER, [this](QJsonObject jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::Success)
        {
            enableOperation(true);
        }
        switch(error)
        {
            case ErrorCodes::Success:
            {
                auto email = jsonObj["email"].toString();
                //发送信号通知tcpMgr发送长链接
		        _si = std::make_shared<ServerInfo>();

		        _si->_uid = jsonObj["uid"].toInt();
		        _si->_chat_host = jsonObj["chathost"].toString();
		        _si->_chat_port = jsonObj["chatport"].toString();
		        _si->_token = jsonObj["token"].toString();

		        _si->_res_host = jsonObj["reshost"].toString();
		        _si->_res_port = jsonObj["resport"].toString();


		        qDebug()<< "email is " << email << " uid is " << _si->_uid <<" chat host is "
		                << _si->_chat_host << " chat port is "
		                << _si->_chat_port << " token is " << _si->_token
		                << " res host is " << _si->_res_host
		                << " res port is " << _si->_res_port;
		        emit sig_connect_tcp(_si);

                break;
            }
            case ErrorCodes::Error_Json:
                showTip(tr("服务器json解析错误"));
                break;
            case ErrorCodes::PasswdInvalid:
                showTip(tr("邮箱或密码错误"));
                break;
            case ErrorCodes::RPCFailed:
                showTip(tr("获取聊天服务器ip失败"));
                break;
            default: showTip(tr("未知错误")); break;
        }
    });
}

void LoginDialog::showTip(QString str)
{
    this->notification->setText(str);
    this->notification->setGeometry(QRect(QPoint(this->rect().center().x() - (this->notification->width() / 2), this->rect().top() - (this->notification->height())), QSize(this->notification->size())));
    this->notification->show();

    this->animation->setDirection(QTimeLine::Forward);
    this->animation->start();
    QTimer::singleShot(1000, this, [=]() {
        this->animation->setDirection(QTimeLine::Backward);
        this->animation->start();
    });
}

void LoginDialog::slot_forget_pwd()
{
    qDebug()<<"slot forget pwd";
    emit switchReset();
}

bool LoginDialog::checkEmailValid(){
    auto email = ui->email_edit->text();
    QRegularExpression regex(R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)");
    bool match = regex.match(email).hasMatch();
    if(!match){
        qDebug() << "email empty " ;
        showTip(tr("邮箱地址不正确"));
        return false;
    }
    return true;
}

bool LoginDialog::checkPwdValid(){
    auto pwd = ui->pass_edit->text();
    if(pwd.length() < 6 || pwd.length() > 15){
        qDebug() << "Pass length invalid";
        //提示长度不准确
        showTip(tr("密码长度应为6~15"));
        return false;
    }

    // 创建一个正则表达式对象，按照上述密码要求
    // 这个正则表达式解释：
    // ^[a-zA-Z0-9!@#$%^&*]{6,15}$ 密码长度至少6，可以是字母、数字和特定的特殊字符
    QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*.]{6,15}$");
    bool match = regExp.match(pwd).hasMatch();
    if(!match){
        //提示字符非法
        showTip(tr("不能包含非法字符且长度为(6~15)"));
        return false;;
    }

    return true;
}

bool LoginDialog::enableOperation(bool enabled)
{
    ui->email_edit->setEnabled(enabled);
    ui->pass_edit->setEnabled(enabled);
    ui->login_btn->setDisabled(!enabled);
    return true;
}

void LoginDialog::on_login_btn_clicked()
{
    qDebug()<<"login btn clicked";
    if(checkEmailValid() == false){
        return;
    }

    if(checkPwdValid() == false){
        return ;
    }
    enableOperation(false);
    auto email = ui->email_edit->text();
    auto pwd = ui->pass_edit->text();
    //发送http请求登录
    QJsonObject json_obj;
    json_obj["email"] = email;
    json_obj["passwd"] = xorString(pwd);
    HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix+"/user_login"),
                                        json_obj, ReqId::ID_LOGIN_USER,Modules::LOGINMOD);
}

void LoginDialog::slot_login_mod_finish(ReqId id, QString res, ErrorCodes err)
{
    if(err != ErrorCodes::Success){
        showTip(tr("网络请求错误"));
        enableOperation(true);
        return;
    }

    // 解析 JSON 字符串,res需转化为QByteArray
    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());
    //json解析错误
    if(jsonDoc.isNull()){
        showTip(tr("json解析错误"));
        return;
    }

    //json解析错误
    if(!jsonDoc.isObject()){
        showTip(tr("json解析错误"));
        return;
    }


    //调用对应的逻辑,根据id回调。
    _handlers[id](jsonDoc.object());

    return;
}

void LoginDialog::slot_tcp_con_finish(bool bsuccess)
{

   if(bsuccess){
      showTip(tr("聊天服务连接成功，正在登录..."));
	  emit sig_connect_res_server(_si);
   }else{
      showTip(tr("网络异常"));
      enableOperation(true);
   }

}

void LoginDialog::slot_login_failed(int err)
{
    switch (err) {
        case ErrorCodes::Error_Json:
            showTip(tr("服务器json解析错误"));
            break;
        case ErrorCodes::UidInvalid:
            showTip(tr("用户或Token不存在"));
            break;
        case ErrorCodes::TokenInvalid:
            showTip(tr("用户Token错误"));
            break;
        default:
            showTip(tr("未知错误"));
            break;
    }
    enableOperation(true);
}

void LoginDialog::slot_res_con_finish(bool bsuccess)
{
       if(bsuccess){
          showTip(tr("聊天服务连接成功，正在登录..."));
          QJsonObject jsonObj;
          jsonObj["uid"] = _si->_uid;
          jsonObj["token"] = _si->_token;

          QJsonDocument doc(jsonObj);
          QByteArray jsonData = doc.toJson(QJsonDocument::Indented);

          //发送tcp请求给chat server
         emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_CHAT_LOGIN, jsonData);

       }else{
          showTip(tr("网络异常"));
          enableOperation(true);
       }

}
