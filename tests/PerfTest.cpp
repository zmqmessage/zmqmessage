/**
 * @file PerfTest.cpp
 * @copyright Copyright (c) 2010-2011 Phorm, Inc.
 * @copyright GNU LGPL v 3.0, see http://www.gnu.org/licenses/lgpl-3.0-standalone.html
 * @author Andrey Skryabin <andrew@zmqmessage.org>, et al.
 *
 *
 * \test
 * \brief
 * Measuring performance of receiving/transmitting multipart ZMQ message using ZmqMessage library
 * compared to plain ZMQ C++ API.
 *
 * We make 100000 request-response transactions between 2 threads and print results.
 */

#include "pthread.h"
#include <cstddef>
#include <cassert>
#include <cstring>
#include <iostream>

/**
 */
#include "../examples/StringFace.hpp"

#define ZMQMESSAGE_LOG_STREAM if(1); else std::cerr
#define ZMQMESSAGE_STRING_CLASS StringFace

#include "ZmqMessage.hpp"
#ifdef HEADERONLY
# include "ZmqMessageImpl.hpp"
#endif

#define ARRAY_LEN(arr) sizeof(arr)/sizeof((arr)[0])

const char* endpoint_raw = "inproc://simple-test-raw";
const char* endpoint_mes = "inproc://simple-test-mes";

const char PART1[] = "01234567890"; //10b
const char PART2[] = "aaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeee"; //100b
const char PART3[] = "aaaaaaaaaabbbbbbbbbbccccccccccdddddddddd"; //40b

const char* req_parts[] = {"part1", "part2", "part3"};

typedef ZmqMessage::StackPartsStorage<ARRAY_LEN(req_parts)> ReqStorage;
typedef ZmqMessage::StackPartsStorage<1> ResStorage;

//typedef ZmqMessage::DynamicPartsStorage<> ReqStorage;
//typedef ReqStorage ResStorage;

const size_t ITERS = 100000;

zmq::context_t ctx(1);

void
raw_sender(zmq::socket_t& s)
{
  for (size_t i = 0; i < ITERS; ++i)
  {
    zmq::message_t msg1(ARRAY_LEN(PART1)-1);
    memcpy(msg1.data(), PART1, msg1.size());
    s.send(msg1, ZMQ_SNDMORE);

    zmq::message_t msg2(ARRAY_LEN(PART2)-1);
    memcpy(msg2.data(), PART2, msg2.size());
    s.send(msg2, ZMQ_SNDMORE);

    zmq::message_t msg3(ARRAY_LEN(PART3)-1);
    memcpy(msg3.data(), PART3, msg3.size());
    s.send(msg3);

    zmq::message_t msg_res;
    s.recv(&msg_res, 0);

    if (i % 1000 == 0)
    {
      std::cout << ".";
      std::cout.flush();
    }
  }
}

void
multipart_sender(zmq::socket_t& s)
{
  for (size_t i = 0; i < ITERS; ++i)
  {
    ZmqMessage::Outgoing<ZmqMessage::SimpleRouting> outgoing(s, 0);

    outgoing
      << StringFace(PART1, ARRAY_LEN(PART1)-1)
      << StringFace(PART2, ARRAY_LEN(PART2)-1)
      << StringFace(PART3, ARRAY_LEN(PART3)-1)
      << ZmqMessage::Flush;

    ZmqMessage::Incoming<ZmqMessage::SimpleRouting, ResStorage> incoming(s);

    incoming.receive(1, true);

    if (i % 1000 == 0)
    {
      std::cout << ".";
      std::cout.flush();
    }
  }
}

void*
raw_receiver(void* arg)
{
  zmq::socket_t s(ctx, ZMQ_REP);
  s.connect(static_cast<char*>(arg));

  for (size_t i = 0; i < ITERS; ++i)
  {
    zmq::message_t msg1, msg2, msg3, msg_res;
    s.recv(&msg1, 0);
    s.recv(&msg2, 0);
    s.recv(&msg3, 0);

    assert(msg1.size() == ARRAY_LEN(PART1)-1);
    assert(!memcmp(msg1.data(), PART1, msg1.size()));

    assert(msg2.size() == ARRAY_LEN(PART2)-1);
    assert(!memcmp(msg2.data(), PART2, msg2.size()));

    assert(msg3.size() == ARRAY_LEN(PART3)-1);
    assert(!memcmp(msg3.data(), PART3, msg3.size()));

    s.send(msg_res);
  }
  return 0;
}

void*
multipart_receiver(void* arg)
{
  zmq::socket_t s(ctx, ZMQ_REP);
  s.connect(static_cast<char*>(arg));

  ZmqMessage::has_more(s); //to load libzmqmessage.so

  for (size_t i = 0; i < ITERS; ++i)
  {
    ZmqMessage::Incoming<ZmqMessage::SimpleRouting, ReqStorage> incoming(s);

    incoming.receive(ARRAY_LEN(req_parts), req_parts, true);

    StringFace s1, s2, s3;

    incoming >> s1 >> s2 >> s3;

    assert(s1 == PART1);
    assert(s2 == PART2);
    assert(s3 == PART3);

    ZmqMessage::Outgoing<ZmqMessage::SimpleRouting> outgoing(s, 0);

    outgoing << ZmqMessage::NullMessage << ZmqMessage::Flush;
  }
  return 0;
}

int
main(int, char**)
{
  pthread_t raw_receiver_tid;

  zmq::socket_t raw_sender_s(ctx, ZMQ_REQ);
  raw_sender_s.bind(endpoint_raw);

  clock_t start, finish;
  double elapsed;

  std::cout << "Testing raw..." << std::endl;
  start = clock();

  pthread_create(&raw_receiver_tid, 0, raw_receiver, const_cast<char*>(endpoint_raw));
  raw_sender(raw_sender_s);

  pthread_join(raw_receiver_tid, 0);

  finish = clock();
  elapsed = static_cast<double>(finish - start)/CLOCKS_PER_SEC;

  std::cout << "raw: elapsed: " << elapsed << std::endl;

  //--------------------------------------------------

  pthread_t multipart_receiver_tid;

  zmq::socket_t multipart_sender_s(ctx, ZMQ_REQ);
  multipart_sender_s.bind(endpoint_mes);

  std::cout << "Testing multipart..." << std::endl;
  start = clock();

  pthread_create(&multipart_receiver_tid, 0, multipart_receiver, const_cast<char*>(endpoint_mes));
  multipart_sender(multipart_sender_s);

  pthread_join(multipart_receiver_tid, 0);

  finish = clock();
  elapsed = static_cast<double>(finish - start)/CLOCKS_PER_SEC;

  std::cout << "multipart: elapsed: " << elapsed << std::endl;

}
