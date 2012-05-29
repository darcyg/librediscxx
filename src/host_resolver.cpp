/** @file
 * @brief host resolver
 * @author yafei.zhang@langtaojin.com
 * @date
 * @version
 *
 */
#include "host_resolver.h"
#include <boost/bind.hpp>
#include <boost/thread.hpp>

LIBREDIS_NAMESPACE_BEGIN

class HostResolver::Impl
{
  private:
    typedef std::pair<std::string, std::string> host_service_t;

    struct HostEntry
    {
      endpoint_vector_t endpoints;
      boost::system::error_code ec;
    };

    typedef std::map<host_service_t, HostEntry> dns_cache_t;

    dns_cache_t dns_cache_;
    mutable boost::shared_mutex dns_cache_mutex_;

    boost::asio::io_service io_service_;
    boost::asio::ip::tcp::resolver resolver_;

    void __resolve_host(
        const std::string& host,
        const std::string& service,
        endpoint_vector_t& endpoints,
        boost::system::error_code& ec)
    {
      endpoints.clear();

      boost::asio::ip::tcp::resolver::query query(host, service);
      boost::asio::ip::tcp::resolver::iterator iter = resolver_.resolve(query, ec);

      if (ec)
        return;

      for (; iter!=boost::asio::ip::tcp::resolver::iterator(); ++iter)
        endpoints.push_back(iter->endpoint());
    }

    bool __lookup_cache(
        const std::string& host,
        const std::string& service,
        endpoint_vector_t& endpoints,
        boost::system::error_code& ec)const
    {
      host_service_t key = std::make_pair(host, service);
      dns_cache_t::const_iterator iter;

      boost::shared_lock<boost::shared_mutex> guard(dns_cache_mutex_);

      iter = dns_cache_.find(key);
      if (iter == dns_cache_.end())
        return false;

      endpoints = (*iter).second.endpoints;
      ec = (*iter).second.ec;
      return true;
    }

    void __update_cache(
        const std::string& host,
        const std::string& service,
        const endpoint_vector_t& endpoints,
        const boost::system::error_code& ec)
    {
      host_service_t key = std::make_pair(host, service);
      HostEntry host_entry;
      host_entry.endpoints = endpoints;
      host_entry.ec = ec;

      boost::unique_lock<boost::shared_mutex> guard(dns_cache_mutex_);
      dns_cache_[key] = host_entry;
    }

  public:
    Impl()
      : io_service_(),
      resolver_(io_service_) {}

    ~Impl() {}

    void resolve_host(
        const std::string& host,
        const std::string& service,
        endpoint_vector_t& endpoints,
        boost::system::error_code& ec)
    {
      if (__lookup_cache(host, service, endpoints, ec))
        return;

      __resolve_host(host, service, endpoints, ec);
      __update_cache(host, service, endpoints, ec);
    }

    void clear_cache()
    {
      boost::unique_lock<boost::shared_mutex> guard(dns_cache_mutex_);
      dns_cache_.clear();
    }
};

HostResolver::HostResolver()
{
  impl_ = new Impl();// may throw
}

HostResolver::~HostResolver()
{
  delete impl_;
}

void HostResolver::resolve_host(const std::string& host,
    const std::string& service,
    endpoint_vector_t& endpoints,
    boost::system::error_code& ec)
{
  impl_->resolve_host(host, service, endpoints, ec);
}

void HostResolver::clear_cache()
{
  impl_->clear_cache();
}

LIBREDIS_NAMESPACE_END
