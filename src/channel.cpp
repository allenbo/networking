#include "networking/channel.hpp"
#include "networking/monitor.hpp"
#include "common/all.hpp"

using namespace common;

namespace networking {

Channel::Channel(bool async)
    :socket_(-1), addrinfo_(nullptr), alive_(false),
     async_(async), handler_(nullptr),
     read_packet_(nullptr), write_packet_(nullptr),
     async_read_(false), read_waiter_(0),
     async_read_mutex_(), async_read_cond_(&async_read_mutex_),
     async_write_(false), write_waiter_(0),
     async_write_mutex_(), async_write_cond_(&async_write_mutex_) {
}

Channel::Channel(std::string addr, bool async)
    :socket_(-1), addrinfo_(nullptr), alive_(false),
     async_(async), handler_(nullptr),
     read_packet_(nullptr), write_packet_(nullptr),
     async_read_(false), read_waiter_(0),
     async_read_mutex_(), async_read_cond_(&async_read_mutex_),
     async_write_(false), write_waiter_(0),
     async_write_mutex_(), async_write_cond_(&async_write_mutex_) {

  std::vector<std::string> pair = VarString::split(addr, ":");
  CHECK_EQ(pair.size(), 2);
  CHECK_NE(pair[0], "");
  CHECK_NE(pair[1], "");
  LOG(DEBUG) << "Host:port " << pair[0] << ":" << pair[1] << std::endl;

  std::string host = pair[0];
  std::string port = pair[1];

  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  CHECK_EQ(getaddrinfo(host.c_str(),
                       port.c_str(),
                       &hints,
                       &addrinfo_),
           0);
  int st = connect();
  CHECK(st == 0);
  LOG(DEBUG) << "Connected" << std::endl;
  alive_ = true;
}

Channel::~Channel() {
  if (addrinfo_)
    freeaddrinfo(addrinfo_);
  
  if (alive_) {
    close();
  }
}

int Channel::set_socket(int socket) {
  ScopeLock _(&change_mutex_);
  if (socket == -1 || socket_ != -1) {
    return 1;
  }
  socket_ = socket;
  LOG(DEBUG) << "Set socket to " << socket_ << std::endl;
  return 0;
}

int Channel::send(const Buffer& buffer) {
  ScopeLock _(&change_mutex_);
  CHECK_NE(socket_, -1);
  CHECK_NE(async_, true);
  packet p(buffer);
  
  while(true) {
    int len = ::write(socket_, p.curr_bytes, p.remain_size());
    if ((size_t)len != p.remain_size()) {
      p.curr_bytes += len;
    } else {
      break;
    }
  }

  LOG(DEBUG) << "Channel " << socket_ << " Sent out message with length [" << buffer.size() << "]" << std::endl;
  return 0;
}

int Channel::recv(Buffer* buffer) {
  ScopeLock _(&change_mutex_);
  CHECK_NE(socket_, -1);
  CHECK_NE(async_, true);

  int total_len;
  int len = ::read(socket_, (char*)&total_len, sizeof(int));

  if (len == 0){
    return 1;
  }

  int curr_len = 0;
  char bytes[1024];

  while (curr_len < total_len) {
    len = ::read(socket_, (char*)bytes, 1024);
    if (len == 0) {
      return 1;
    }
    buffer->write((Buffer::Byte*)bytes, len);

    curr_len += len;
  }
  LOG(DEBUG) << "Channel " << socket_ << " Received a message with length [" << buffer->size() << "]" << std::endl;
  buffer->reset();
  return 0;
}

int Channel::isend(const Buffer& buffer) {
  async_write_mutex_.lock(); // weird scope lock error

  write_waiter_ ++;
  while (async_write_) {
    async_write_cond_.wait();
    if (!is_alive()) {
      async_write_mutex_.unlock();
      return 1;
    }
  }
  write_waiter_ --;

  LOG(DEBUG) << "Channel " << socket_ << " Isenting a message with length [" << buffer.size() << "]" << std::endl;
  async_write_ = true;
  write_packet_ = new packet(buffer);

  write_pdu();
  if (write_packet_) {
    Monitor::get_instance()->watch_write(this);
  }
  async_write_mutex_.unlock();
  return 0;
}

int Channel::irecv() {
  ScopeLock _(&async_read_mutex_);

  read_waiter_ ++;
  while (async_read_) {
    async_read_cond_.wait(); 
    if (!is_alive()) {
      return 1;
    }
  }
  read_waiter_ --;

  LOG(DEBUG) << "Channel " << socket_ << " Irecv a message" << std::endl;
  async_read_ = true;
  read_packet_ = new packet();

  Monitor::get_instance()->watch_read(this);
  return 0;
}

int Channel::read_pdu() {
  CHECK_NOTNULL(read_packet_);

  if (read_packet_->size == 0) {
    int total_len;
    int len = ::read(socket_, (char*)&total_len, sizeof(int));

    if (len == 0){
      on_async_close();
      return 1;
    }

    read_packet_->set_size(total_len);
    LOG(DEBUG) << read_packet_->size << std::endl;
  }

  size_t len = ::read(socket_, read_packet_->curr_bytes, read_packet_->remain_size());
  if (len == 0){
    on_async_close();
    return 1;
  }
  read_packet_->curr_bytes += len;

  if (read_packet_->ready()) {
    Buffer buffer = read_packet_->toBuffer();
    LOG(DEBUG) << "Channel " << socket_ << " Irecved a message with length [" << buffer.size() << "]" << std::endl;

    Monitor::get_instance()->unwatch_read(this);

    cleanup_read();
    if (handler_) {
      handler_->on_recv_complete(this, buffer);
    }
  }
  return 0;
}

int Channel::write_pdu() {
  CHECK_NOTNULL(write_packet_);

  size_t len = ::write(socket_, write_packet_->curr_bytes, write_packet_->remain_size());
  write_packet_->curr_bytes += len;

  if (write_packet_->ready()) {
    Monitor::get_instance()->unwatch_write(this);
    LOG(DEBUG) << "Channel " << socket_ << " Isent a message" << std::endl;

    cleanup_write();
    if (handler_) {
      handler_->on_send_complete(this);
    }
  }
  return 0;
}

void Channel::cleanup_read() {
  CHECK_NOTNULL(read_packet_);
  delete read_packet_;
  read_packet_ = nullptr;

  async_read_ = false;
  async_read_cond_.notify_all();
}

void Channel::cleanup_write() {
  CHECK_NOTNULL(write_packet_);
  delete write_packet_;
  write_packet_ = nullptr;

  async_write_ = false;
  async_write_cond_.notify_all();
}

void Channel::on_async_close() {
  Monitor::get_instance()->unregister_channel(this);
  cleanup_read();

  if (handler_) {
    handler_->on_channel_close(this);
  }
}

int Channel::connect() {
  /* if reconnect */
  ScopeLock _(&change_mutex_);
  if (socket_ != -1) {
    LOG(DEBUG) << "Reconnect ..." << std::endl;
    ::close(socket_);
    socket_ = -1;
  }

  if (addrinfo_ == nullptr) {
    return 1;
  }

  struct addrinfo * ptr = nullptr;
  for(ptr = addrinfo_; ptr != nullptr; ptr = ptr->ai_next) {
    socket_ = ::socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (socket_ == -1) {
      continue;
    }
    if (::connect(socket_, ptr->ai_addr, (int)ptr->ai_addrlen) == 0) {
      break;
    }
    ::close(socket_);
  }

  if (ptr == nullptr) {
    return 2;
  }
  return 0;
}

void Channel::close() {
  ScopeLock _(&change_mutex_);
  if (alive_ && socket_ != -1) {
    ::close(socket_);
  }

  if (async_) {
    Monitor::get_instance()->unregister_channel(this);
  }

  alive_ = false;
  socket_ = -1;

}

void Channel::set_async_handler(Asio* handler) {
  CHECK(async_);
  ScopeLock _(&change_mutex_);
  nonblock_fd(socket_);
  handler_ = handler;
  Monitor::get_instance()->register_channel(this);
}

} // networking
