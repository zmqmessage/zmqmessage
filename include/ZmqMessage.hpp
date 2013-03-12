/**
 * @file ZmqMessage.hpp
 * @copyright Copyright (c) 2010-2011 Phorm, Inc.
 * @copyright GNU LGPL v 3.0, see http://www.gnu.org/licenses/lgpl-3.0-standalone.html
 * @author Andrey Skryabin <andrew@zmqmessage.org>, et al.
 *
 */

#include <cstddef>
#include <zmq.hpp>

#include <ZmqMessageFwd.hpp>

#include <ZmqTools.hpp>

#include <zmqmessage/exceptions.hpp>
#include <zmqmessage/send.hpp>
#include <zmqmessage/Multipart.hpp>
#include <zmqmessage/PartsStorage.hpp>
#include <zmqmessage/MultipartContainer.hpp>
#include <zmqmessage/Incoming.hpp>
#include <zmqmessage/Manip.hpp>
#include <zmqmessage/OutOptions.hpp>
#include <zmqmessage/Sink.hpp>
#include <zmqmessage/Outgoing.hpp>

#ifndef ZMQMESSAGE_HPP_
#define ZMQMESSAGE_HPP_

/**
 * @namespace ZmqMessage
 * @brief Main namespace for ZmqMessage library
 */
namespace ZmqMessage
{
}

#endif /* ZMQMESSAGE_HPP_ */

#include "zmqmessage/ZmqMessageTemplateImpl.hpp"
