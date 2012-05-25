/** @file
 * @brief RedisTss: thread specific storage support,
 *                  also can be used as a connection pool
 * @author yafei.zhang@langtaojin.com
 * @date
 * @version
 *
 */
#include "redis_tss.h"
#include "redis.h"
#include "redis_protocol.h"
#include "redis_partition.h"
#include "tss.h"
#include <set>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>

LIBREDIS_NAMESPACE_BEGIN

class RedisTss::Impl
{
  public:
    Impl(const std::string& host, int port,
        int db_index, int partitions,
        int timeout_ms, kRedisClientType type,
        size_t pool_size, size_t check_interval);

    Impl(const std::string& host, const std::string& port,
        int db_index, int partitions,
        int timeout_ms, kRedisClientType type,
        size_t pool_size, size_t check_interval);

    ~Impl();

    RedisBase2 * get();

  private:
    const std::string host_;
    const std::string port_;
    const int db_index_;
    const int timeout_ms_;
    const int partitions_;
    const kRedisClientType type_;
    const size_t pool_size_;
    const size_t check_interval_;

    std::vector<std::string> redis_hosts_;

    //boost::mutex invalid_redis_lock_;
    //std::set<size_t> invalid_redis_;

    boost::scoped_ptr<boost::thread> check_thread_;
    boost::asio::io_service io_service_;
    boost::asio::deadline_timer check_timer_;

    boost::mutex free_redis_lock_;
    std::vector<RedisBase2 *> free_redis_;

    // NOTICE:
    // 'client_' must be the last one, constructed last and destroyed first.
    // Because its cleanup handler may use 'free_redis_lock_', 'free_redis_'
    // or some others members.
    thread_specific_ptr<RedisBase2> client_;

    void inner_init();
    //void check_invalid_redis();
    //void set_check();
    //void start_check();
    //void stop_check();

    void clear_free_redis()
    {
      boost::mutex::scoped_lock guard(free_redis_lock_);
      for (size_t i=0; i<free_redis_.size(); i++)
      {
        delete free_redis_[i];
      }
      free_redis_.clear();
    }

    RedisBase2 * get_free_redis()
    {
      boost::mutex::scoped_lock guard(free_redis_lock_);
      if (free_redis_.empty())
        return NULL;

      RedisBase2 * redis = free_redis_.back();
      free_redis_.pop_back();
      return redis;
    }

    void put_free_redis(RedisBase2 * redis)
    {
      if (redis==NULL)
        return;

      boost::mutex::scoped_lock guard(free_redis_lock_);
      if (free_redis_.size()>=pool_size_)
      {
        guard.unlock();
        delete redis;
      }
      else
      {
        free_redis_.push_back(redis);
      }
    }
};

RedisTss::Impl::Impl(const std::string& host, int port,
    int db_index, int partitions,
    int timeout_ms, kRedisClientType type,
    size_t pool_size, size_t check_interval)
: host_(host), port_(boost::lexical_cast<std::string>(port)),
  db_index_(db_index), timeout_ms_(timeout_ms),
  partitions_(partitions), type_(type),
  pool_size_(pool_size), check_interval_(check_interval_),
  io_service_(), check_timer_(io_service_),
  client_(boost::bind(&RedisTss::Impl::put_free_redis, this, _1))
{
  inner_init();
}

RedisTss::Impl::Impl(const std::string& host, const std::string& port,
    int db_index, int partitions,
    int timeout_ms, kRedisClientType type,
    size_t pool_size, size_t check_interval)
: host_(host), port_(port),
  db_index_(db_index), timeout_ms_(timeout_ms),
  partitions_(partitions), type_(type),
  pool_size_(pool_size), check_interval_(check_interval_),
  io_service_(), check_timer_(io_service_),
  client_(boost::bind(&RedisTss::Impl::put_free_redis, this, _1))
{
  inner_init();
}

RedisTss::Impl::~Impl()
{
  // stop_check();
  clear_free_redis();
}

RedisBase2 * RedisTss::Impl::get()
{
  RedisBase2 * redis_ptr = client_.get();
  if (redis_ptr==NULL)
  {
    redis_ptr = get_free_redis();
    if (redis_ptr==NULL)
    {
      switch (type_)
      {
        case kNormal:
          redis_ptr = new Redis2(host_, port_, db_index_, timeout_ms_);// may throw
          break;
        case kPartition:
          redis_ptr = new Redis2P(host_, port_, db_index_, timeout_ms_, partitions_);// may throw
          break;
      }
    }

    client_.reset(redis_ptr);
  }

  //if (type_==kPartition && redis_hosts_.size()>1)
  //{
  //  boost::mutex::scoped_lock guard(invalid_redis_lock_);
  //  (static_cast<Redis2P *>(redis_ptr))->set_invalid_redis(invalid_redis_);
  //}
  return redis_ptr;
}

void RedisTss::Impl::inner_init()
{
  boost::split(redis_hosts_, host_, boost::is_any_of(","));

  //if (type_==kPartition && redis_hosts_.size()>1)
  //{
  //  start_check();
  //}
}

//void RedisTss::Impl::check_invalid_redis()
//{
//  std::set<size_t> invalid_redis_tmp;
//  bool has_changed = false;
//  for (size_t i=0; i<redis_hosts_.size(); i++)
//  {
//    RedisProtocol r(redis_hosts_[i], port_, timeout_ms_);
//    if (!r.connect())
//    {
//      invalid_redis_tmp.insert(i);
//      if (invalid_redis_.find(i)==invalid_redis_.end())
//      {
//        has_changed = true;
//      }
//    }
//    else
//    {
//      if (invalid_redis_.find(i)!=invalid_redis_.end())
//      {
//        has_changed = true;
//      }
//    }
//  }
//
//  if (has_changed)
//  {
//    boost::mutex::scoped_lock guard(invalid_redis_lock_);
//    invalid_redis_.swap(invalid_redis_tmp);
//  }
//
//  set_check();
//}
//
//void RedisTss::Impl::set_check()
//{
//  check_timer_.expires_from_now(boost::posix_time::seconds(check_interval_));
//  check_timer_.async_wait(boost::bind(&RedisTss::Impl::check_invalid_redis, this));
//}
//
//void RedisTss::Impl::start_check()
//{
//  if (!check_thread_)
//  {
//    check_invalid_redis();
//
//    io_service_.reset();
//    check_thread_.reset(new boost::thread
//        (boost::bind(&boost::asio::io_service::run, &io_service_)));// may throw
//  }
//}
//
//void RedisTss::Impl::stop_check()
//{
//  if (check_thread_)
//  {
//    io_service_.stop();
//    check_thread_->join();
//    check_thread_.reset();
//  }
//}

/************************************************************************/
/*RedisTss*/
/************************************************************************/
RedisTss::RedisTss(const std::string& host, int port,
    int db_index, int partitions,
    int timeout_ms, kRedisClientType type,
    size_t pool_size, size_t check_interval)
{
  impl_ = new Impl(host, port, db_index, partitions, timeout_ms, type,
      pool_size, check_interval);// may throw
}

RedisTss::RedisTss(const std::string& host, const std::string& port,
    int db_index, int partitions,
    int timeout_ms, kRedisClientType type,
    size_t pool_size, size_t check_interval)
{
  impl_ = new Impl(host, port, db_index, partitions, timeout_ms, type,
      pool_size, check_interval);// may throw
}

RedisTss::~RedisTss()
{
  delete impl_;
}

RedisBase2 * RedisTss::get()
{
  return impl_->get();
}

LIBREDIS_NAMESPACE_END
