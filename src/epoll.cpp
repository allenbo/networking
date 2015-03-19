#include "networking/epoll.hpp"

namespace networking {

EPoll::EPoll()
    :efd_(-1), mutex_(){
  efd_ = epoll_create1(0);
  CHECK_NE(efd_, -1);
}

void EPoll::watch(int fd, Pollable::MODE mod) {
  ScopeLock _(&mutex_);
  LOG(DEBUG) << "watch " << fd << (mod == Pollable::MODE::READ? " on read" : " on write") << std::endl;

  struct epoll_event event;
  event.data.fd = fd;
  if (flags_.count(fd) == 0) {
    // Not being watched
    if (mod == Pollable::MODE::READ) {
      event.events = EPOLLIN;
      flags_[fd] = Flag::READ_ONLY;
    } else {
      event.events = EPOLLOUT;
      flags_[fd] = Flag::WRITE_ONLY;
    }
    epoll_ctl(efd_, EPOLL_CTL_ADD, fd, &event);
  } else {
    // In watch, but has to change event flag
    Flag old = flags_[fd];
    if ((old == Flag::READ_ONLY && mod == Pollable::MODE::READ) ||
        (old == Flag::WRITE_ONLY && mod == Pollable::MODE::WRITE)) {
      event.events = EPOLLIN | EPOLLOUT;
      epoll_ctl(efd_, EPOLL_CTL_MOD, fd, &event);

      flags_[fd] = Flag::READ_WRITE;
    }
  }
}

void EPoll::unwatch(int fd, Pollable::MODE mod) {
  ScopeLock _(&mutex_);
  LOG(DEBUG) << "unwatch " << fd << (mod == Pollable::MODE::READ? " on read" : " on write") << std::endl;

  if (flags_.count(fd) != 0) {
    Flag old = flags_[fd];

    if ((old == Flag::READ_ONLY && mod == Pollable::MODE::READ) ||
        (old == Flag::WRITE_ONLY && mod == Pollable::MODE::WRITE)) {
      epoll_ctl(efd_, EPOLL_CTL_DEL, fd, nullptr);
      flags_.erase(fd);
    } else if ((old == Flag::READ_ONLY && mod == Pollable::MODE::WRITE) ||
               (old == Flag::WRITE_ONLY && mod == Pollable::MODE::READ)) {
      return;
    } else {
      struct epoll_event event;
      event.data.fd = fd;
      if (mod == Pollable::MODE::READ) {
        event.events = EPOLLOUT;
        flags_[fd] = Flag::WRITE_ONLY;
      } else {
        event.events = EPOLLOUT;
        flags_[fd] = Flag::READ_ONLY;
      }
      epoll_ctl(efd_, EPOLL_CTL_MOD, fd, &event);
    }
  }
}

bool EPoll::poll(std::vector<int>& reads, std::vector<int>& writes) {
  while (true) {
    int rn = 0;
    // Remove lock protection here
    // TODO: need to figure out is this safe or not
    // timeout 1 ms
    rn = epoll_wait(efd_, eventv_, MAX_EVENT_SIZE, 1);

    if (rn == -1) return false;
    if (rn == 0) continue;

    for(int i = 0; i < rn; i ++ ) {
      if (eventv_[i].events & EPOLLIN) {
        reads.push_back(eventv_[i].data.fd);
      }

      if (eventv_[i].events & EPOLLOUT) {
        writes.push_back(eventv_[i].data.fd);
      }
    }
    break;
  }
  return true;
}

}
