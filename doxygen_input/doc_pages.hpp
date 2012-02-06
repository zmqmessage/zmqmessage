/** \mainpage ZmqMessage C++ Library
Sending and receiving <a href="http://www.zeromq.org">ZeroMQ</a> \ref zm_multipart "multipart" messages.

<h3>Main features:</h3>
<ul>
<li>Transparent interoperability between "simple" and "X" ZMQ endpoint types
due to routing policies</li>
<li>Checking for multipart message consistency when receiving
(if number of parts to be received is known in advance)
<li>Using library as built-in (more configurable) or linking with shared library
(see \ref ref_linking_options "linking options" for details)</li>
<li>Configurable to use application-specific logging and exception policies
</li>
<li>Possibility to use user-supplied string-like classes to avoid copying</li>
<li>Supporting iterators, insert (<<) operator for outgoing messages and
extract (>>) operator for incoming messages</li>
<li>Text (default) and binary \ref zm_modes "modes" for extraction/insertion/iteration of parts</li>
<li>Possibility for \ref zm_queueing "queueing messages" for delayed sending if
either blocking or dropping is unacceptable</li>
</ul>

<div class="zm_toc">
<h3>Table of contents</h3>
<ul>
  <li>\ref zm_multipart "What multipart messages are and why we need them"</li>
  <li>\ref zm_tutorial "Tutorial"</li>
  <li>Reference</li>
    <dd>
    - Class ZmqMessage::Incoming for receiving incoming messages
    - Class ZmqMessage::Outgoing for sending outgoing messages
    - \ref ZmqMessage "All ZmqMessage namespace members",
    including functions for convenient working with message parts \n
    </dd>
  </li>
  <li>Advanced aspects</li>
    <dd>
    - \ref zm_modes "Text and binary modes"
    - \ref zm_queueing "Queueing messages for delayed sending"
    </dd>
  </li>
  <li><a class="el" href="examples.html">Examples</a></li>
  <li><a class="el" href="test.html">Tests list</a></li>
  <li>\ref zm_build "Build instructions"</li>
  <li>\ref zm_performance "Performance notes"</li>
  <li><a class="el" href="https://github.com/zmqmessage/zmqmessage/">GitHub project download page</a></li>
</ul>
</div>
 */

/** \page zm_multipart
<h2>What multipart messages are and why we need them</h2>

ZMQ documentation says on multipart messages the following:

\htmlonly
<div class="zm_boxed">
<p>A &Oslash;MQ message is composed of 1 or more message parts; each message part is an independent <em>zmq_msg_t</em> in its own right. &Oslash;MQ ensures atomic delivery of messages; peers shall receive either all <em>message parts</em> of a message or none at all.</p>
<p>The total number of message parts is unlimited.</p>
<p>An application wishing to send a multi-part message does so by specifying the <em>ZMQ_SNDMORE</em> flag to <em>zmq_send()</em>. The presence of this flag indicates to &Oslash;MQ that the message being sent is a multi-part message and that more message parts are to follow. When the application wishes to send the final message part it does so by calling <em>zmq_send()</em> without the <em>ZMQ_SNDMORE</em> flag; this indicates that no more message parts are to follow.</p>
</div>
\endhtmlonly

Thus multipart messages may be used to implement custom text/binary protocols of arbitrary complexity based on ZeroMQ.

The goal of ZmqMessage library is to make working with multipart messages as convenient as possible.
*/

/** \page zm_queueing
<h2>Queueing messages for delayed sending</h2>

This feature is useful when:
<ul>
<li>sending messages may block
(ex. <a href="http://api.zeromq.org/2-1-1:zmq-setsockopt">ZMQ_HWM</a> is set and has been reached)
</li>
<li>we cannot block on sending message (it would make our thread unresponsive to other events).
</li>
<li>we cannot drop the message, cause it MUST be delivered when possible.
</li>
</ul>

In this case we should create outgoing messages with ZmqMessage::OutOptions::NONBLOCK and ZmqMessage::OutOptions::CACHE_ON_BLOCK.
If sending fails, we may 'detach' composed message and resend it later, when output socket will be available for writing.

\code
//delayed queue
std::vector<Zmqmessage::Multipart*> queue;

zmq::socket_t s_req(ctx, ZMQ_PUSH);

//set HWM
uint64_t lim = 10;
s_req.setsockopt(ZMQ_HWM, &lim, sizeof(uint64_t));

s_req.connect("inproc://test-ep");

ZmqMessage::OutOptions opts(s_req,
  ZmqMessage::OutOptions::CACHE_ON_BLOCK | ZmqMessage::OutOptions::NONBLOCK);

ZmqMessage::Outgoing<ZmqMessage::SimpleRouting> egress(opts);

egress << "finished" << 888 << ZmqMessage::Flush;

if (egress.is_queued())
{
  queue.push_back(egress.detach());
}
\endcode

See example \ref zasync.cpp "zasync" for details.

Note, that currently queueing functionality has proven working only for PUSH-PULL sockets.
Request-reply patterns (REQ, RES, XREQ, XRES sockets) in many cases
prevent congestion due to internal socket states.

 */

