#include "networking/channel.hpp"
#include "networking/acceptor.hpp"
#include "common/all.hpp"

using namespace COMMON;
using networking::Channel;
using networking::Acceptor;
using networking::Buffer;

const char* stc_msg = "hello world";
const char* cts_msg = "This is a message";

class Server : public Thread {
  public:
    Server(std::string addr) {
      acceptor = new Acceptor(addr);
    }

    void run() {
      Channel ch;
      acceptor->accept(&ch);
      Buffer buffer;
      buffer.writeString(std::string(stc_msg));

      int st = ch.send(buffer);
      assert(st == 0);

      Buffer buffer2;
      st = ch.recv(&buffer2);
      assert(st == 0);

      std::string msg;
      st = buffer2.readString(&msg);
      assert(st == 0 && msg == cts_msg);
    }

    ~Server() {
      delete acceptor;
    }

  private:
    Acceptor *acceptor;
};

int main() {
  Server server("8888");
  server.run();
}
