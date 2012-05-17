/** @file
 * @brief host resolver
 * @author yafei.zhang@langtaojin.com
 * @date
 * @version
 *
 */
#include "host_resolver.h"
#include <boost/bind.hpp>

LIBREDIS_NAMESPACE_BEGIN

class HostResolver::Impl
{
  private:
    boost::asio::io_service io_service_;
    boost::asio::ip::tcp::resolver resolver_;
    boost::asio::deadline_timer resolver_timer_;


    void __resolve_timeout_handler(
        const boost::system::error_code& ec,
        boost::system::error_code * error_out)
    {
      if (ec == boost::asio::error::operation_aborted)
      {
        // The timer was canceled, so the resolver is ok.
      }
      else
      {
        // The deadline has passed, cancel the resolver.
        resolver_.cancel();

        // Unluckily, there is a bug that resolver's cancellation works unexpectedly:
        // https://svn.boost.org/trac/boost/ticket/6138
        // Wait for its patch.

        // Temporarily, we assign 'error_out', and check it in '__resolve_handler'.
        // But the cancellation is still not immediate.
        *error_out = boost::asio::error::make_error_code(boost::asio::error::operation_aborted);
      }
    }


    void __resolve_handler(
        const boost::system::error_code& ec,
        boost::asio::ip::tcp::resolver::iterator iterator,
        boost::system::error_code * error_out,
        boost::asio::ip::tcp::resolver::iterator * iterator_out)
    {
      if (*error_out == boost::asio::error::operation_aborted
          || ec == boost::asio::error::operation_aborted)
      {
        // The resolver was canceled, timed out.
      }
      else
      {
        // The resolver is ok, cancel the timer.
        resolver_timer_.cancel();

        *error_out = ec;
        *iterator_out = iterator;
      }
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
          boost::bind(&HostResolver::Impl::__resolve_timeout_handler, this, _1, &ec));

      io_service_.reset();
      io_service_.run();
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

LIBREDIS_NAMESPACE_END
