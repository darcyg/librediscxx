/** @file
* @brief redis monitor program
* @author yafei.zhang@langtaojin.com
* @date
* @version
*
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <map>
#include <vector>
#include <gflags/gflags.h>
#include <boost/asio.hpp>
#include <boost/date_time.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>
#include "redis_protocol.h"

DEFINE_bool(redis_monitor, false, "use command MONITOR");
DEFINE_string(redis_host, "localhost", "redis host.");
DEFINE_string(redis_port, "6379", "redis port.");
DEFINE_int32(redis_timeout, 1000, "redis timeout in ms.");
DEFINE_int32(update_cycle, 5, "update cycle in seconds.");

typedef boost::asio::ip::tcp::socket tcp_socket;
typedef boost::asio::deadline_timer deadline_timer;
typedef boost::asio::io_service io_service;

using namespace com::langtaojin::adgaga;

namespace
{
  typedef std::pair<std::string, int> cmd_db_t;
  typedef std::map<cmd_db_t, int> cmd_db_map_t;
  typedef std::map<std::string, int> cmd_map_t;

  cmd_db_map_t s_cmd_db_map;
  cmd_map_t s_cmd_map;

  bool s_stop_flag = false;
  io_service s_ios;
  boost::scoped_ptr<RedisProtocol> s_monitor;
  boost::scoped_ptr<RedisProtocol> s_info;
  boost::scoped_ptr<deadline_timer> s_timer;

  bool init_monitor()
  {
    s_monitor.reset(new RedisProtocol(FLAGS_redis_host,
      FLAGS_redis_port, FLAGS_redis_timeout));
    if (!s_monitor->assure_connect(NULL))
    {
      fprintf(stderr, "connection failed\n");
      s_monitor.reset();
      return false;
    }

    RedisCommand command;
    va_list ap;
    bool ret;
    memset(&ap, 0, sizeof(ap));
    ret = s_monitor->exec_command(&command, "MONITOR", ap);

    if (!ret)
    {
      s_monitor.reset();
      fprintf(stderr, "MONITOR failed\n");
      return false;
    }

    if (!command.out.is_status_ok())
    {
      s_monitor.reset();
      fprintf(stderr, "MONITOR response is not OK\n");
      return false;
    }

    fprintf(stderr, "%s\n", command.out.ptr.status->c_str());
    return true;
  }


  void monitor()
  {
    if (!s_monitor)
      init_monitor();
    if (!s_monitor)
      return;

    RedisProtocol * pro = s_monitor.get();
    bool ret = true;
    std::string line;
    char * enddb;
    char * endcmd;
    int db;
    std::string cmd;

    while (pro->available() && !s_stop_flag)
    {
      ret = pro->read_line(&line);
      if (ret)
      {
        //printf("%s\n", line.c_str());
        //example:
        //+1317279070.687700 (db 1) "EXPIRE" "tag_pv2_de8c68" "3600"
        //+1317280373.276440 "MONITOR"
        if (line.size() < 24 || line[20]!='d')
          continue;

        db = static_cast<int>(strtoul(&line[23], &enddb, 10));
        if (errno == ERANGE)
          continue;

        enddb += 3;
        endcmd = strchr(enddb, '"');
        if (endcmd == NULL)
          continue;

        cmd.assign(enddb, endcmd);

        s_cmd_db_map[std::make_pair(cmd, db)]++;
        s_cmd_map[cmd]++;
      }
      else
      {
        s_monitor.reset();
        break;
      }
    }
  }


  void uninit_monitor()
  {
    s_monitor.reset();
  }


  void print_monitor()
  {
    {
      cmd_db_map_t::const_iterator first = s_cmd_db_map.begin();
      cmd_db_map_t::const_iterator last = s_cmd_db_map.end();
      for (; first!=last; ++first)
      {
        printf("[%s %d]:%d\n", (*first).first.first.c_str(),
          (*first).first.second, (*first).second);
      }
      s_cmd_db_map.clear();
    }

    {
      cmd_map_t::const_iterator first = s_cmd_map.begin();
      cmd_map_t::const_iterator last = s_cmd_map.end();
      for (; first!=last; ++first)
      {
        printf("[%s]:%d\n", (*first).first.c_str(), (*first).second);
      }
      s_cmd_map.clear();
    }
  }


  void init_info()
  {
    s_info.reset(new RedisProtocol(FLAGS_redis_host,
      FLAGS_redis_port, FLAGS_redis_timeout));
    if (!s_info->assure_connect(NULL))
    {
      fprintf(stderr, "connection failed\n");
      s_info.reset();
      return;
    }
  }


  void info()
  {
    if (!s_info)
      init_info();
    if (!s_info)
      return;

    RedisCommand command;
    va_list ap;
    bool ret;
    memset(&ap, 0, sizeof(ap));
    ret = s_info->exec_command(&command, "INFO", ap);

    if (!ret)
    {
      s_info.reset();
      fprintf(stderr, "INFO failed\n");
      return;
    }

    if (!command.out.is_bulk())
    {
      s_info.reset();
      fprintf(stderr, "INFO response is not OK\n");
      return;
    }

    printf("%s", command.out.ptr.bulk->c_str());
    //some useful infos:
    //connected_clients:859
    //used_memory:5923363536
    //used_memory_rss:7423385600
    //mem_fragmentation_ratio:1.25
    //expired_keys:16372092
    //evicted_keys:0
    //keyspace_hits:1424977714
    //keyspace_misses:339686219
    //hash_max_zipmap_entries:256
    //hash_max_zipmap_value:256
    //db0:keys=19749,expires=0
    //db1:keys=136557,expires=136543
    //db2:keys=131072,expires=0
    //db3:keys=131072,expires=0
    //db4:keys=131072,expires=0
    //db5:keys=205,expires=0
    //db6:keys=58238,expires=0
    //db7:keys=44855,expires=0
  }


  void ping()
  {
    if (!s_info)
      init_info();
    if (!s_info)
      return;

    boost::posix_time::ptime begin, end;
    RedisCommand command;
    va_list ap;
    bool ret;
    int iteration = 3;

    memset(&ap, 0, sizeof(ap));
    begin = boost::posix_time::microsec_clock::local_time();

    for (int i=0; i<iteration; i++)
    {
      ret = s_info->exec_command(&command, "PING", ap);
      if (!ret)
      {
        s_info.reset();
        fprintf(stderr, "PING failed\n");
        return;
      }

      if (!command.out.is_status_pong())
      {
        s_info.reset();
        fprintf(stderr, "PING response is not PONG\n");
        return;
      }
    }

    end = boost::posix_time::microsec_clock::local_time();
    boost::posix_time::time_duration td = end - begin;
    int ms = static_cast<int>(td.total_milliseconds())/iteration;
    printf("[PING]:%d\n", ms);
  }


  void uninit_info()
  {
    s_info.reset();
  }


  void update(const boost::system::error_code& ec)
  {
    if (ec)
      return;

    printf("{begin}\n");
    printf("%lu\n", static_cast<unsigned long>(time(NULL)));
    printf("%s:%s\n", FLAGS_redis_host.c_str(), FLAGS_redis_port.c_str());
    if (FLAGS_redis_monitor)
    {
      monitor();
      print_monitor();
    }
    info();
    ping();
    printf("{end}\n");

    boost::posix_time::ptime next = s_timer->expires_at();
    boost::posix_time::seconds cycle = boost::posix_time::seconds(
      FLAGS_update_cycle);
    while (next <= boost::asio::deadline_timer::traits_type::now())
      next = next + cycle;

    s_timer->expires_at(next);
    s_timer->async_wait(update);
  }


  void signal_handler(int signo)
  {
    s_stop_flag = true;
    s_ios.stop();
  }
}

int main(int argc, char ** argv)
{
  google::ParseCommandLineFlags(&argc, &argv, true);

  signal(SIGINT, signal_handler);

  s_timer.reset(new deadline_timer(s_ios));
  if (FLAGS_redis_monitor)
    init_monitor();
  init_info();

  printf("starting...\n");

  time_t now = time(NULL);
  now = now - (now % FLAGS_update_cycle) + FLAGS_update_cycle;
  boost::posix_time::ptime next = boost::posix_time::from_time_t(now);
  boost::posix_time::seconds cycle = boost::posix_time::seconds(FLAGS_update_cycle);
  while (next <= boost::asio::deadline_timer::traits_type::now())
    next = next + cycle;

  s_timer->expires_at(next);
  s_timer->async_wait(update);

  s_ios.reset();
  s_ios.run();

  printf("exiting...\n");

  uninit_info();
  if (FLAGS_redis_monitor)
    uninit_monitor();
  s_timer.reset();
  google::ShutDownCommandLineFlags();
  return 0;
}
