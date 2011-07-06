/**
 * @file zsort.hpp
 * Sorting strings in separate thread.
 */
#include "pthread.h"

#include "ZmqMessage.hpp"

//0mq context is globally available
zmq::context_t ctx(1);

// endpoint to pass data from main thread to worker and back
const char* endpoint = "inproc://transport";

// endpoint to publish stop signal
const char* stop_endpoint = "inproc://stop";

const char* to_worker_fields[] = {"message_type", "request_identifier"};
const char* from_worker_fields[] = {"message_type", "response_identifier"};

uint64_t one_message_queue = 1;

// worker runs in a dedicated thread
void*
worker(void*)
{
  try
  {
    zmq::socket_t s(ctx, ZMQ_REP);
    s.setsockopt(ZMQ_HWM, &one_message_queue, sizeof(uint64_t));
    
    s.connect(endpoint);
    // socket to receive data from main thread and to send result back
    //   is connected

    zmq::socket_t ss(ctx, ZMQ_SUB);
    ss.setsockopt (ZMQ_SUBSCRIBE, "", 0);
    ss.connect(stop_endpoint);
    // socket to receive stop notifications from main thread and to send result back
    //   is connected

    zmq_pollitem_t item[2];

    item[0].socket = ss;
    item[0].events = ZMQ_POLLIN;
    
    item[1].socket = s;
    item[1].events = ZMQ_POLLIN | ZMQ_POLLOUT;

    std::auto_ptr<ZmqMessage::Multipart> queue;

    for(;;)
    {
      zmq::poll(item, sizeof(item) / sizeof(item[0]), 0);
      
      if (item[0].revents) // stop
      {
        std::cout << "stop" << std::endl;
        break;
      }
      else if (item[1].revents & ZMQ_POLLOUT)
      {
        std::cout << "POLLOUT" << std::endl;
        send(s, *queue.get(), true);
        queue.reset(0);
      }
      else if (item[1].revents & ZMQ_POLLIN)
      {
        std::cout << "POLLIN" << std::endl;
        ZmqMessage::Incoming<ZmqMessage::SimpleRouting> ingress(s);
        ingress.receive(2, to_worker_fields, 2, true);

        ZmqMessage::Outgoing<ZmqMessage::SimpleRouting> egress(
          ZmqMessage::OutOptions(
            s, ZmqMessage::OutOptions::CACHE_ON_BLOCK), ingress);
        
        egress << "response" << ingress[1] << ZmqMessage::Flush();

        if (egress.is_queued())
        {
          assert(!queue.get());
          std::cout << "is_queued" << std::endl;
          queue.reset(egress.detach());
        }
      }
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

  zmq::socket_t s(ctx, ZMQ_XREQ);
  s.setsockopt(ZMQ_HWM, &one_message_queue, sizeof(uint64_t));
  s.bind(endpoint);
  // socket to talk to worker is bound

  zmq::socket_t ss(ctx, ZMQ_PUB);
  ss.bind(stop_endpoint);
  // socket to talk to worker is bound

  try
  {
    // start worker
    pthread_create(&worker_tid, 0, worker, 0);

    ZmqMessage::Outgoing<ZmqMessage::XRouting> to_worker0(s, 0);
    to_worker0 << "request" << "#0" << ZmqMessage::Flush();
    
    sleep(1);
    
    ZmqMessage::Outgoing<ZmqMessage::XRouting> to_worker1(s, 0);
    to_worker1 << "request" << "#1" << ZmqMessage::Flush();
    
    ZmqMessage::Outgoing<ZmqMessage::XRouting> to_worker2(s, 0);
    // to_worker2 << "request" << "#2" << ZmqMessage::Flush;

    sleep(1);
    
    std::string msg_type;
    std::string msg_id;
    
    ZmqMessage::Incoming<ZmqMessage::XRouting> from_worker0(s);
    from_worker0.receive(2, from_worker_fields, 2, true);
    
    from_worker0 >> msg_type >> msg_id;
    std::cout << msg_type << msg_id << "received by main thread" << std::endl;
    
    ZmqMessage::Incoming<ZmqMessage::XRouting> from_worker1(s);
    from_worker1.receive(2, from_worker_fields, 2, true);
    
    from_worker1 >> msg_type >> msg_id;
    std::cout << msg_type << msg_id << "received by main thread" << std::endl;

    ZmqMessage::Outgoing<ZmqMessage::SimpleRouting> to_stop(ss, 0);
    to_stop << "stop" << ZmqMessage::Flush();
  }
    catch(const std::exception& ex)
  {
    std::cout << "caught (main): " << ex.what();
    exit(3);
  }

  pthread_join(worker_tid, 0);
  // thread is completed and sockets are closed
}

  
      
      
      

