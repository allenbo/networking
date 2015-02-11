#ifndef __NETWORKING_MONITOR_HPP__
#define __NETWORKING_MONITOR_HPP__

#include "networking/channel.hpp"
#include "networking/pollable.hpp"
#include "common/all.hpp"
#include <string>
#include <map>

namespace networking {

class Monitor: public COMMON::Thread {
  public:
    static Monitor* get_instance();
    ~Monitor();

    int register_channel(Channel* ch);
    int unregister_channel(Channel* ch);

    int watch_read(Channel* ch);
    int watch_write(Channel* ch);
    int unwatch_read(Channel* ch);
    int unwatch_write(Channel* ch);

    void run();

  private:

    Monitor();
    Monitor(const Monitor& o);
    Monitor& operator=(const Monitor& o);

    std::map<int, Channel*> channels_;
    static Monitor *instance_;
    Pollable* poll_manager_;

    friend void init_monitor();
};

} // end of networking

#endif
