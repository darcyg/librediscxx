/** @file
* @brief a blocking tcp client with timeout
* @author yafei.zhang@langtaojin.com
* @date
* @version
* inner header
*/
#ifndef _LANGTAOJIN_LIBREDIS_BLOCKING_TCP_CLIENT_H_
#define _LANGTAOJIN_LIBREDIS_BLOCKING_TCP_CLIENT_H_

#include "redis_common.h"
#include <boost/system/system_error.hpp>

NAMESPACE_BEGIN

//single thread safety
class TcpClient
{
private:
  class Impl;
  Impl * impl_;

public:
  TcpClient();
  ~TcpClient();


  //NOTICE: connect, write, read, read_line
  void connect(
    const std::string& ip_or_host,
    const std::string& port_or_service,
    boost::posix_time::time_duration timeout,
    boost::system::error_code& ec);


  void write(
    const std::string& line,
    boost::posix_time::time_duration timeout,
    boost::system::error_code& ec);


  std::string read(
    size_t size,
    const std::string& delim,
    boost::posix_time::time_duration timeout,
    boost::system::error_code& ec);


  std::string read_line(
    const std::string& delim,
    boost::posix_time::time_duration timeout,
    boost::system::error_code& ec);


  void close();


  bool is_open()const;


  bool available()const;
};

NAMESPACE_END

#endif //_LANGTAOJIN_LIBREDIS_BLOCKING_TCP_CLIENT_H_
