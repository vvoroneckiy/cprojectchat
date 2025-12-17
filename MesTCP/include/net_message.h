#ifndef NET_MESSAGE
#define NET_MESSAGE

#include "net_common.h"

namespace net {

  template <typename T>
  struct message_header {
    T id{};    // for what type the message is
    std::array<wchar_t, 256> name{};    // who pass this massage
  };

  template <typename T>
  struct message {
    message_header<T> header{};
    std::array<wchar_t, 256> data{};    // message content
    std::chrono::system_clock::time_point time = std::chrono::system_clock::now();
  };
  // An "owned" message is identical to a regular message, but it is associated with
  // a connection. On a server, the owner would be the client that sent the message,
  // on a client the owner would be the server.

  // Forward declare the connection
  template <typename T>
  class connection;

  template <typename T>
  struct owned_message {
    std::shared_ptr<connection<T>> remote = nullptr;
    message<T> msg;

    // Again, a friendly string maker
    friend std::ostream &operator<<(std::ostream &os, const owned_message<T> &msg)
    {
      os << msg.msg;
      return os;
    }
  };
}    // namespace net

#endif