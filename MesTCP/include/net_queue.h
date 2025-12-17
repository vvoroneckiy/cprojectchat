#ifndef NET_QUEUE
#define NET_QUEUE

#include "net_common.h"

namespace net {

  template <typename T>
  class ts_queue {
  public:
    ts_queue() = default;
    ts_queue(const ts_queue &) = delete;
    virtual ~ts_queue() { clear(); }

  public:
    // Returns and maintains item at front of Queue
    const T &front()
    {
      std::scoped_lock lock(mux_queue);
      return deqQueue.front();
    }

    // Returns and maintains item at back of Queue
    const T &back()
    {
      std::scoped_lock lock(mux_queue);
      return deqQueue.back();
    }

    // Removes and returns item from front of Queue
    T pop_front()
    {
      std::scoped_lock lock(mux_queue);
      auto t = std::move(deqQueue.front());
      deqQueue.pop_front();
      return t;
    }

    // Removes and returns item from back of Queue
    T pop_back()
    {
      std::scoped_lock lock(mux_queue);
      auto t = std::move(deqQueue.back());
      deqQueue.pop_back();
      return t;
    }

    // Adds an item to back of Queue
    void push_back(const T &item)
    {
      std::scoped_lock lock(mux_queue);
      deqQueue.push_back(std::move(item));

      std::unique_lock<std::mutex> ul(mux_blocking);
      cvBlocking.notify_one();
    }

    // Adds an item to front of Queue
    void push_front(const T &item)
    {
      std::scoped_lock lock(mux_queue);
      deqQueue.push_front(std::move(item));

      std::unique_lock<std::mutex> ul(mux_blocking);
      cvBlocking.notify_one();
    }

    // Returns true if Queue has no items
    bool empty()
    {
      std::scoped_lock lock(mux_queue);
      return deqQueue.empty();
    }

    // Returns number of items in Queue
    size_t count()
    {
      std::scoped_lock lock(mux_queue);
      return deqQueue.size();
    }

    // Clears Queue
    void clear()
    {
      std::scoped_lock lock(mux_queue);
      deqQueue.clear();
    }

    void wait()
    {
      while (empty()) {
        std::unique_lock<std::mutex> ul(mux_blocking);
        cvBlocking.wait(ul);
      }
    }

  protected:
    std::mutex mux_queue;
    std::deque<T> deqQueue;
    std::condition_variable cvBlocking;
    std::mutex mux_blocking;
  };
}    // namespace net

#endif