#include "net_client.h"
#include <queue>
#include <mutex>
#include <memory>
#include <QApplication>
#include <QWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QInputDialog>
#include <QString>
#include <QListWidget>
#include <QSet>
// Windows API for detaching console at runtime
#ifdef _WIN32
#include <windows.h>
#endif

namespace user_detail {
  enum class msg_type : uint32_t {
    JoinServer,
    ServerAccept,
    ServerDeny,
    ServerPing,
    MessageAll,
    ServerMessage,
    PassString
  };

  class Client : public net::client_interface<msg_type> {
  public:
    void ping_server()
    {
      net::message<msg_type> msg;
      msg.header.id = msg_type::ServerPing;
      msg.header.name = user_name;
      send(msg);
    }

    void message_all()
    {
      net::message<msg_type> msg;
      msg.header.id = msg_type::MessageAll;
      msg.header.name = user_name;
      send(msg);
    }

    void join_server()
    {
      std::cout << "Input Your Name: ";
      std::wcin.get(user_name.data(), user_name.size());
      std::wcin.clear();
      std::wcin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

      net::message<msg_type> msg;
      msg.header.id = msg_type::JoinServer;
      msg.header.name = user_name;
      send(msg);
    }

    void send_msg(std::wstring &__data)
    {
      net::message<msg_type> msg;
      msg.header.id = msg_type::PassString;
      msg.header.name = user_name;
      for (unsigned int i{}, j{}; j < __data.size(); ++i, ++j)
        msg.data[i] = __data[j];

      send(msg);
    }

    // join using std::wstring (for Qt)
    void join_server_from_wstring(const std::wstring &name)
    {
      std::fill(user_name.begin(), user_name.end(), L'\0');
      for (size_t i = 0; i < name.size() && i < user_name.size(); ++i)
        user_name[i] = name[i];

      net::message<msg_type> msg;
      msg.header.id = msg_type::JoinServer;
      msg.header.name = user_name;
      send(msg);
    }

    // send using std::wstring (for Qt)
    void send_msg_w(const std::wstring &__data)
    {
      net::message<msg_type> msg;
      msg.header.id = msg_type::PassString;
      msg.header.name = user_name;
      std::fill(msg.data.begin(), msg.data.end(), L'\0');
      for (unsigned int i = 0; i < __data.size() && i < msg.data.size(); ++i)
        msg.data[i] = __data[i];

      send(msg);
    }

  public:
    std::array<wchar_t, 256> user_name{};
  };
}    // namespace user_detail

// Qt chat window (no Q_OBJECT)
class ChatWindow : public QWidget {
public:
  ChatWindow(QWidget *parent = nullptr) : QWidget(parent)
  {
    textView = new QTextEdit(this);
    textView->setReadOnly(true);
    input = new QLineEdit(this);
    sendBtn = new QPushButton("Send", this);
    userList = new QListWidget(this);
    userList->setMaximumWidth(180);

    // input + send horizontal
    auto *h = new QHBoxLayout();
    h->addWidget(input);
    h->addWidget(sendBtn);

    // left: chat (text + input), right: users
    auto *leftV = new QVBoxLayout();
    leftV->addWidget(textView);
    leftV->addLayout(h);

    auto *mainH = new QHBoxLayout(this);
    mainH->addLayout(leftV);
    mainH->addWidget(userList);
    setLayout(mainH);
    setWindowTitle("MesTCP Chat Client (Qt)");

    // Network client
    client = std::make_unique<user_detail::Client>();
    client->connect("127.0.0.1", 9030);

    // Ask for user name
    bool ok = false;
    QString qname = QInputDialog::getText(this, "Name", "Enter your name:", QLineEdit::Normal, QString(), &ok);
    if (!ok || qname.isEmpty())
      qname = QString("User");
    client->join_server_from_wstring(qname.toStdWString());

    connect(sendBtn, &QPushButton::clicked, [this]() { onSend(); });
    connect(input, &QLineEdit::returnPressed, [this]() { onSend(); });
    connect(userList, &QListWidget::itemClicked, [this](QListWidgetItem *it){
      if(!it) return;
      QString name = it->data(Qt::UserRole).toString();
      if(name.isEmpty()) name = it->text();
      toggleMuteForUser(name);
    });

    pollTimer = new QTimer(this);
    connect(pollTimer, &QTimer::timeout, [this]() { pollIncoming(); });
    pollTimer->start(50);
  }

  ~ChatWindow() override
  {
    if (client) client->disconnect();
  }

private:
  // helper to get my name from client
  QString myName() const {
    if(!client) return QString();
    std::wstring wn(client->user_name.data());
    return QString::fromStdWString(wn);
  }

  void addOrRefreshUser(const QString &name) {
    if (name.isEmpty()) return;
    // find by stored user role
    for (int i = 0; i < userList->count(); ++i) {
      auto *it = userList->item(i);
      if (it->data(Qt::UserRole).toString() == name) {
        // update text to show muted flag
        QString display = name;
        if (mutedUsers.contains(name)) display += " (muted)";
        it->setText(display);
        return;
      }
    }
    // new
    QString display = name;
    if (mutedUsers.contains(name)) display += " (muted)";
    auto *it = new QListWidgetItem(display);
    it->setData(Qt::UserRole, name);
    userList->addItem(it);
  }

