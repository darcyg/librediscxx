/** @file
* @brief host resolver
* @author yafei.zhang@langtaojin.com
* @date
* @version
*
*/
#include <boost/bind.hpp>
#include "host_resolver.h"

NAMESPACE_BEGIN

class HostResolver::Impl
{
private:
  boost::asio::io_service io_service_;
  boost::asio::ip::tcp::resolver resolver_;
  boost::asio::deadline_timer resolver_timer_;


  void __resolve_timeout_handler()
  {
    if (resolver_timer_.expires_at() <= boost::asio::deadline_timer::traits_type::now())
    {
      //The deadline has passed
      resolver_.cancel();

      resolver_timer_.expires_at(boost::posix_time::pos_infin);
    }
    resolver_timer_.async_wait(boost::bind(&HostResolver::Impl::__resolve_timeout_handler, this));
  }


  void __resolve_handler(
    const boost::system::error_code& error,
    boost::asio::ip::tcp::resolver::iterator iterator,
    boost::system::error_code * error_out,
    boost::asio::ip::tcp::resolver::iterator * iterator_out)
  {
    resolver_timer_.cancel();

    if (error_out)
      *error_out = error;
    if (iterator_out)
      *iterator_out = iterator;
  }


public:
  Impl()
    :io_service_(),
    resolver_(io_service_),
    resolver_timer_(io_service_)
  {}


  ~Impl()
  {}


  boost::asio::ip::tcp::resolver::iterator resolve_host(
    const std::string& ip_or_host,
    const std::string& port_or_service,
    boost::system::error_code& ec)
  {
    boost::asio::ip::tcp::resolver::query query(ip_or_host, port_or_service);
    boost::asio::ip::tcp::resolver::iterator iterator_ret = resolver_.resolve(query, ec);

    if (ec)
    {
      //return an invalid iterator
      return boost::asio::ip::tcp::resolver::iterator();
    }

    return iterator_ret;
  }


  boost::asio::ip::tcp::resolver::iterator resolve_host(
    const std::string& ip_or_host,
    const std::string& port_or_service,
    boost::posix_time::time_duration timeout,
    boost::system::error_code& ec)
  {
    boost::asio::ip::tcp::resolver::query query(ip_or_host, port_or_service);
    boost::asio::ip::tcp::resolver::iterator iterator_ret;

    ec = boost::asio::error::would_block;

    resolver_.async_resolve(query,
      boost::bind(&HostResolver::Impl::__resolve_handler, this, _1, _2, &ec, &iterator_ret));

    //set timer
    resolver_timer_.expires_from_now(timeout);
    resolver_timer_.async_wait(
      boost::bind(&HostResolver::Impl::__resolve_timeout_handler, this));

    do io_service_.run_one(); while (ec == boost::asio::error::would_block);

    if (ec)
    {
      //return an invalid iterator
      return boost::asio::ip::tcp::resolver::iterator();
    }

    return iterator_ret;
  }
};


HostResolver::HostResolver()
{
  impl_ = new Impl();
}

HostResolver::~HostResolver()
{
  delete impl_;
}

boost::asio::ip::tcp::resolver::iterator HostResolver::resolve_host(
  const std::string& ip_or_host,
  const std::string& port_or_service,
  boost::system::error_code& ec)
{
  return impl_->resolve_host(ip_or_host, port_or_service, ec);
}

boost::asio::ip::tcp::resolver::iterator HostResolver::resolve_host(
  const std::string& ip_or_host,
  const std::string& port_or_service,
  boost::posix_time::time_duration timeout,
  boost::system::error_code& ec)
{
  return impl_->resolve_host(ip_or_host, port_or_service, timeout, ec);
}

NAMESPACE_END
