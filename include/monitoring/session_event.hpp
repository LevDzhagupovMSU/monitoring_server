#pragma once


#include <boost/beast/core/error.hpp>


namespace monitoring{

enum class SessionEventType {
      handshake_error,
      disconnected,
      read_error,
      write_error,
      close_error
  };


struct SessionEvent{
    SessionEventType type;
    boost::beast::error_code ec;
};

}


