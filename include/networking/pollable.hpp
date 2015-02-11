#ifndef __NETWORKING_POLLABLE_HPP__
#define __NETWORKING_POLLABLE_HPP__

#include <vector>
#include "common/all.hpp"

using namespace COMMON;

namespace networking {

class Pollable {
  public:

    enum class MODE {
      READ,
      WRITE,
    };

    virtual ~Pollable() {}

    /**
     * watch this file descriptor in mode
     */
    virtual void watch(int fd, MODE mode) = 0;

    /**
     * unwatch this file descriptor in mode
     */
    virtual void unwatch(int fd, MODE mode) = 0;

    /**
     * poll
     */
    virtual bool poll(std::vector<int>& reads, std::vector<int>& writes) = 0;

};

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

} // end of networking

#endif
