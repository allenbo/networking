#ifndef __NETWORKING_CHANNEL_HPP__
#define __NETWORKING_CHANNEL_HPP__

#include "networking/buffer.hpp"
#include "common/all.hpp"

#include <string>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <string.h>

using namespace COMMON;

namespace networking {

class Channel;

class Asio {
  public:
    virtual ~Asio(){}

    virtual void on_recv_complete(Channel*, Buffer buffer) {}
    virtual void on_send_complete(Channel*) {}
    virtual void on_channel_close(Channel*) { };
};

static inline void nonblock_fd(int fd) {
  int flag = fcntl(fd, F_GETFL, 0);
  flag = flag | O_NONBLOCK;
  fcntl(fd, F_SETFL, flag);
}

static inline void block_fd(int fd) {
  int flag = fcntl(fd, F_GETFL, 0);
  flag = flag & ~O_NONBLOCK;
  fcntl(fd, F_SETFL, flag);
}

class Channel {
  CLASS_NOCOPY(Channel)
  public:

    struct packet {
      packet(const Buffer& buffer) {
        bytes = new char[buffer.size() + sizeof(int)];
        *(int*)bytes = buffer.size();
        memcpy(bytes + sizeof(int), buffer.data(), buffer.size());
        size = buffer.size() + sizeof(int);
        curr_bytes = bytes;
      }

      packet() {
        bytes = nullptr;
        curr_bytes = nullptr;
        size = 0;
      }

      ~packet() {
        if (bytes)
          delete [] bytes;
        bytes = nullptr;
        curr_bytes = nullptr;
        size = 0;
      }

      void set_size(size_t s) {
        if (bytes != nullptr) {
          delete bytes;
          bytes = nullptr;
          size = 0;
        }
        size = s + sizeof(int);
        bytes = new char[size];
        *(int*)bytes = size;
        curr_bytes = bytes + sizeof(int);
      }

      inline size_t remain_size() {
        return size - (curr_bytes - bytes);
      }

      inline bool ready() {
        return remain_size() == 0;
      }

      Buffer toBuffer() {
        Buffer b;
        b.write((Buffer::Byte*)(bytes + sizeof(int)), size - sizeof(int));
        b.reset();
        return b;
      }

      char*  bytes;
      char*  curr_bytes;
      size_t size;
    };

    Channel(bool async = false);
    Channel(std::string addr, bool async = false);
    ~Channel();

    int set_socket(int socket);
    inline int socket() { return socket_; }

    int send(const Buffer& buffer);
    int recv(Buffer* buffer);

    int isend(const Buffer& buffer);
    int irecv();

    int connect();
    void close();

    inline bool is_alive() { ScopeLock _(&change_mutex_); return alive_; }
    inline void set_alive() { ScopeLock _(&change_mutex_); alive_ = true; }

    void set_async_handler(Asio* handler);
    inline bool is_async() { ScopeLock _(&change_mutex_); return async_ == true; }

    int read_pdu();
    int write_pdu();

  private:

    int socket_;
    struct addrinfo* addrinfo_;
    bool alive_;

    bool async_;
    Asio* handler_;
    packet* read_packet_;
    packet* write_packet_;

    bool async_read_;           // async reading
    size_t read_waiter_;        // #threads waiting for irecv
    Mutex async_read_mutex_;    // async reading mutex
    Condition async_read_cond_; // async reading condition

    bool async_write_;
    size_t write_waiter_;
    Mutex async_write_mutex_;
    Condition async_write_cond_;

    Mutex change_mutex_;

    void cleanup_read();
    void cleanup_write();
    void on_async_close();
};

} // end of namespace networking

#endif
