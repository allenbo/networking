#ifndef __NETWORKING_ACCEPTOR_HPP__
#define __NETWORKING_ACCEPTOR_HPP__

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "networking/channel.hpp"

namespace networking {

class Acceptor {
  public:

    Acceptor(std::string addr);
    ~Acceptor();

    /* @return 0 means success, 1 means timeout */
    int accept(Channel* ch, struct timeval tv = {0, 0});

  private:
    int socket_;
    struct addrinfo* addrinfo_;
};

} // networking

#endif
