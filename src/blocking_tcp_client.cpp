/** @file
 * @brief a blocking tcp client with timeout
 * @author yafei.zhang@langtaojin.com
 * @date
 * @version
 *
 */
#include "blocking_tcp_client.h"
#include "host_resolver.h"
#include <assert.h>
#include <time.h>
#include <string.h>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

LIBREDIS_NAMESPACE_BEGIN

static HostResolver s_host_resolver;

enum
{
  kDefaultBufferSize = 512,
  kMaxBufferSize = 65536,

  kConnectTimeoutProportion = 5,
  kMinConnectTimeout = 10,

  kCheckOpenInterval = 180
};

/************************************************************************/
/*TcpClientBuffer*/
/************************************************************************/
class TcpClientBuffer
{
  private:
    std::vector<char> buffer_;
    char * read_;// ptr to first byte already read
    char * to_read_;// ptr to first byte to be read next time
    // buffer_.begin()<=read<=to_read<=buffer_.end()

    inline char * begin()
    {
      return &buffer_[0];
    }

    inline char * end()
    {
      return &buffer_[0] + buffer_.size();
    }

    inline const char * begin()const
    {
      return &buffer_[0];
    }

    inline const char * end()const
    {
      return &buffer_[0] + buffer_.size();
    }

  public:
    explicit TcpClientBuffer(size_t buf_size = kDefaultBufferSize)
      :buffer_(buf_size), read_(begin()), to_read_(begin())
    {
      assert(buf_size);
    }

    inline boost::asio::const_buffers_1 get_read_buffer()const
    {
      return boost::asio::const_buffers_1(read_, read_size());
    }

    inline boost::asio::mutable_buffers_1 get_to_read_buffer()const
    {
      return boost::asio::mutable_buffers_1(to_read_, free_size());
    }

    inline size_t total_size()const
    {
      return buffer_.size();
    }

    inline size_t consumed_size()const
    {
      return read_ - begin();
    }

    inline size_t read_size()const
    {
      return to_read_ - read_;
    }

    inline size_t free_size()const
    {
      return end() - to_read_;
    }

    inline void clear()
    {
      buffer_.resize(kDefaultBufferSize);
      read_ = to_read_ = begin();
    }

    void prepare(size_t size = kDefaultBufferSize, bool drain = false)
    {
      if (size==0)
        return;

      if (!drain && free_size()>=size)
        return;

      size_t rsize = read_size();
      size_t min_new_size = rsize + size;
      size_t new_size = buffer_.size();

      for (; new_size<=min_new_size; new_size = new_size << 1)
      {
      }

      std::vector<char> buffer(new_size);
      ::memcpy(&buffer[0], read_, rsize);

      buffer.swap(buffer_);
      read_ = begin();
      to_read_ = read_ + rsize;
    }

    void produce(size_t size)
    {
      if (size==0)
        return;

      assert(free_size()>=size);

      to_read_ += size;
    }

    std::string consume(size_t size)
    {
      assert(size);
      assert(read_size()>=size);

      char * read = read_;
      read_ += size;
      std::string ret(read, size);

      if (read_size()==0 && total_size()>=kMaxBufferSize)
        drain();
      else if (read_size()==0)
        read_ = to_read_ = begin();

      return ret;
    }

    inline void drain()
    {
      prepare(kDefaultBufferSize, true);
    }
};

/************************************************************************/
/*TcpClient::Impl*/
/************************************************************************/
class TcpClient::Impl
{
  private:
    boost::asio::io_service io_service_;
    mutable boost::asio::ip::tcp::socket socket_;
    boost::asio::deadline_timer socket_timer_;

    TcpClientBuffer buffer_;

    mutable time_t last_check_open_time_;

  private:
    void __connect_handler(
        const boost::system::error_code& error,
        boost::system::error_code * error_out)
    {
      *error_out = error;
    }

    void __read_write_handler(
        const boost::system::error_code& error,
        size_t size,
        boost::system::error_code * error_out,
        size_t * size_out)
    {
      *error_out = error;
      *size_out = size;
    }

    void __socket_timeout_handler()
    {
      if (socket_timer_.expires_at()<=boost::asio::deadline_timer::traits_type::now())
      {
        // The deadline has passed
        // close();
        boost::system::error_code ec;
        socket_.close(ec);
        socket_timer_.expires_at(boost::posix_time::pos_infin);
      }
      socket_timer_.async_wait(boost::bind(&TcpClient::Impl::__socket_timeout_handler, this));
    }

