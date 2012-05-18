/** @file
 * @brief host resolver
 * @author yafei.zhang@langtaojin.com
 * @date
 * @version
 * inner header
 */
#ifndef _LANGTAOJIN_LIBREDIS_HOST_RESOLVER_H_
#define _LANGTAOJIN_LIBREDIS_HOST_RESOLVER_H_

#include "redis_common.h"
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

LIBREDIS_NAMESPACE_BEGIN

// single thread safety
class HostResolver
{
  private:
    class Impl;
    Impl * impl_;

  public:
    HostResolver();
    ~HostResolver();

    /*
     * usage of the return value of resolve_host:

     * 1.iterator all the hosts:
     * iter = resolve_host(...);
     * // check if ec is ok
     * for (; iter!=boost::asio::ip::tcp::resolver::iterator(); ++iter)
     * {
     * }

     * 2.judge the host is resolved:
     * iter = resolve_host(...);
     * // check if ec is ok
     * if (iter!=boost::asio::ip::tcp::resolver::iterator())
     * {
     * }
     */

    // resolve host without a timeout
    boost::asio::ip::tcp::resolver::iterator resolve_host(
        const std::string& ip_or_host,
        const std::string& port_or_service,
        boost::system::error_code& ec);

    // resolve host with a timeout
    boost::asio::ip::tcp::resolver::iterator resolve_host(
        const std::string& ip_or_host,
        const std::string& port_or_service,
        boost::posix_time::time_duration timeout,
        boost::system::error_code& ec);
};

LIBREDIS_NAMESPACE_END

#endif// _LANGTAOJIN_LIBREDIS_HOST_RESOLVER_H_
