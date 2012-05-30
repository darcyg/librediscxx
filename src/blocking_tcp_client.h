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

LIBREDIS_NAMESPACE_BEGIN

// single thread safety
class TcpClient
{
  private:
    class Impl;
    Impl * impl_;

  public:
    TcpClient();
    ~TcpClient();

    // all 'timeout' is in milliseconds
    void connect(
        const std::string& ip_or_host,
        const std::string& port_or_service,
        size_t timeout,
        boost::system::error_code& ec);

    void write(
        const std::string& line,
        size_t timeout,
        boost::system::error_code& ec);

    // if 'timeout' is zero, block to read
    std::string read(
        size_t size,
        const std::string& delim,
        size_t timeout,
        boost::system::error_code& ec);

    // if 'timeout' is zero, block to read until 'delim'
    std::string read_line(
        const std::string& delim,
        size_t timeout,
        boost::system::error_code& ec);

    void close();

    bool is_open()const;

    bool available()const;
};

LIBREDIS_NAMESPACE_END

#endif// _LANGTAOJIN_LIBREDIS_BLOCKING_TCP_CLIENT_H_
