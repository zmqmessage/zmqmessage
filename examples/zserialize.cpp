/**
 * @file zserialize.cpp
 * Pass data from one thread to another.
 */
#include <pthread.h>
#include <vector>

#include <zmq.hpp>

#include "ZmqMessage.hpp"

/**
\example zserialize.cpp

This example launches a separate thread which receives multipart messages
contain different data types serialized in different ways
*/

//0mq context is globally available
zmq::context_t ctx(1);

// endpoint to pass data from main thread to worker and back
const char* endpoint = "inproc://transport";

struct SomeData
{
  int f1;
  char f2[20];
};

// it is possible to send SomeData objects
//  in text form which require serialization
//  and desirialization methods.


template <class charT, class traits>
std::basic_ostream<charT, traits>&
operator<<(std::basic_ostream<charT, traits>& strm,
           const SomeData& d)
{
  strm << d.f1 << ':' << d.f2;
  return strm;
}

template <class charT, class traits>
std::basic_istream<charT, traits>&
operator>>(std::basic_istream<charT, traits>& strm,
           SomeData& d)
{
  char ignore;
  strm >> d.f1  >> ignore >> d.f2;
  return strm;
}

// to send in binary form
//  because of "raw_mark" tag
struct SomeBinaryData
{
  typedef void raw_mark;

  int f1;
  char f2[20];
};

// to send in binary form
ZMQMESSAGE_BINARY_TYPE(double)
;

const char* to_worker_fields[] = {"text", "structure", "numeric"};

// worker runs in a dedicated thread
void*
receiver(void*)
{
  try
  {
    zmq::socket_t s(ctx, ZMQ_PULL);
    s.connect(endpoint);
    // socket to receive data from main thread
    //   is connected

    typedef ZmqMessage::Incoming<ZmqMessage::SimpleRouting> ZIn;

    std::string text;
    SomeData data;
    int num;

    // receive data
    ZIn(s).receive(3, to_worker_fields, 3, true) >> text >> data >> num;
    //   and print it out
    std::cout << text << ' ' << data << num << ", numeric " << std::endl;

    ZIn(s).receive(3, to_worker_fields, 3, true) >> text >>
      ZmqMessage::Binary>> data >> num;
    std::cout << text << ' ' << data << num << ", numeric " << std::endl;

    ZIn(s).receive(3, to_worker_fields, 3, true) >> text >>
      ZmqMessage::Binary>> data >> ZmqMessage::Text >> num;
    std::cout << text << ' ' << data << num << ", numeric " << std::endl;

    ZIn(s).receive(3, to_worker_fields, 3, true) >> text >>
      data >> ZmqMessage::Text >> num;
    std::cout << text << ' ' << data << ", numeric " << num << std::endl;

    SomeBinaryData binary_data;

    ZIn(s).receive(3, to_worker_fields, 3, true) >> text >>
      binary_data >> ZmqMessage::Text >> num;
    std::cout << text << ' ' << binary_data.f1 << ':' << binary_data.f2 <<
      ", numeric " << num << std::endl;

    ZIn(s).receive(3, to_worker_fields, 3, true) >> text >>
      ZmqMessage::Binary >> binary_data >> ZmqMessage::Text >> num;
    std::cout << text << ' ' << binary_data.f1 << ':' << binary_data.f2 <<
      ", numeric " << num << std::endl;

    ZIn(s).receive(3, to_worker_fields, 3, true) >> text >>
      binary_data >> ZmqMessage::Text >> num;
    std::cout << text << ' ' << binary_data.f1 << ':' << binary_data.f2 <<
      ", numeric " << num << std::endl;

    ZIn(s).receive(3, to_worker_fields, 3, true) >> text >>
      binary_data >> ZmqMessage::Binary >> num;
    std::cout << text << ' ' << binary_data.f1 << ':' << binary_data.f2 <<
      ", numeric " << num << std::endl;

    double double_num;

    ZIn(s).receive(3, to_worker_fields, 3, true) >> text >>
      binary_data >> double_num;
    std::cout << text << ' ' << binary_data.f1 << ':' << binary_data.f2 <<
      ", numeric " << double_num << std::endl;

  }
  catch(const std::exception& ex)
  {
    std::cout << "caught (worker): " << ex.what();
    exit(3);
  }

  return 0;
}

int
main(int, char**)
{
  pthread_t worker_tid;

  zmq::socket_t s(ctx, ZMQ_PUSH);
  s.bind(endpoint);
  // socket to talk to worker is bound

  try
  {
    // start worker
    pthread_create(&worker_tid, 0, receiver, 0);

    SomeData data;
    data.f1 = 123;
    strcpy(data.f2, "a string");

    typedef ZmqMessage::Outgoing<ZmqMessage::SimpleRouting> ZOut;

    ZOut(s, 0) << "Sent in default mode" << data << 100 <<
      ZmqMessage::Flush;

    ZOut(s, 0) << "Sent in binary mode" << ZmqMessage::Binary << data << 100 <<
      ZmqMessage::Flush;;

    ZOut(s, 0) << "Structure in binary mode, numeric as text" <<
      ZmqMessage::Binary << data << ZmqMessage::Text << 100 <<
      ZmqMessage::Flush;

    ZOut(s, 0) << "Structure in default (text) mode, numeric as binary" <<
      data << ZmqMessage::Binary << 100 << ZmqMessage::Flush;;

    SomeBinaryData binary_data;
    binary_data.f1 = 123;
    strcpy(binary_data.f2, "another string");

    ZOut(s, 0) << "Sent in default mode" << binary_data << 100 <<
      ZmqMessage::Flush;

    ZOut(s, 0) << "Sent in binary mode" << ZmqMessage::Binary << data <<
      100 << ZmqMessage::Flush;

    ZOut(s, 0) << "Structure in binary mode, numeric as text" <<
      ZmqMessage::Binary << data << ZmqMessage::Text << 100 <<
      ZmqMessage::Flush;

    ZOut(s, 0) << "Structure in default (binary) mode, numeric as binary" <<
      data << ZmqMessage::Binary << 100 << ZmqMessage::Flush;

    ZOut(s, 0) << "Structure in default (binary) mode, "
      "numeric as binary too" <<
      data << (double)100.1 << ZmqMessage::Flush;
  }
  catch(const std::exception& ex)
  {
    std::cout << "caught (main): " << ex.what();
    exit(3);
  }

  pthread_join(worker_tid, 0);
  // thread is completed and sockets are closed
}