    void __init_socket_timer()
    {
      socket_timer_.expires_at(boost::posix_time::pos_infin);

      __socket_timeout_handler();
    }

    //inline boost::asio::ip::tcp::resolver::iterator __resolve_host(
    //    const std::string& ip_or_host,
    //    const std::string& port_or_service,
    //    boost::posix_time::time_duration timeout,
    //    boost::system::error_code& ec)
    //{
    //  HostResolver host_resolver;
    //  return host_resolver.resolve_host(ip_or_host, port_or_service, timeout, ec);
    //}

  public:
    Impl()
      :io_service_(),
      socket_(io_service_),
      socket_timer_(io_service_),
      last_check_open_time_(0)
  {
    __init_socket_timer();
  }

    ~Impl()
    {
    }

    inline void connect(
        const std::string& ip_or_host,
        const std::string& port_or_service,
        boost::posix_time::time_duration timeout,
        boost::system::error_code& ec)
    {
      endpoint_vector_t endpoints;
      s_host_resolver.resolve_host(ip_or_host, port_or_service, endpoints, ec);
      if (ec)
        return;

      // adjust timeout
      timeout /= kConnectTimeoutProportion;
      if (timeout.total_milliseconds()<kMinConnectTimeout)
        timeout = boost::posix_time::milliseconds(kMinConnectTimeout);

      // set timer
      socket_timer_.expires_from_now(timeout);

      for (size_t i=0; i<endpoints.size(); i++)
      {
        close();
        io_service_.reset();

        socket_.open(endpoints[i].protocol(), ec);
        if (ec)
          continue;

        // use non-block mode
        boost::asio::socket_base::non_blocking_io command(true);
        socket_.io_control(command, ec);
        if (ec)
          continue;

        ec = boost::asio::error::would_block;
        socket_.async_connect(endpoints[i],
            boost::bind(&TcpClient::Impl::__connect_handler, this, _1, &ec));

        do io_service_.run_one(); while (ec==boost::asio::error::would_block);

        if (!ec && socket_.is_open())
        {
          ::time(&last_check_open_time_);
          return;
        }
      }

      if (!ec)
        ec = boost::asio::error::host_not_found;
      return;
    }

    inline void write(
        const std::string& line,
        boost::posix_time::time_duration timeout,
        boost::system::error_code& ec)
    {
      size_t count;

      // set timer
      socket_timer_.expires_from_now(timeout);

      ec = boost::asio::error::would_block;
      boost::asio::async_write(socket_, boost::asio::buffer(line),
          boost::bind(&TcpClient::Impl::__read_write_handler, this, _1, _2, &ec, &count));
      do io_service_.run_one(); while (ec==boost::asio::error::would_block);

      if (ec)
      {
        close();
        return;
      }
      ::time(&last_check_open_time_);
    }

    inline std::string read(
        size_t size,
        const std::string& delim,
        boost::posix_time::time_duration timeout,
        boost::system::error_code& ec)
    {
      assert(size && !delim.empty());

      std::string line;
      const size_t delim_size = delim.size();
      size_t expect = size + delim_size;
      size_t to_read = expect - buffer_.read_size();
      size_t count;

      // set timer
      socket_timer_.expires_from_now(timeout);

      for (;;)
      {
        // try consume (previous) buffer
        if (buffer_.read_size()>=expect)
        {
          boost::asio::const_buffers_1 bufs = buffer_.get_read_buffer();
          line = std::string(
              boost::asio::buffers_begin(bufs),
              boost::asio::buffers_begin(bufs) + expect);
          buffer_.consume(expect);

          // remove delim
          line.resize(line.size() - delim_size);
          return line;
        }

        // try async_read
        buffer_.prepare(to_read);
        boost::asio::async_read(socket_, buffer_.get_to_read_buffer(),
            boost::asio::transfer_at_least(1),
            boost::bind(&TcpClient::Impl::__read_write_handler, this, _1, _2, &ec, &count));

        ec = boost::asio::error::would_block;
        do io_service_.run_one(); while (ec==boost::asio::error::would_block);

        if (ec)
        {
          close();
          return std::string();
        }
        // push in buffer_
        buffer_.produce(count);
        to_read -= count;
      }
    }

