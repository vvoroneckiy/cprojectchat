#ifndef NET_CLIENT
#define NET_CLIENT

#include "net.h"

using boost::asio::ip::tcp;

namespace net {
  template <typename T>
  class client_interface {
  public:
    client_interface() {}
    virtual ~client_interface() { disconnect(); }

  public:
    // connect to server with hostname/ip-address and port
    bool connect(const std::string &host, const uint16_t port)
    {
      try {
        // Resolve hostname/ip-address into tangiable physical address
        tcp::resolver resolver(__io_context);
        tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));

        // Create connection
        connect_ptr = std::make_unique<connection<T>>(connection<T>::owner::client, __io_context, tcp::socket(__io_context), __q_messages_in);

        // Tell the connection object to connect to server
        connect_ptr->connect_to_server(endpoints);

        // Start Context Thread
        thrContext = std::thread([this]() { __io_context.run(); });
        std::cerr << "leave connect function\n";
      } catch (std::exception &e) {
        std::cerr << "Client Exception: " << e.what() << '\n';
        return false;
      }
      return true;
    }

    // Disconnect from server
    void disconnect()
    {
      // If connection exists, and it's connected then...
      if (is_connected()) {
        // ...disconnect from server gracefully
        connect_ptr->disconnect();
      }

      // Either way, we're also done with the asio context...
      __io_context.stop();
      // ...and its thread
      if (thrContext.joinable())
        thrContext.join();

      // Destroy the connection object
      connect_ptr.release();
    }

    // Check if client is actually connected to a server
    bool is_connected()
    {
      if (connect_ptr)
        return connect_ptr->is_connected();
      else
        return false;
    }

  public:
    // Send message to server
    void send(const message<T> &msg)
    {
      if (is_connected())
        connect_ptr->send(msg);
    }

    // Retrieve queue of messages from server
    ts_queue<owned_message<T>> &get_in_comming()
    {
      return __q_messages_in;
    }

  protected:
    // asio context handles the data transfer...
    boost::asio::io_context __io_context;
    // ...but needs a thread of its own to execute its work commands
    std::thread thrContext;
    // The client has a single instance of a "connection" object, which handles data transfer
    std::unique_ptr<connection<T>> connect_ptr;

  private:
    // This is the thread safe queue of in_comming messages from server
    ts_queue<owned_message<T>> __q_messages_in;
  };
}    // namespace net

#endif