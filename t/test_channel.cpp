#include "networking/channel.hpp"
#include "networking/acceptor.hpp"
#include "common/all.hpp"
#include <iostream>

using namespace COMMON;
using networking::Channel;
using networking::Acceptor;
using networking::Buffer;

const char* stc_msg = "hello world";
const char* cts_msg = "This is a message";
const char* alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

std::string rand_string(int len) {
  char* arr = new char[len];
  int total_len = strlen(alphabet);

  for (int i = 0; i < len ; i ++) {
    arr[i] = alphabet[rand() % total_len];
  }
  return std::string(arr);
}
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
  //Server server("8888");
  //server.start();
  //

  std::string bigmsg = rand_string(10*1000*1000);

  Channel ch("127.0.0.1:8888", false);
  Buffer buffer;
  buffer.writeString(bigmsg);
  int st = ch.send(buffer);
  std::cout << "Send out message to server" << std::endl;
  assert(st == 0);

  Buffer buffer2;
  st = ch.recv(&buffer2);
  assert(st == 0);


  std::string msg;
  st = buffer2.readString(&msg);
  assert(st == 0 && msg == bigmsg);

  //server.join();
  return 0;
}