/** \page zm_build
<h2>Build instructions</h2>

After you have downloaded an archive or cloned a git repository, you may
build the shared library, examples and tests.

<h3>Build Requirements</h3>
<ul>
<li>Library is built with <a href="http://cmake.org/">CMake</a> cross-platform build tool, so you need it installed.
<li>You need <a href="http://zeromq.org">ZeroMQ</a> library in order to compile ZmqMessage library.
</li>
</ul>

<h3>Runtime requirements</h3>
<ul>
<li>You need <a href="http://zeromq.org">ZeroMQ</a> library at runtime too.
</li>
</ul>

<h3>Build Steps</h3>
Go to zmqmessage directory and do following:
\code
$ mkdir build
$ cd build
//configure CMAKE: This command generates makefiles.
$ cmake -DCMAKE_INSTALL_PREFIX=/path/to/install ..
//or just "cmake ..". /path/to/install by default is /usr/local
//or type "ccmake .." for more control, or when zeromq library is installed in non-standard places

//then just make and install:
$ make
$ make install
//or "sudo make install" depending on permissions
\endcode

That's all.

If you wish to build shared library with custom exceptions
(\ref ZMQMESSAGE_EXCEPTION_MACRO) or with \ref ZMQMESSAGE_WRAP_ZMQ_ERROR defined,
you may create .hpp file with definitions of these exceptions.
\code
//@file mydefs.hpp

#define ZMQMESSAGE_WRAP_ZMQ_ERROR

#define ZMQMESSAGE_EXCEPTION_MACRO(name) \
  class name \
  { \
  .... \
  }

\endcode

On "cmake" step you can force include the file:
\code
$ cmake -DZMQMESSAGE_CONFIGURATION_HEADER=/path/to/mydefs.hpp ..
\endcode

Note, that building shared library is not really necessary,
as ZmqMessage library may be compiled in your binary.
See \ref ref_linking_options "linking options" section in tutorial for details.
 */

/** \page zm_performance
<h2>Performance notes</h2>
<hr>
Nothing comes for free, and our library introduces some overhead
over plain zeromq messaging interface.

To measure this overhead we have written \ref tests/PerfTest.cpp "performance test".
We make 100000 request-response transactions between 2 threads and print results.

First of all, we need to say that this "application" does nothing
but sending and receiving multipart messages, so it should be considered highly synthetic,
and in real apps performance costs probably will be unnoticeable.

We make 10 contiguous test runs and we get <b>20%</b> of average overhead.

 */

/** \page zm_modes
<h2>Word on Text and Binary modes</h2>
<hr>
Text and Binary modes determine how raw ZMQ message content is converted into user type,
when message content is extracted from Incoming, inserted into Outgoing,
or we iterate over incoming multipart message.

<b>Binary:</b> message content pointer is interpreted as pointer to user type,
and assign operator is invoked. This mode is suitable for implementing binary protocols.

<b>Text:</b> message content is interpreted as char array.
For string types (see \ref ZMQMESSAGE_STRING_CLASS on string concept definition)
we initialize object from char array, for other types we put (>>) chars into @c stringstream
and read stream into instance of type.

<h3>How to set:</h3>
<ul>
<li>For Incoming:
Use Text and Binary manipulators to switch stream to/from binary mode. Default mode is text.
\code
//read text command and binary integer as value.
std::string command;
int value;
incoming >> ZmqMessage::Text >> command << ZmqMessage::Binary << value;
\endcode
</li>
<li>For iteration over incoming:
Call Incoming::begin() method:
\code
std::ostream_iterator<int> out_it(std::cout, ", ");

//print messages with binary integers.
std::copy(
  incoming.begin<int>(true), incoming.end<int>(), out_it);
\endcode
</li>
<li>For insertion into outgoing:
You may initialize Outgoing with OutOptions::BINARY_MODE to initially set binary mode,
otherwise text mode is set.
\code
ZmqMessage::Outgoing<ZmqMessage::XRouting> outgoing(
  sock, ZmqMessage::OutOptions::NONBLOCK | ZmqMessage::OutOptions::BINARY_MODE);

//then use Text and Binary manipulators:
long id = 1;

outgoing << id //insert binary
  << ZmqMessage::Text //switch to text
  << "SET_VALUE"
  << ZmqMessage::Binary << 999; //again binary

\endcode
</li>
<li>For all:
When you need a particular type to be ALWAYS sent/received/iterated in either binary or text form
(independent on current stream state), you may use any of two ways for it.

1. Declare your 'binary' types with @c raw_mark field.
\code
struct SomeBinaryData
{
  typedef void raw_mark;

  int f1;
  char f2[20];
};
\endcode

2. Declare particular types as 'binary' using \ref ZMQMESSAGE_BINARY_TYPE
or as text using \ref ZMQMESSAGE_TEXT_TYPE macros.
\code

struct OtherBinaryData
{
  int aa;
  char[100] data;
};
ZMQMESSAGE_BINARY_TYPE(OtherBinaryData);

//always send/receive 'long' as text.
ZMQMESSAGE_TEXT_TYPE(long);
\endcode
Actually, these macros create a specialization
of traits type ZmqMessage::Private::IsRaw for the type specified.

See example zserialize.cpp for details.
</ul>
 */