    inline std::string read_line(
        const std::string& delim,
        boost::posix_time::time_duration timeout,
        boost::system::error_code& ec)
    {
      assert(!delim.empty());

      std::string line;
      const size_t delim_size = delim.size();
      const char * search_begin;
      const char * search_end;
      const char * search_curr;
      size_t search_offset = 0;
      size_t to_consume;
      size_t count;

      // set timer
      socket_timer_.expires_from_now(timeout);

      for (;;)
      {
        // try consume (previous) buffer
        if (buffer_.read_size()>=delim_size)
        {
          boost::asio::const_buffers_1 bufs = buffer_.get_read_buffer();
          // find delim in buffer_
          search_begin = boost::asio::buffer_cast<const char *>(bufs);
          search_end = search_begin + boost::asio::buffer_size(bufs) - delim_size;
          search_curr = search_begin + search_offset;

          for (; search_curr<=search_end; search_curr++, search_offset++)
          {
            if (::memcmp(delim.c_str(), search_curr, delim_size)==0)
            {
              // find delim in buffer_, grep it and return

              to_consume = search_curr - search_begin + delim_size;
              line = std::string(
                  boost::asio::buffers_begin(bufs),
                  boost::asio::buffers_begin(bufs) + to_consume);
              buffer_.consume(to_consume);

              // remove delim
              line.resize(line.size() - delim_size);
              return line;
            }
          }
        }

        // try async_read
        buffer_.prepare();
        boost::asio::async_read(socket_, buffer_.get_to_read_buffer(),
            boost::asio::transfer_at_least(1),
            boost::bind(&TcpClient::Impl::__read_write_handler, this, _1, _2, &ec, &count));

        ec = boost::asio::error::would_block;
        do io_service_.run_one(); while (ec==boost::asio::error::would_block);

        if (ec)
        {
          close();
          return std::string();
        }

        // push in buffer_
        buffer_.produce(count);
      }
    }

    inline void close()
    {
      boost::system::error_code ec;
      socket_.close(ec);
      buffer_.clear();
    }

    inline bool is_open()const
    {
      if (!socket_.is_open())
        return false;

      // to detect the close of redis server
      time_t now;
      ::time(&now);
      if (now - last_check_open_time_>kCheckOpenInterval)
      {
        last_check_open_time_ = now;
        char c;
        boost::system::error_code ec;

        size_t s = socket_.receive(boost::asio::buffer(&c, sizeof(c)),
            boost::asio::socket_base::message_peek,
            ec);

        if (s==0
            && ec!=boost::asio::error::try_again
            && ec!=boost::asio::error::would_block
            && ec!=boost::asio::error::interrupted)
        {
          return false;
        }
      }

      return true;
    }

    inline bool available()const
    {
      if (buffer_.read_size()!=0)
        return true;

      boost::system::error_code ec;
      size_t bytes = socket_.available(ec);

      if (ec)
        return false;
      return bytes!=0;
    }
};

/************************************************************************/
/*TcpClient*/
/************************************************************************/
TcpClient::TcpClient()
{
  impl_ = new Impl;// may throw
}

TcpClient::~TcpClient()
{
  delete impl_;
}

void TcpClient::connect(const std::string& ip_or_host,
    const std::string& port_or_service,
    boost::posix_time::time_duration timeout,
    boost::system::error_code& ec)
{
  impl_->connect(ip_or_host, port_or_service, timeout, ec);
}

void TcpClient::write(const std::string& line,
    boost::posix_time::time_duration timeout,
    boost::system::error_code& ec)
{
  impl_->write(line, timeout, ec);
}

std::string TcpClient::read(size_t size,
    const std::string& delim,
    boost::posix_time::time_duration timeout,
    boost::system::error_code& ec)
{
  return impl_->read(size, delim, timeout, ec);
}

std::string TcpClient::read_line(const std::string& delim,
    boost::posix_time::time_duration timeout,
    boost::system::error_code& ec)
{
  return impl_->read_line(delim, timeout, ec);
}

void TcpClient::close()
{
  impl_->close();
}

bool TcpClient::is_open()const
{
  return impl_->is_open();
}

bool TcpClient::available()const
{
  return impl_->available();
}

LIBREDIS_NAMESPACE_END
