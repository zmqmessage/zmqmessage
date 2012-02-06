/**
 * @file zbisort.cpp
 * Complex ZmqMessage example demonstrating Text and Binary modes, iterators,
 * message relaying
 */
#include "pthread.h"
#include <vector>

#include <zmq.hpp>

#include "ZmqMessage.hpp"

#include "StringFace.hpp"

/**
\example zbisort.cpp

This is a little more complex example. Demonstrates Text and Binary modes,
iterators, message relaying.

Main thread starts a relay thread (@c relay(void*)).
It in turn starts two sorting workers. One worker is sorting incoming messages as integers
(@c sorter<int>(void*)) in numeric order, and the second one
(@c sorter<StringFace>(void*)) is sorting as strings (in alphanumeric order).
Relay thread is a dispatcher: it reads first message
("mode" message, it can be either "INT", "STRING" or "STOP")
and relays all tail message parts to appropriate worker.
Response from worker is relayed back to main thread.

Also we implemented simple class StringFace that conforms to string concept
(see \ref ZMQMESSAGE_STRING_CLASS for description) and just wraps external memory
to avoid intermediate copying while sorting.
*/

// 0mq context is globally available
zmq::context_t ctx(1);

// endpoint to pass data from main thread to relay and back
const char* relay_endpoint = "inproc://relay";
// endpoints to pass data from realy thread to string and numeric workers and back
const char* string_endpoint = "inproc://string";
const char* int_endpoint = "inproc://int";

const std::string int_mode = "INT";
const std::string string_mode = "STRING";
const std::string stop_mode = "STOP";

// worker run in a dedicated thread. Launched by relay thread.
template <typename T>
void*
sorter(void* endpoint)
{
  try
  {
    zmq::socket_t s(ctx, ZMQ_REP);
    s.connect(static_cast<char*>(endpoint));
    std::cout << "connected to " << static_cast<char*>(endpoint) << std::endl;
    // socket to receive data from main thread and to send result back
    //   is connected

    for(;;)
    {
      ZmqMessage::Incoming<ZmqMessage::SimpleRouting> ingress(s);
      ingress.receive_all();
      // all parts of incoming message are received

      if (ingress.size() == 1)
      {
        // treat one-part mesages as termination marker
        std::cout << "leaving ..." << std::endl;
        return 0;
      }

      std::vector<T> sort_area;

      std::copy(ingress.begin<T>(),
                ingress.end<T>(),
                std::back_inserter(sort_area));
      // every message part is a vector element now

      std::sort(sort_area.begin(), sort_area.end());
      // they are sorted according to type

      ZmqMessage::Outgoing<ZmqMessage::SimpleRouting> egress(s, 0);

      std::copy(sort_area.begin(),
                sort_area.end(),
                ZmqMessage::Outgoing<ZmqMessage::SimpleRouting>::iterator<T>(egress));
      // put to outgoing container

      egress.flush();
      // and sent to the main thread
    }
  }
  catch(const std::exception& ex)
  {
    std::cout << "caught (worker): " << ex.what();
    exit(3);
  }

  return 0;
}

void*
relay(void*)
{
  pthread_t string_tid;
  pthread_t int_tid;

  try
  {
    zmq::socket_t relay(ctx, ZMQ_REP);
    relay.connect(relay_endpoint);

    zmq::socket_t s(ctx, ZMQ_REQ);
    s.bind(string_endpoint);

    zmq::socket_t i(ctx, ZMQ_REQ);
    i.bind(int_endpoint);

    // start workers
    pthread_create(&string_tid, 0, sorter<StringFace>, const_cast<char*>(string_endpoint));
    pthread_create(&int_tid, 0, sorter<int>, const_cast<char*>(int_endpoint));

    for(;;)
    {
      ZmqMessage::Incoming<ZmqMessage::SimpleRouting> ingress(relay);
      ingress.receive(1, false);

      StringFace mode;
      ingress >> mode;

      if (mode == int_mode)
      {
        ZmqMessage::Outgoing<ZmqMessage::SimpleRouting> to_int(i, 0);
        to_int.relay_from(relay);
        // request is read
        to_int << 38 << ZmqMessage::Flush;
        // extra stuff added. sent to worker.

        ZmqMessage::relay_raw(i, relay, false);
        // answer from worker resent to main thread
      }
      else if (mode == string_mode)
      {
        ZmqMessage::Outgoing<ZmqMessage::SimpleRouting> to_string(s, 0);
        to_string.relay_from(relay);
        to_string << "some addition" << ZmqMessage::Flush;

        ZmqMessage::relay_raw(s, relay, false);
        // answer from worker resent to main thread
      }
      else if (mode == stop_mode)
      {
        ZmqMessage::Outgoing<ZmqMessage::SimpleRouting> to_stop_int(i, 0);
        to_stop_int << stop_mode << ZmqMessage::Flush;

        ZmqMessage::Outgoing<ZmqMessage::SimpleRouting> to_stop_string(s, 0);
        to_stop_string << stop_mode << ZmqMessage::Flush;

        break;
      }
      else
      {
        assert(!"Unknown mode");
      }
    }
  }
  catch(const std::exception& ex)
  {
    std::cout << "caught (relay): " << ex.what();
    exit(3);
  }

  pthread_join(string_tid, 0);
  pthread_join(int_tid, 0);
  // worker threads are completed and sockets are closed

  return 0;
}