/** \page zm_tutorial

<h2>Tutorial</h2>
<div class="zm_toc">
<ul>
  <li>\ref ref_configuring "Configuring library to integarte smoothly into your application"</li>
  <li>\ref ref_linking_options "Linking options"</li>
  <li>\ref ref_receiving "Receiving messages"</li>
  <li>\ref ref_sending "Sending messages"</li>
</ul>
</div>

<hr>

\anchor ref_configuring
<h3>Configuring library</h3>
To integrate the library into your application you can (and encouraged to)
define a few macro constants before including library headers
(ZmqMessage.hpp and ZmqTools.hpp) anywhere in your application.
ZmqMessageFwd.hpp may be included wherever (i.e. before the definitions).
Though none of these definitions are mandatory.

These constants are:

<ul>

<li>
\code
ZMQMESSAGE_STRING_CLASS
\endcode
@copydoc ZMQMESSAGE_STRING_CLASS
</li>

<li>
\code
ZMQMESSAGE_LOG_STREAM
\endcode
@copydoc ZMQMESSAGE_LOG_STREAM
</li>

<li>
\code
ZMQMESSAGE_LOG_TERM
\endcode
@copydoc ZMQMESSAGE_LOG_TERM
</li>

<li>
\code
ZMQMESSAGE_EXCEPTION_MACRO
\endcode
@copydoc ZMQMESSAGE_EXCEPTION_MACRO
</li>

<li>
\code
ZMQMESSAGE_WRAP_ZMQ_ERROR
\endcode
@copydoc ZMQMESSAGE_WRAP_ZMQ_ERROR
</li>

</ul>

\anchor ref_linking_options
<h3>Linking options</h3>

<ol>
<li>
You may build ZmqMessage as shared library (see \ref zm_build "build instructions")
and link against it. But if you need extensive configuration, the following problem may arise:
library is built separately, with definite and basically default configuration
(\ref ZMQMESSAGE_LOG_STREAM, \ref ZMQMESSAGE_EXCEPTION_MACRO, \ref ZMQMESSAGE_WRAP_ZMQ_ERROR),
So if you need to override them in your application,
you really need to \ref zm_build "rebuild shared library" with appropriate definitions
(the same as you define in your application), otherwise you can get the link error as following:
\code
In function `__static_initialization_and_destruction_0':
/home/andrey_skryabin/projects/zmqmessage/include/zmqmessage/TypeCheck.hpp:32: undefined reference to `ZmqMessage::Private::TypeCheck<ZmqMessage::MessageFormatError, ZmqMessage::NoSuchPartError, zmq::error_t>::value'
collect2: ld returned 1 exit status
\endcode
That's because we check that generated types are the same.
Link with shared library if you don't need extensive configuration or if it's not too cumbersome
for you to rebuild shared library with the same configuration.
</li>
<li>
Do not link against shared library. In this case you MUST include implementation
code in ONE of your .cpp files in order to assemble you binary:
\code

//@file foo.cpp

//make definitions available for linking
#include "ZmqMessageImpl.hpp"

...

\endcode
Do not include "ZmqMessageImpl.hpp" on more than one .cpp file,
or you will get multiple definitions and thus link error.

Such approach is better than header-only, cause it reduces compilation time.

</li></ol>

\anchor ref_receiving
<h3>Receiving messages</h3>
To receive multipart message, you create instance of ZmqMessage::Incoming.
For XREQ and XREP socket types use \ref ZmqMessage::XRouting as template parameter,
for other socket types use \ref ZmqMessage::SimpleRouting.

\code
#include "ZmqMessage.hpp"

zmq::context_t ctx(1);
zmq::socket sock(ctx, ZMQ_PULL); //will use SimpleRouting
sock.connect("inproc://some-endpoint");

ZmqMessage::Incoming<ZmqMessage::SimpleRouting> incoming(sock);

//say, we know what message parts we receive. Here are their names:
const char* req_parts[] = {"id", "name", "blob"};

//true because it's a terminal message, no more parts allowed at end
incoming.receive(3, req_parts, true);

//Get 2nd part explicitly (assume ZMQMESSAGE_STRING_CLASS is std::string):
std::string name = ZmqMessage::get_string(incoming[1]);
//or more verbose:
std::string name_cpy = ZmqMessage::get<std::string>(incoming[1]);

//if we also have some MyString class:
MyString my_id = ZmqMessage::get<MyString>(incoming[0]);

//or we can extract data as variables or plain zmq messages.
zmq::message_t blob;
incoming >> my_id >> name >> blob;

//or we can iterate on message parts and use standard algorithms:
std::ostream_iterator<MyString> out_it(std::cout, ", ");
std::copy(
  incoming.begin<MyString>(), incoming.end<MyString>(), out_it);

\endcode

Of course, passing message part names is not necessary (see \ref ZmqMessage::Incoming::receive "receive" functions).

There are cases when implemented protocol implies a fixed number of message parts at the beginning of multipart message.
And the number of subsequent messages is either undefined or estimated based on contents of first parts
(i.e. first part may contain command name, and other parts may contain data dependent on command).
So you first may call \ref ZmqMessage::Incoming::receive "receive" function with @c false as last parameter
(not checking if message is terminal), do something with first parts,
and then call \ref ZmqMessage::Incoming::receive "receive" or
\ref ZmqMessage::Incoming::receive_all "receive_all" again to fetch the rest.

\code
const char* req_parts[] = {"command"};
incoming.receive(1, req_parts, false);

std::string cmd = ZmqMessage::get_string(incoming[0]);

if (cmd == "SET_PARAM")
{
  const char* tail_parts[] = {"parameter 1 value (text)", "parameter 2 value (binary)"};
  incoming.receive(2, tail_parts, true);

  //message with parameter 1 contains text data converted into unsigned 32 bit integer (ex. "678" -> 678)
  uint32_t param1 = ZmqMessage::get<uint32_t>(incoming[1]);

  //message with parameter 2 contains binary data (unsigned 32 bit integer)
  uint32_t param2 = ZmqMessage::get_bin<uint32_t>(incoming[2]);

  //...
}
else
{
  //otherwise we receive all remaining message parts
  incoming.receive_all();

  if (incoming.size() > 1)
  {
    std::string first;
    incoming >> ZmqMessage::Skip //command is already analyzed
      >> first; // same as first = ZmqMessage::get<std::string>(incoming[1]);
    //...
  }
}

\endcode

\anchor ref_sending
<h3>Sending messages</h3>

To send a multipart message, you create instance of ZmqMessage::Outgoing.
For sending to XREQ and XREP socket types use \ref ZmqMessage::XRouting as template parameter,
for other socket types use \ref ZmqMessage::SimpleRouting.

\code
#include "ZmqMessage.hpp"

zmq::context_t ctx(1);
zmq::socket sock(ctx, ZMQ_XREQ); //will use XRouting
sock.connect("inproc://some-endpoint");

//create Outgoing specifying sending options: use nonblocking send
//and drop messages if operation would block
ZmqMessage::Outgoing<ZmqMessage::XRouting> outgoing(
  sock, ZmqMessage::OutOptions::NONBLOCK | ZmqMessage::OutOptions::DROP_ON_BLOCK);

//suppose we have some MyString class:
MyString id("112233");

outgoing << id << "SET_VARIABLES";

//Number will be converted to string (written to stream), cause Outgoing is in Text mode.
outgoing << 567099;

char buffer[128];
::memset (buffer, 'z', 128); //fill buffer

outgoing << ZmqMessage::RawMessage(buffer, 128);

//send message with binary number and flush it;
int num = 9988;
outgoing << ZmqMessage::Binary << num << ZmqMessage::Flush;
\endcode

Flushing Outgoing message sends final (terminal) message part (which was inserted before flushing),
and no more insertions allowed after it.
If you do not flush Outgoing message manually, it will flush in destructor.

*/

