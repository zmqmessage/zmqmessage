/**
 * @file SimpleTest.cpp
 * @author askryabin
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

#include "ZmqMessage.hpp"

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

template <typename Routing, int socktype>
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

  ZmqMessage::Outgoing<Routing> outgoing(
    s, ZmqMessage::OutOptions::NONBLOCK);

  //request
  outgoing << part1 << SECOND_PART <<
    ZmqMessage::RawMessage(buf, payloadsz, &my_free) <<
    ZmqMessage::Binary << BIN_PART <<
    ZmqMessage::Text << NUM_TEXT_PART <<
    ZmqMessage::Flush;

  std::cout << "req: request sent" << std::endl;

  //response
  ZmqMessage::Incoming<Routing> incoming(s);

  incoming.receive(
    ARRAY_LEN(res_parts), res_parts, ARRAY_LEN(res_parts), true);

  std::cout << "req: response received" << std::endl;

  std::string id = ZmqMessage::get_string(incoming[0]);
  std::string status = ZmqMessage::get_string(incoming[1]);

  assert(status == STATUS);

  //inserter
  id.clear(); status.clear();

  incoming >> id >> status;
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

template <typename Routing, int socktype>
void* res(void* arg)
{
  zmq::context_t& ctx = *static_cast<zmq::context_t*>(arg);
  zmq::socket_t s(ctx, socktype);

  s.bind(endpoint);
  std::cout << "res: bound" << std::endl;

  //request
  ZmqMessage::Incoming<Routing> incoming(s);

  incoming.receive(
    ARRAY_LEN(req_parts), req_parts, ARRAY_LEN(req_parts), true);
  std::cout << "res: request received" << std::endl;

  //extract one
  std::string rec_payload = ZmqMessage::get_string(incoming[2]);
  assert(rec_payload == payload);

  //extract all
  std::string part1, part2, part3;
  int part4, part5;
  incoming >> part1 >> part2 >> part3 >>
    ZmqMessage::Binary >> part4 >>
    ZmqMessage::Text >> part5;
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

template <typename ReqRouting, int reqsocktype, typename ResRouting, int ressocktype>
void test(zmq::context_t& ctx_, const char* name)
{
  zmq::context_t ctx(1);
  pthread_t req_thread, res_thread;

  std::cout << "main: testing " << name << std::endl;

  pthread_create(&req_thread, NULL, &req<ReqRouting, reqsocktype>, &ctx);
  pthread_create(&res_thread, NULL, &res<ResRouting, ressocktype>, &ctx);

  pthread_join(req_thread, NULL);
  pthread_join(res_thread, NULL);
  pthread_detach(req_thread);
  pthread_detach(res_thread);
  std::cout << "main: threads joined" << std::endl;
}

int main()
{
  zmq::context_t ctx(1);

  std::cout << "main: testing simple routing" << std::endl;

  test<ZmqMessage::SimpleRouting, ZMQ_REQ, ZmqMessage::SimpleRouting, ZMQ_REP>(
    ctx, "simple routing");
  test<ZmqMessage::SimpleRouting, ZMQ_REQ, ZmqMessage::XRouting, ZMQ_XREP>(
    ctx, "simple to X routing");
  test<ZmqMessage::XRouting, ZMQ_XREQ, ZmqMessage::SimpleRouting, ZMQ_REP>(
    ctx, "X to simple routing");
  test<ZmqMessage::XRouting, ZMQ_XREQ, ZmqMessage::XRouting, ZMQ_XREP>(
    ctx, "X to X routing");

  return 0;
}
