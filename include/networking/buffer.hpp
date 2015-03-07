#ifndef __NETWORKING_BUFFER_HPP__
#define __NETWORKING_BUFFER_HPP__

#include <string>

#include <stdint.h>

namespace networking {

class Buffer {
  public:
    typedef unsigned char Byte;
    static const size_t MIN_BUFFER = 1024;

    Buffer();
    Buffer(const Buffer& buffer);
    Buffer(Byte *bytes, size_t);
    ~Buffer();

    inline void reset() { curr_pos_ = 0; }
    const size_t size() const { return size_; }
    const unsigned char*  data() const { return bytes_; }

    size_t writeInt(int n);
    size_t writeUInt(unsigned int un);
    size_t writeLongInt(int64_t ln);
    size_t writeULongInt(uint64_t uln);
    size_t writeString(std::string s);
    size_t writeString(const char* s, int len);
    size_t writeByte(Byte b);
    size_t write(Byte bytes[], size_t size);

    /* @return 0 means success, -1 means failed */
    int readInt(int* n);
    int readUInt(unsigned int* un);
    int readLongInt(int64_t* ln);
    int readULongInt(uint64_t* uln);
    int readString(std::string * s);
    int readByte(Byte* b);
    int read(Byte bytes[], size_t size);
    int read(char* s, size_t size);

    void append(Buffer& buf);

  private:
    Byte   *bytes_;
    size_t cap_;
    size_t size_;
    size_t curr_pos_;
};


} // end of namespace networking

#endif
