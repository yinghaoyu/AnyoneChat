#ifndef CHATPAGE_H
#define CHATPAGE_H

#include <QWidget>
#include "userdata.h"
#include "chatitembase.h"
#include <QMap>

namespace Ui {
class ChatPage;
}

class ChatPage : public QWidget
{
    Q_OBJECT
public:
    explicit ChatPage(QWidget *parent = nullptr);
    ~ChatPage();
    void SetChatData(std::shared_ptr<ChatThreadData> chat_data);
    void AppendChatMsg(std::shared_ptr<ChatDataBase> msg);
    void UpdateChatStatus(QString unique_id, int status);
    void SetSelfIcon(ChatItemBase* pChatItem, QString icon);
protected:
    void paintEvent(QPaintEvent *event) override;
    void closeEvent(QCloseEvent* event) override;
private slots:
    void on_send_btn_clicked();
private:
    void clearItems();
    Ui::ChatPage *ui;
    std::shared_ptr<ChatThreadData> _chat_data;
    QMap<QString, QWidget*>  _bubble_map;
    QHash<QString, ChatItemBase*> _unrsp_item_map;
signals:
    void sig_window_close();
};

#endif // CHATPAGE_H