  void toggleMuteForUser(const QString &name) {
    if (name.isEmpty()) return;
    QString me = myName();
    if (name == me) return; // can't mute self
    bool nowMuted = false;
    if (mutedUsers.contains(name)) {
      mutedUsers.remove(name);
      nowMuted = false;
    } else {
      mutedUsers.insert(name);
      nowMuted = true;
    }
    addOrRefreshUser(name);
    // NOTE: mute is local-only. To stop this client's messages reaching muted users,
    // we will mark outgoing messages with a per-message exclude list (handled in onSend).
  }

  void onSend()
  {
    QString txt = input->text();
    if (txt.isEmpty()) return;
    // Показываем собственное сообщение локально
    textView->append(QString("Me: %1").arg(txt));
    // If we have locally muted users, add an exclude header so those users ignore this message.
    QString payload = txt;
    if (!mutedUsers.empty()) {
      QStringList excl;
      for (const QString &u : mutedUsers) excl << u;
      payload = QString("/exclude:%1;%2").arg(excl.join(',')).arg(txt);
    }
    client->send_msg_w(payload.toStdWString());
    input->clear();
  }

  void pollIncoming()
  {
    if (!client) return;
    auto &q = client->get_in_comming();
    while (!q.empty()) {
      auto owned = q.pop_front();
      auto &msg = owned.msg;
      switch (msg.header.id) {
      case user_detail::msg_type::ServerAccept:
        textView->append("Server: Accepted connection");
        break;
      case user_detail::msg_type::ServerPing:
        textView->append("Server: Ping reply");
        break;
      case user_detail::msg_type::ServerMessage: {
         std::wstring wname(msg.header.name.data());
         std::wstring wdata(msg.data.data());
         QString qname = QString::fromStdWString(wname);
         QString qdata = QString::fromStdWString(wdata).trimmed();
         // If message contains per-message exclude header "/exclude:user1,user2;message"
         const QString exclPrefix = "/exclude:";
         if (qdata.startsWith(exclPrefix)) {
           int sep = qdata.indexOf(';', exclPrefix.length());
           if (sep > 0) {
             QString list = qdata.mid(exclPrefix.length(), sep - exclPrefix.length());
             QStringList parts = list.split(',', Qt::SkipEmptyParts);
             for (QString &p : parts) p = p.trimmed();
             // update user list even if message is excluded
             addOrRefreshUser(qname);
             // if I am in the exclude list -> don't show
             if (parts.contains(myName())) break;
             // otherwise strip header and continue with actual message
             qdata = qdata.mid(sep + 1).trimmed();
           } else {
             // malformed header -> ignore entirely
             break;
           }
         }
         // ignore legacy control commands (not shown in chat)
         if (qdata.startsWith("/block:") || qdata.startsWith("/unblock:") ||
             qdata.startsWith("/mute:") || qdata.startsWith("/unmute:")) {
           break;
         }
         // update user list
         addOrRefreshUser(qname);
         // if sender is muted locally, skip showing
         if (mutedUsers.contains(qname)) break;
         QString line = qname + ": " + qdata;
         textView->append(line);
         break;
       }
      case user_detail::msg_type::PassString: {
        std::wstring wname(msg.header.name.data()); // original sender of passstring (may be irrelevant)
        std::wstring wdata(msg.data.data());
        QString qdata = QString::fromStdWString(wdata).trimmed();
        // If PassString contains per-message exclude header, process same as ServerMessage
        const QString exclPrefix2 = "/exclude:";
        if (qdata.startsWith(exclPrefix2)) {
          int sep = qdata.indexOf(';', exclPrefix2.length());
          if (sep > 0) {
            QString list = qdata.mid(exclPrefix2.length(), sep - exclPrefix2.length());
            QStringList parts = list.split(',', Qt::SkipEmptyParts);
            for (QString &p : parts) p = p.trimmed();
            QString qname = QString::fromStdWString(wname);
            addOrRefreshUser(qname);
            if (parts.contains(myName())) break;
            qdata = qdata.mid(sep + 1).trimmed();
            if (!mutedUsers.contains(qname)) textView->append(qname + ": " + qdata);
          }
          // malformed -> ignore
          break;
        }
        // ignore legacy control commands
        if (qdata.startsWith("/block:") || qdata.startsWith("/unblock:") ||
            qdata.startsWith("/mute:") || qdata.startsWith("/unmute:")) {
          break;
        }
        // plain pass string
        {
          QString qname = QString::fromStdWString(wname);
          addOrRefreshUser(qname);
          if (!mutedUsers.contains(qname)) {
            textView->append(qname + ": " + qdata);
          }
        }
        break;
       }
      default:
        break;
      }
    }
  }

  QTextEdit *textView{nullptr};
  QLineEdit *input{nullptr};
  QPushButton *sendBtn{nullptr};
  QListWidget *userList{nullptr};
  QSet<QString> mutedUsers;
  QTimer *pollTimer{nullptr};
  std::unique_ptr<user_detail::Client> client;
};

// main
int main(int argc, char **argv)
{
  // Detach console if present (prevents console window when running the GUI exe)
#ifdef _WIN32
  FreeConsole();
#endif
  QApplication app(argc, argv);
  ChatWindow w;
  w.resize(600, 400);
  w.show();
  return app.exec();
}