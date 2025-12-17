#include "net_server.h"

namespace server_detail {
  enum class msg_type : uint32_t {
    JoinServer,
    ServerAccept,
    ServerDeny,
    ServerPing,
    MessageAll,
    ServerMessage,
    PassString
  };

  class Server : public net::server_interface<msg_type> {
  public:
    Server(uint16_t port)
        : net::server_interface<msg_type>(port) {}

  protected:
    virtual bool __on_client_connect(std::shared_ptr<net::connection<msg_type>> client)
    {
      net::message<msg_type> msg;
      msg.header.id = msg_type::ServerAccept;
      client->send(msg);
      return true;
    }

    // Called when a client appears to have disconnected
    virtual void __on_client_disconnect(std::shared_ptr<net::connection<msg_type>> client)
    {
      std::cout << "Removing client [" << client->get_id() << "] \n";
    }

    // Called when a message arrives
    virtual void __on_message(std::shared_ptr<net::connection<msg_type>> client,
                              net::message<msg_type> &msg)
    {
      switch (msg.header.id) {
      case msg_type::ServerPing: {
        std::wcout << "[" << msg.header.name.data() << "]: Ping the server\n";

        // Simply bounce message back to client
        client->send(msg);
        break;
      }

      case msg_type::MessageAll: {
        std::wcout << "[" << msg.header.name.data() << "]: Send the message to all user\n";

        //Construct a new message and send it to all clients
        net::message<msg_type> __msg;
        __msg.header.id = msg_type::ServerMessage;
        __msg.header.name = msg.header.name;
        message_all_clients(__msg, client);
        break;
      }

      case msg_type::JoinServer: {
        std::wcout << "[" << msg.header.name.data() << "] Join the server\n";
        break;
      }

      case msg_type::PassString: {
        std::wcout << "[" << msg.header.name.data() << "]: " << msg.data.data() << '\n';

        // Forward this text to all other clients
        net::message<msg_type> __msg;
        __msg.header.id = msg_type::ServerMessage;
        __msg.header.name = msg.header.name;
        __msg.data = msg.data;
        __msg.time = msg.time;
        message_all_clients(__msg, client);
        break;
      }
      }
    }
  };
}    // namespace server_detail

int main()
{
  using namespace server_detail;
  Server server(9030);
  server.start();

  while (true) {
    server.update(-1, true);
  }

  return 0;
}