/**
 * @file zasync.cpp
 * @copyright Copyright (c) 2010-2011 Phorm, Inc.
 * @copyright GNU LGPL v 3.0, see http://www.gnu.org/licenses/lgpl-3.0-standalone.html 
 * @author Andrey Skryabin <andrew@zmqmessage.org>, et al.
 *
 * Demonstrating ZmqMessage queueing functionality for delayed sending of messages
 * to implement asynchronous worker thread.
 * (Note that currently queueing has proven working only for PUSH-PULL sockets.)
 */
#include "pthread.h"
#include "stdint.h"

#include <vector>
#include "ZmqMessage.hpp"

/**
\example zasync.cpp

This example emulates processing of asynchronous tasks in a separate thread.
Tasks and their results are received and sent via ZMQ.
HWM (high watermark) along with queueing and delayed sending
is used to avoid congestion of execution thread, blocking and losing any of processing results.

Characteristics of task processing:
<ul>
<li>Tasks are composed of random number of 'steps'.</li>
<li>All tasks are processed concurrently, step-by-step
(actually we decrement step counter each second when thread is idle).</li>
</ul>
In real life tasks would be execution of a series of remote requests, launching processes,
IO operations or whatever.

Execution thread behavior is following:
<ul>
<li>We have a separate results queue for delayed execution. When a task has been processed
and we failed to send result with no blocking (ZMQ internal queue is full) the message is put to delayed queue.
</li>
<li>We are accepting new tasks for execution ONLY when our results queue is empty
(so task sender thread is able to detect clobbering if it's not able to send request with no blocking).
</li>
<li>We are polling result channel for writing if queue is not empty (to send delayed messages when possible).
</li>
</ul>
 */

//0mq context is globally available
zmq::context_t ctx(1);

// endpoints to pass data from main thread to worker and back
const char* req_endpoint = "inproc://req_ep";
const char* res_endpoint = "inproc://res_ep";

// endpoint to publish stop signal, we shouldn't miss it!
const char* stop_endpoint = "inproc://stop";

const char* to_worker_fields[] = {"message_type", "task_identifier"};
const char* from_worker_fields[] = {"message_type", "task_identifier"};

uint64_t message_queue_limit = 5;
int max_task_steps = 10;

//we emulate asynchronous tasks by decrementing counter on each poll timeout event
struct async_task
{
  int id;
  int remaining_steps;
};

typedef std::vector<async_task> TaskVec;

void task_step(async_task& task)
{
  --task.remaining_steps;
}

template <typename QueueInserter>
void run_tasks(TaskVec& tasks, zmq::socket_t& s_res, QueueInserter q)
{
  std::for_each(tasks.begin(), tasks.end(), &task_step);
  for (TaskVec::iterator it = tasks.begin(); it != tasks.end(); )
  {
    async_task& task = *it;
    if (task.remaining_steps == 0)
    {
      std::cout << " task " << task.id << " done" << std::endl;
      ZmqMessage::Outgoing<ZmqMessage::SimpleRouting> egress(
        ZmqMessage::OutOptions(
          s_res, ZmqMessage::OutOptions::CACHE_ON_BLOCK |
          ZmqMessage::OutOptions::NONBLOCK));

      egress << "finished" << task.id << ZmqMessage::Flush;

      if (egress.is_queued())
      {
        std::cout << " is_queued" << std::endl;
        *q = egress.detach();
      }
      it = tasks.erase(it);
    }
    else
    {
      ++it;
    }
  }
}

// async task processor, runs in a dedicated thread
//Receives 'tasks' designated by id. Each task will take random (1-max_task_steps) steps to complete.
//so 'finished' responses will come in arbitrary order
void*
async_task_processor(void*)
{
  try
  {
    zmq::socket_t s_req(ctx, ZMQ_PULL);
    zmq::socket_t s_res(ctx, ZMQ_PUSH);
    uint64_t one_lim = 1;
#ifdef ZMQ_HWM
    s_req.setsockopt(ZMQ_HWM, &one_lim, sizeof(uint64_t));
#else
    s_req.setsockopt(ZMQ_SNDHWM, &one_lim, sizeof(uint64_t));
    s_req.setsockopt(ZMQ_RCVHWM, &one_lim, sizeof(uint64_t));
#endif
#ifdef ZMQ_HWM
    s_res.setsockopt(ZMQ_HWM, &one_lim, sizeof(uint64_t));
#else
    s_res.setsockopt(ZMQ_SNDHWM, &one_lim, sizeof(uint64_t));
    s_res.setsockopt(ZMQ_RCVHWM, &one_lim, sizeof(uint64_t));
#endif

    s_req.connect(req_endpoint);
    s_res.connect(res_endpoint);

    zmq::socket_t ss(ctx, ZMQ_SUB);
    ss.setsockopt (ZMQ_SUBSCRIBE, "", 0);
    ss.connect(stop_endpoint);
    // socket to receive stop notifications from main thread and to send result back
    //   is connected

    zmq_pollitem_t item[3]; //stop, in, out

    item[0].socket = ss;
    item[0].events = ZMQ_POLLIN;

    item[1].socket = s_req;
    item[1].events = ZMQ_POLLIN;

    item[2].socket = s_res;
    item[2].events = ZMQ_POLLOUT;

    std::vector<ZmqMessage::Multipart*> queue;

    TaskVec tasks;

    for(;;)
    {
      //have something to send
      item[2].events = (queue.empty()) ? 0 : ZMQ_POLLOUT;
      //do not receive more tasks until we send pending responses
      item[1].events = (queue.empty()) ? ZMQ_POLLIN : 0;

      int res = zmq::poll(item, sizeof(item) / sizeof(item[0]), 1000000); //1s

      if (res == 0)
      {
        //timeout
        std::cout << "RUN TASKS: " << tasks.size() << std::endl;
        run_tasks(tasks, s_res, std::back_inserter(queue));
        continue;
      }
      if (item[0].revents) // stop
      {
        std::cout << " stop" << std::endl;
        break;
      }
      else if ((item[2].revents & ZMQ_POLLOUT) && (item[2].revents & ZMQ_POLLOUT))
      {
        std::cout << "POLLOUT, sending" << std::endl;
        std::auto_ptr<ZmqMessage::Multipart> m(queue.back()); //no throw
        queue.back() = 0; //no throw
        queue.pop_back();
        ZmqMessage::send(s_res, *m, true);
      }
      else if ((item[1].events & ZMQ_POLLIN) && (item[1].revents & ZMQ_POLLIN))
      {
        std::cout << "POLLIN, new task" << std::endl;
        ZmqMessage::Incoming<ZmqMessage::SimpleRouting> ingress(s_req);
        ingress.receive(2, to_worker_fields, 2, true);

        std::string tmp;
        async_task task;
        ingress >> tmp >> task.id;

        task.remaining_steps = (rand() % (max_task_steps - 1)) + 1; //1-max_task_steps

        tasks.push_back(task);
      }
    }
  }
  catch(const std::exception& ex)
  {
    std::cout << "caught (processor): " << ex.what();
    exit(3);
  }
  return 0;
}


