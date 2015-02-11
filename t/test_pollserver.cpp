#include "networking/channel.hpp"
#include "networking/acceptor.hpp"
#include "common/all.hpp"
#include <map>
#include <iostream>

using namespace COMMON;
using networking::Channel;
using networking::Asio;
using networking::Acceptor;
using networking::Buffer;

const char* stc_msg = "hello world";
const char* cts_msg = "This is a message";

class Server : public Asio {
  public:
    Server(std::string addr) {
      acceptor_ = new Acceptor(addr);
    }

    void on_recv_complete(Channel* ch, Buffer buffer) {
      int st;
      std::string msg;
      st = buffer.readString(&msg);

      Buffer buffer2;
      buffer2.writeString(std::string(msg));
      st = ch->isend(buffer2);
    }

    void on_send_complete(Channel* ch) {
      ch->close();
      channels_.erase(ch->socket());
      delete ch;
    }

    void on_channel_close(Channel* ch) {
      on_send_complete(ch);
    }

    void loop() {
      while(true) {
        std::cout << "Waiting for channel" << std::endl;        
        Channel *ch = new Channel(true);
        acceptor_->accept(ch);
        ch->set_async_handler(this);
        std::cout << "Got a new channel " << ch->socket() << std::endl;

        int st = ch->irecv();
        assert(st == 0);
      }
    }

    ~Server() {
      delete acceptor_;
      auto it = channels_.begin();
      for(; it != channels_.end(); it ++) {
        delete it->second;
      }
    }

  private:
    Acceptor *acceptor_;
    std::map<int, Channel*> channels_;
};

int main() {
  Server server("8888");
  AsyncMethod async;
  async.start_async(&Server::loop, &server);
  async.wait();
}
