/**
 * @file zsort.hpp
 * Sorting strings in separate thread.
 */
#include <pthread.h>
#include <vector>

#include <zmq.hpp>

#include "ZmqMessage.hpp"

/**
\example zsort.cpp

This example launches a separate thread which receives multipart message
with arbitrary number of strings to sort. The thread sorts it and sends back
the outgoing multipart message. Main thread receives sorted messages.
*/

//0mq context is globally available
zmq::context_t ctx(1);

// endpoint to pass data from main thread to worker and back
const char* endpoint = "inproc://transport";


// worker runs in a dedicated thread
void*
sorter(void*)
{
  try
  {
    zmq::socket_t s(ctx, ZMQ_REP);
    s.connect(endpoint);
    // socket to receive data from main thread and to send result back
    //   is connected

    for(;;)
    {
      ZmqMessage::Incoming<ZmqMessage::SimpleRouting> ingress(s);
      ingress.receive_all(0, 0);
      // all parts of incoming message are received

      if (ingress.size() == 1)
      {
        // treat one-part mesages as termination marker
        std::cout << "leaving ..." << std::endl;
        return 0;
      }

      std::vector<std::string> sort_area;

      std::copy(ingress.begin<std::string>(),
                ingress.end<std::string>(),
                std::back_inserter(sort_area));
      // every message part is a vector element now

      std::sort(sort_area.begin(), sort_area.end());
      // they are semi-lexicography sorted

      ZmqMessage::Outgoing<ZmqMessage::SimpleRouting> egress(s, 0);

      std::copy(sort_area.begin(),
                sort_area.end(),
                ZmqMessage::Outgoing<ZmqMessage::SimpleRouting>::iterator<std::string>(egress));
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

int
main(int, char**)
{
  pthread_t worker_tid;

  zmq::socket_t s(ctx, ZMQ_REQ);
  s.bind(endpoint);
  // socket to talk to worker is bound

  try
  {
    // start worker
    pthread_create(&worker_tid, 0, sorter, 0);

    std::vector<std::string> desc;

    // populate 'zzzzzzzzzz' ... 'aaaaaaaaaa' string
    for (char letter = 'z'; letter >= 'a'; --letter)
    {
      std::string part(10, letter);
      desc.push_back(part);
    }

    std::cout << "Original array:" << std::endl;
    std::copy(desc.begin(),
              desc.end(),
              std::ostream_iterator<std::string>(std::cout, "\n"));

    ZmqMessage::Outgoing<ZmqMessage::SimpleRouting> to_sort(s, 0);

    //     we can use std::copy algorithm as well
    for (size_t i = 0; i < desc.size(); ++i)
    {
      to_sort << desc[i];
    }
    // strings are in outgoing container

    to_sort << "an arbitrary string" << 123;
    // extra string and number added to outgoing container

    to_sort.flush();
    // outgoing container content is sent

    ZmqMessage::Incoming<ZmqMessage::SimpleRouting> sorted(s);
    sorted.receive_all(0, 0);
    // worker response received

    std::vector<std::string> asc;

    std::copy(sorted.begin<std::string>(),
              sorted.end<std::string>(),
              std::back_inserter(asc));
    // copied to vector

    std::cout << "Sorted array:" << std::endl;
    std::copy(asc.begin(),
              asc.end(),
              std::ostream_iterator<std::string>(std::cout, "\n"));
    // and printed out to demonstracte it is in ascending order


    ZmqMessage::Outgoing<ZmqMessage::SimpleRouting> to_stop(s, 0);
    to_stop << "stop" << ZmqMessage::Flush;
    // send termination marker to worker
  }
  catch(const std::exception& ex)
  {
    std::cout << "caught (main): " << ex.what();
    exit(3);
  }

  pthread_join(worker_tid, 0);
  // thread is completed and sockets are closed
}

