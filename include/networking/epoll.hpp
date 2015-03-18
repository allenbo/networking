#ifndef __NETWORKING_EPOLL_HPP__
#define __NETWORKING_EPOLL_HPP__

#include "networking/pollable.hpp"
#include "sys/epoll.h"
#include <map>

namespace networking {

class EPoll : public Pollable {
  public:

    EPoll();

    void watch(int fd, Pollable::MODE mode);
    void unwatch(int fd, Pollable::MODE mode);

    bool poll(std::vector<int>& reads, std::vector<int>& writes);

  private:
    enum class Flag {
      READ_ONLY,
      WRITE_ONLY,
      READ_WRITE,
    };

    static const int MAX_EVENT_SIZE = 256;
    struct epoll_event eventv_[MAX_EVENT_SIZE];
    int   efd_;
    Mutex mutex_;
    std::map<int, Flag> flags_;


};

}

#endif
