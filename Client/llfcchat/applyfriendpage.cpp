#include "applyfriendpage.h"
#include "ui_applyfriendpage.h"
#include <QPainter>
#include <QPaintEvent>
#include <QStyleOption>
#include <QRandomGenerator>
#include <QTimer>
#include "applyfrienditem.h"
#include "authenfriend.h"
#include "applyfriend.h"
#include "tcpmgr.h"
#include "usermgr.h"


ApplyFriendPage::ApplyFriendPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ApplyFriendPage)
{
    ui->setupUi(this);
    connect(ui->apply_friend_list, &ApplyFriendList::sig_show_search, this, &ApplyFriendPage::sig_show_search);
    loadApplyList();
    //接受tcp传递的authrsp信号处理
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_auth_rsp, this, &ApplyFriendPage::slot_auth_rsp);
}

ApplyFriendPage::~ApplyFriendPage()
{
    delete ui;
}

void ApplyFriendPage::AddNewApply(std::shared_ptr<AddFriendApply> apply)
{
    //先模拟头像随机，以后头像资源增加资源服务器后再显示
    int randomValue = QRandomGenerator::global()->bounded(100); // 生成0到99之间的随机整数
    int head_i = randomValue % heads.size();
	auto* apply_item = new ApplyFriendItem();
    auto apply_info = std::make_shared<ApplyInfo>(apply->_from_uid,
             apply->_name, apply->_desc,heads[head_i], apply->_name, 0, 0);
    apply_item->SetInfo( apply_info);
	QListWidgetItem* item = new QListWidgetItem;
	//qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
	item->setSizeHint(apply_item->sizeHint());
	item->setFlags(item->flags() & ~Qt::ItemIsEnabled & ~Qt::ItemIsSelectable);
	ui->apply_friend_list->insertItem(0,item);
	ui->apply_friend_list->setItemWidget(item, apply_item);
    apply_item->ShowAddBtn(true);

    _unauth_items[apply->_from_uid] = apply_item;

	//收到审核好友信号
    connect(apply_item, &ApplyFriendItem::sig_auth_friend, [this](std::shared_ptr<ApplyInfo> apply_info) {
		auto* authFriend = new AuthenFriend(this);
		authFriend->setModal(true);
        authFriend->SetApplyInfo(apply_info);
		authFriend->show();
		});
}

void ApplyFriendPage::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void ApplyFriendPage::loadApplyList()
{
    //添加好友申请
    auto apply_list = UserMgr::GetInstance()->GetApplyList();

    for(auto &apply: apply_list){
        int randomValue = QRandomGenerator::global()->bounded(100); // 生成0到99之间的随机整数
        int head_i = randomValue % heads.size();
        auto* apply_item = new ApplyFriendItem();
        apply->SetIcon(heads[head_i]);
        apply_item->SetInfo(apply);
        QListWidgetItem* item = new QListWidgetItem;
        //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
        item->setSizeHint(apply_item->sizeHint());
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled & ~Qt::ItemIsSelectable);
        ui->apply_friend_list->insertItem(0,item);
        ui->apply_friend_list->setItemWidget(item, apply_item);

        bool should_notice = false;

        if(apply->_status){
            apply_item->ShowAddBtn(false);
        }else{
             apply_item->ShowAddBtn(true);
             auto uid = apply_item->GetUid();
             _unauth_items[uid] = apply_item;
             should_notice = true;
        }

        if(should_notice)
        {
            // FIXME(yinghaoyu): 修改ui文件，让ContactUserList提前构造
            // 等待500ms其他窗口构造完毕再发送，尽量避免丢失信号
            QTimer::singleShot(500, [this]()
                               {
                                   qDebug() << "sig_set_read_point true";
                                   emit sig_set_read_point(true);
                               });

        }


        //收到审核好友信号
        connect(apply_item, &ApplyFriendItem::sig_auth_friend, [this](std::shared_ptr<ApplyInfo> apply_info) {
            auto* authFriend = new AuthenFriend(this);
            authFriend->setModal(true);
            authFriend->SetApplyInfo(apply_info);
            authFriend->show();
            });
    }
}

void ApplyFriendPage::slot_auth_rsp(std::shared_ptr<AuthRsp> auth_rsp) {
    auto uid = auth_rsp->_uid;
    auto find_iter = _unauth_items.find(uid);
    if (find_iter == _unauth_items.end()) {
        return;
    }
    _unauth_items.erase(find_iter);
    if(_unauth_items.empty())
    {
        qDebug() << "sig_set_read_point false";
        emit sig_set_read_point(false);
    }
    find_iter->second->ShowAddBtn(false);
}



