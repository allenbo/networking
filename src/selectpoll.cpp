#include "networking/selectpoll.hpp"
#include "common/all.hpp"
#include <iostream>

using namespace common;

namespace networking {

SelectPoll::SelectPoll()
    :highest_(0), mutex_(){
}

void SelectPoll::watch(int fd, Pollable::MODE mod) {
  ScopeLock _(&mutex_);
  LOG(DEBUG) << "watch " << fd << std::endl;
  if (mod == Pollable::MODE::READ) {
    FD_SET(fd, &read_set_);
  } else {
    FD_SET(fd, &write_set_);
  }

  if (fd > highest_) {
    highest_ = fd;
    LOG(DEBUG) << "highest fd is " << highest_ << std::endl;
  }
}

void SelectPoll::unwatch(int fd, Pollable::MODE mod) {
  ScopeLock _(&mutex_);
  LOG(DEBUG) << "unwatch " << fd << std::endl;
  if (mod == Pollable::MODE::READ) {
    FD_CLR(fd, &read_set_);
  } else  {
    FD_CLR(fd, &write_set_);
  }

  int new_highest = 0;
  for(int i = 0; i <= highest_; i ++) {
    if (FD_ISSET(i, &read_set_)) {
      new_highest = i;
    }

    if (FD_ISSET(i, &write_set_)) {
      new_highest = i;
    }
  }

  highest_ = new_highest;
  LOG(DEBUG) << "highest fd is " << highest_ << std::endl;
}

bool SelectPoll::poll(std::vector<int>& reads, std::vector<int>& writes) {
  while (true) {
    fd_set read_set;
    fd_set write_set;
    int highest;
    {
      ScopeLock _(&mutex_);
      read_set = read_set_;
      write_set = write_set_;
      highest = highest_;
    }

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1000; // 1ms
    int retval = select(highest + 1, &read_set, &write_set, nullptr, &timeout);

    if (retval == -1) return false;

    for(int i = 0; i <= highest; i ++) {
      if (FD_ISSET(i, &read_set)) {
        reads.push_back(i);
      }

      if (FD_ISSET(i, &write_set)) {
        writes.push_back(i);
      }
    }

    if (reads.size() != 0 || writes.size() != 0) {
      break;
    }
  }
  return true;
}

} // networking