int
main(int, char**)
{
  pthread_t relay_tid;

  try
  {
    zmq::socket_t s(ctx, ZMQ_REQ);
    s.bind(relay_endpoint);
    // socket to talk to worker is bound

    // start relay
    pthread_create(&relay_tid, 0, relay, 0);

    {
      std::vector<std::string> desc;

      // populate 'zzzzzzzzzz' ... 'aaaaaaaaaa' string
      for (char letter = 'z'; letter >= 'a'; --letter)
      {
        std::string part(10, letter);
        desc.push_back(part);
      }

      std::cout << "Original array:" << std::endl;
      std::copy(desc.begin(), desc.end(),
                std::ostream_iterator<std::string>(std::cout, "\n"));

      ZmqMessage::Outgoing<ZmqMessage::SimpleRouting> to_sort(s, 0);

      to_sort << string_mode;
      std::copy(desc.begin(), desc.end(),
                ZmqMessage::Outgoing<ZmqMessage::SimpleRouting>::iterator<std::string>(to_sort));
      // strings are in outgoing container

      to_sort.flush();
      // outgoing container content is sent

      ZmqMessage::Incoming<ZmqMessage::SimpleRouting> sorted(s);
      sorted.receive_all(0, 0);
      // worker response received

      std::vector<std::string> asc;

      std::copy(sorted.begin<std::string>(), sorted.end<std::string>(),
                std::back_inserter(asc));
      // copied to vector

      std::cout << "Sorted array:" << std::endl;
      std::copy(asc.begin(), asc.end(),
                std::ostream_iterator<std::string>(std::cout, "\n"));
      // and printed out to demonstracte it is in ascending order
    }

    {
      std::vector<int> desc;

      // populate 100 ... 90 numbers
      for (int number = 100; number >= 90; --number)
      {
        desc.push_back(number);
      }


      std::cout << "Original array:" << std::endl;
      std::copy(desc.begin(), desc.end(),
                std::ostream_iterator<int>(std::cout, "\n"));

      ZmqMessage::Outgoing<ZmqMessage::SimpleRouting> to_sort(s, 0);

      to_sort << int_mode;

      to_sort << 950;
      // to prove we do numeric sort

      std::copy(desc.begin(), desc.end(),
                ZmqMessage::Outgoing<ZmqMessage::SimpleRouting>::iterator<int>(to_sort));
      // numbers are in outgoing container

      to_sort.flush();
      // outgoing container content is sent

      ZmqMessage::Incoming<ZmqMessage::SimpleRouting> sorted(s);
      sorted.receive_all(0, 0);
      // worker response received

      std::vector<int> asc;

      std::copy(sorted.begin<int>(), sorted.end<int>(),
                std::back_inserter(asc));
      // copied to vector

      std::cout << "Sorted array:" << std::endl;
      std::copy(asc.begin(), asc.end(),
                std::ostream_iterator<int>(std::cout, "\n"));
      // and printed out to demonstracte it is in ascending order
    }


    ZmqMessage::Outgoing<ZmqMessage::SimpleRouting> to_stop(s, 0);
    to_stop << stop_mode;// << ZmqMessage::Flush;
    to_stop.flush();
    // send termination marker to worker
  }
  catch(const std::exception& ex)
  {
    std::cout << "caught (main): " << ex.what();
    exit(3);
  }

  pthread_join(relay_tid, 0);
  // relay thread is completed and sockets are closed
}