int
main(int, char**)
{
  pthread_t worker_tid;
  pthread_t sender_tid;

  zmq::socket_t s_req(ctx, ZMQ_PUSH);
  zmq::socket_t s_res(ctx, ZMQ_PULL);
#ifdef ZMQ_HWM
  s_req.setsockopt(ZMQ_HWM, &message_queue_limit, sizeof(uint64_t));
#else
  s_req.setsockopt(ZMQ_SNDHWM, &message_queue_limit, sizeof(uint64_t));
  s_req.setsockopt(ZMQ_RCVHWM, &message_queue_limit, sizeof(uint64_t));
#endif
#ifdef ZMQ_HWM
  s_res.setsockopt(ZMQ_HWM, &message_queue_limit, sizeof(uint64_t));
#else
  s_res.setsockopt(ZMQ_SNDHWM, &message_queue_limit, sizeof(uint64_t));
  s_res.setsockopt(ZMQ_RCVHWM, &message_queue_limit, sizeof(uint64_t));
#endif
  

  s_req.bind(req_endpoint);
  s_res.bind(res_endpoint);

  zmq::socket_t ss(ctx, ZMQ_PUB);
  ss.bind(stop_endpoint);
  // socket to talk to worker is bound

  try
  {
    // start worker
    pthread_create(&worker_tid, 0, async_task_processor, 0);
    sleep(1);

    for (int i = 0; i < message_queue_limit+2; ++i)
    {
      ZmqMessage::Outgoing<ZmqMessage::SimpleRouting> to_worker(s_req, ZmqMessage::OutOptions::NONBLOCK);
      to_worker << "request" << i << ZmqMessage::Flush;
      usleep(100000);
    }

    std::cout << "1:requests sent: " << (message_queue_limit+2) <<
      ", sleeping " << max_task_steps << std::endl;
    sleep(max_task_steps + 1);

    //all tasks are processed, res queue is full, 1 remaining response is cached
    //so no new requests is accepted
    for (int i = message_queue_limit+2; i < 2*message_queue_limit+2; ++i)
    {
      ZmqMessage::Outgoing<ZmqMessage::SimpleRouting> to_worker(s_req, ZmqMessage::OutOptions::NONBLOCK);
      to_worker << "request" << i << ZmqMessage::Flush;
      usleep(100000);
    }
    std::cout << "2:requests sent: " << message_queue_limit << std::endl;

    //req queue filled
    ZmqMessage::Outgoing<ZmqMessage::SimpleRouting> to_worker(
      s_req, ZmqMessage::OutOptions::NONBLOCK | ZmqMessage::OutOptions::DROP_ON_BLOCK);
    to_worker << "request" << (2*message_queue_limit+2) << ZmqMessage::Flush;
    assert(to_worker.is_dropping());

    //read all
    std::cout << "reading..." << std::endl;

    std::string msg_type;
    std::string msg_id;

    for (int i = 0; i < 2*message_queue_limit; ++i)
    {
      ZmqMessage::Incoming<ZmqMessage::SimpleRouting> from_worker0(s_res);
      from_worker0.receive(2, from_worker_fields, 2, true);

      from_worker0 >> msg_type >> msg_id;
      std::cout << msg_type << msg_id << "received by main thread" << std::endl;
    }

    //stop

    ZmqMessage::Outgoing<ZmqMessage::SimpleRouting> to_stop(ss, 0);
    to_stop << "stop" << ZmqMessage::Flush;
  }
    catch(const std::exception& ex)
  {
    std::cout << "caught (main): " << ex.what();
    exit(3);
  }

  pthread_join(worker_tid, 0);
//  pthread_join(sender_tid, 0);
  // thread is completed and sockets are closed
}






