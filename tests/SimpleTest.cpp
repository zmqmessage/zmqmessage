/**
 * @file SimpleTest.cpp
 * @copyright Copyright (c) 2010-2011 Phorm, Inc.
 * @copyright GNU LGPL v 3.0, see http://www.gnu.org/licenses/lgpl-3.0-standalone.html
 * @author Andrey Skryabin <andrew@zmqmessage.org>, et al.
 *
 *
 * \test
 * @brief
 * We check correctness of sending, receiving, iterating multipart messages between threads.
 *
 * We use different types and methods for insertion and extraction of message parts.
 * We use binary and text modes.
 * We use all possible combinations of Routing policies
 * (simple-to-simple, X-to-X, Simple-to-X and X-to-Simple).
 */

#include "pthread.h"
#include <cstddef>
#include <cassert>
#include <cstring>

#include <string>

#define ZMQMESSAGE_DYNAMIC_DEFAULT_CAPACITY 2

#include "ZmqMessage.hpp"

#ifdef HEADERONLY
# include "ZmqMessageImpl.hpp"
#endif

#define ARRAY_LEN(arr) sizeof(arr)/sizeof((arr)[0])

const char* endpoint = "inproc://simple-test";
const char* payload = "0123456789";
const size_t payloadsz = 10;

const char* req_parts[] = {"part1", "second", "payload", "bin part", "num text part"};
const char* res_parts[] = {"id", "status"};

const char* STATUS = "OK STATUS";
const char* SECOND_PART = "second part";
const int BIN_PART = 567098;
const int NUM_TEXT_PART = 196670;

void
my_free(void *data, void *hint)
{
  ::free(data);
}

class CountingObserver :
  public ZmqMessage::SendObserver, public ZmqMessage::ReceiveObserver
{
public:
  int sent;
  int received;
  int received_full;
  int flushed_successful;

  CountingObserver() : sent(0), received(0), received_full(0),
    flushed_successful(0)
  {}

  virtual
  void
  on_send_part(zmq::message_t& msg)
  {
    std::cout << "on_part: len = " << msg.size() << std::endl;
    ++sent;
  }

  virtual
  void
  on_receive_part(zmq::message_t& msg, bool has_more)
  {
    ++received;
    if (!has_more)
    {
      ++received_full;
    }
  }

  virtual
  void
  on_flush()
  {
    ++flushed_successful;
  }

};

template <typename Routing, typename Storage, int socktype>
void* req(void* arg)
{
  zmq::context_t& ctx = *static_cast<zmq::context_t*>(arg);
  zmq::socket_t s(ctx, socktype);

  sleep(1); //wait for bind
  s.connect(endpoint);
  std::cout << "req: connected" << std::endl;

  std::string part1 = "part 1";
  void* buf = ::malloc(payloadsz);
  memcpy(buf, payload, payloadsz);

  CountingObserver obs;

  ZmqMessage::Outgoing<Routing> outgoing(
    ZmqMessage::OutOptions(s, ZmqMessage::OutOptions::NONBLOCK, &obs));

  //request
  outgoing << part1 << SECOND_PART <<
    ZmqMessage::RawMessage(buf, payloadsz, &my_free) <<
    ZmqMessage::Binary << BIN_PART <<
    ZmqMessage::Text << NUM_TEXT_PART <<
    ZmqMessage::Flush;

  std::cout << "req: request sent" << std::endl;

  assert(obs.flushed_successful == 1);
  assert(obs.sent == 5);

  //response
  ZmqMessage::Incoming<Routing, Storage> incoming(s);

  incoming.receive(
    ARRAY_LEN(res_parts), res_parts, ARRAY_LEN(res_parts), true);

  std::cout << "req: response received" << std::endl;

  std::string id = ZmqMessage::get_string(incoming[0]);
  std::string status = ZmqMessage::get_string(incoming[1]);

  assert(status == STATUS);

  //inserter
  status.clear();

  incoming >> ZmqMessage::Skip //id
    >> status;
  assert(status == STATUS);

  try
  {
    std::string s;
    incoming >> s;
    assert(!"no more parts expected");
  }
  catch (const ZmqMessage::NoSuchPartError& e)
  {
    std::cout << "caught NoSuchPartError (OK): " << e.what() << std::endl;
  }

  pthread_exit(0);
}

