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

typedef std::vector<boost::asio::ip::tcp::endpoint> endpoint_vector_t;

// multi thread safety
class HostResolver
{
  private:
    class Impl;
    Impl * impl_;

  public:
    explicit HostResolver(bool enable_cache = true);
    ~HostResolver();

    void resolve_host(
        const std::string& host,
        const std::string& service,
        endpoint_vector_t& endpoints,
        boost::system::error_code& ec);

    void clear_cache();
};

LIBREDIS_NAMESPACE_END

#endif// _LANGTAOJIN_LIBREDIS_HOST_RESOLVER_H_
