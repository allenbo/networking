#ifndef __NETWORKING_SELECTPOLL_HPP__
#define __NETWORKING_SELECTPOLL_HPP__

#include "networking/pollable.hpp"

namespace networking {

class SelectPoll : public Pollable {
  public:

    SelectPoll();

    void watch(int fd, Pollable::MODE mode);
    void unwatch(int fd, Pollable::MODE mode);

    bool poll(std::vector<int>& reads, std::vector<int>& writes);

  private:

    int highest_;
    fd_set read_set_;
    fd_set write_set_;

    Mutex mutex_;
};

}

#endif