template <typename Routing, typename Storage, int socktype>
void* res(void* arg)
{
  zmq::context_t& ctx = *static_cast<zmq::context_t*>(arg);
  zmq::socket_t s(ctx, socktype);

  s.bind(endpoint);
  std::cout << "res: bound" << std::endl;

  //request
  ZmqMessage::Incoming<Routing, Storage> incoming(s);

  CountingObserver observer;
  incoming.set_receive_observer(&observer);

  incoming.receive(
    ARRAY_LEN(req_parts), req_parts, ARRAY_LEN(req_parts), true);
  std::cout << "res: request received" << std::endl;

  assert(observer.received == ARRAY_LEN(req_parts));
  assert(observer.received_full == 1);

  //extract one
  std::string rec_payload = ZmqMessage::get_string(incoming[2]);
  assert(rec_payload == payload);

  //extract all
  std::string part1, part2, part3;
  int part4, part5;
  assert(incoming.extracted() == 0);
  incoming >> part1 >> part2 >> part3;
  assert(incoming.extracted() == 3);
  incoming >>
    ZmqMessage::Binary >> part4 >>
    ZmqMessage::Text >> part5;
  assert(incoming.extracted() == 5);
  assert(part2 == SECOND_PART);
  assert(part4 == BIN_PART);
  assert(part5 == NUM_TEXT_PART);

  //response
  ZmqMessage::Outgoing<Routing> outgoing(
    s, incoming, ZmqMessage::OutOptions::NONBLOCK);

  outgoing << "ID!" << STATUS;
  outgoing.flush();
  std::cout << "res: response sent" << std::endl;

  pthread_exit(0);
}

template <
  typename Storage,
  typename ReqRouting, int reqsocktype,
  typename ResRouting, int ressocktype>
void test(const char* name)
{
  zmq::context_t ctx(1);
  pthread_t req_thread, res_thread;

  std::cout << ">>>>>>>>>>>>>  testing " << name << std::endl;

  pthread_create(
    &req_thread, NULL, &req<ReqRouting, Storage, reqsocktype>, &ctx);
  pthread_create(
    &res_thread, NULL, &res<ResRouting, Storage, ressocktype>, &ctx);

  pthread_join(req_thread, NULL);
  pthread_join(res_thread, NULL);
  pthread_detach(req_thread);
  pthread_detach(res_thread);
  std::cout << "<<<<<<<<<<<<<< threads joined" << std::endl;
}

void test_time()
{
  char* str = (char*)malloc(6);
  strcpy(str, "-8956");
  zmq::message_t msg(str, 3, &my_free); // -89
  time_t tm = ZmqMessage::get_time(msg);
  assert(tm == -89);
}

void
test_detach()
{
  zmq::context_t ctx(1);
  zmq::socket_t s(ctx, ZMQ_PUSH);
  ZmqMessage::Outgoing<ZmqMessage::SimpleRouting> out(
    s,
    ZmqMessage::OutOptions::EMULATE_BLOCK_SENDS |
    ZmqMessage::OutOptions::CACHE_ON_BLOCK);
  out << "ghjfkjh" << 12 << ZmqMessage::NullMessage << ZmqMessage::Flush;

  typedef std::auto_ptr<ZmqMessage::Multipart> MultipartPtr;
  MultipartPtr p(out.detach());
  assert(p->size() == 3);
  assert(ZmqMessage::get_string((*p)[1]) == "12");
}

template <typename Storage>
void
test_for_storage()
{
  test<Storage, ZmqMessage::SimpleRouting, ZMQ_REQ, ZmqMessage::SimpleRouting, ZMQ_REP>(
    "simple routing");
  test<Storage, ZmqMessage::SimpleRouting, ZMQ_REQ, ZmqMessage::XRouting, ZMQ_XREP>(
    "simple to X routing");
  test<Storage, ZmqMessage::XRouting, ZMQ_XREQ, ZmqMessage::SimpleRouting, ZMQ_REP>(
    "X to simple routing");
  test<Storage, ZmqMessage::XRouting, ZMQ_XREQ, ZmqMessage::XRouting, ZMQ_XREP>(
    "X to X routing");
}

int main()
{
  std::cout << "\n-----------\nmain: storage StackPartsStorage:\n------------" << std::endl;
  test_for_storage<ZmqMessage::StackPartsStorage<ARRAY_LEN(req_parts)> >();

  std::cout << "\n-----------\nmain: storage DynamicPartsStorage:\n------------" << std::endl;
  test_for_storage<ZmqMessage::DynamicPartsStorage<> >();

  std::cout << "\n-----------\nmain: small tests:\n------------" << std::endl;
  //small tests
  test_time();
  test_detach();
  return 0;
}
