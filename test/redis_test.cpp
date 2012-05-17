/** @file
 * @brief unit test
 * @author yafei.zhang@langtaojin.com
 * @date
 * @version
 *
 */
#include <redis_common.h>
#include <blocking_tcp_client.h>
#include <host_resolver.h>
#include <os.h>
#include <redis_protocol.h>
#include <redis_cmd.h>
#include <redis_base.h>
#include <redis.h>
#include <redis_partition.h>
#include <redis_tss.h>
#include <iostream>
#include <boost/program_options.hpp>
#include <boost/assign/std/vector.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/microsec_time_clock.hpp>
#include <boost/thread.hpp>

USING_NAMESPACE
using namespace std;
using namespace boost::assign;

namespace
{
  // command line options
  std::string host, port, host_list, port_list;
  int db_index, timeout;

  // statistics
  size_t s_total = 0;
  size_t s_failed = 0;

#define VERIFY(exp) \
  do{ \
    bool x = (bool)(exp); \
    s_total++; \
    if (!x) \
    { \
      s_failed++; \
      cout << __FILE__ << "(" << __LINE__ << "):\"" << #exp << "\" fails" << endl; \
      cout << __FUNCTION__ << " failed" << endl; \
      return 1; \
    } \
  }while(0)

#define VERIFY_MSG(exp, client) \
  do{ \
    bool x = (bool)(exp); \
    s_total++; \
    if (!x) \
    { \
      s_failed++; \
      cout << __FILE__ << "(" << __LINE__ << "):\"" << #exp << "\" fails:" \
        << (client).last_c_error() << endl; \
      cout << __FUNCTION__ << " failed" << endl; \
      return 1; \
    } \
  }while(0)

#define DUMP_TEST_RESULT() \
  do \
  { \
    cout << "*************************" << endl; \
    cout << "Total Checking Points: " << s_total << endl; \
    cout << "Failure Checking Points: " << s_failed << endl; \
    if (s_failed) cout << "Some Failures may be due to the version of redis server." << endl; \
    cout << "*************************" << endl; \
  }while (0)

  std::string s_redis_version;

  void get_redis_version()
  {
    Redis2 r(host, port, db_index, timeout);
    std::string str;

    if (!r.info(&str))
      return;

    const char * begin, * end;

    begin = strstr(str.c_str(), "redis_version:");
    if (begin == NULL)
      return;

    begin += sizeof("redis_version:") - 1;

    end = strchr(begin, '\n');
    if (end == NULL)
      return;

    s_redis_version.assign(begin, end);
    cout << "redis version: " << s_redis_version << endl;
  }

  int host_resolver_test()
  {
    cout << "host_resolver_test..." << endl;
    HostResolver res;
    boost::asio::ip::tcp::resolver::iterator iter;
    boost::posix_time::time_duration _short = boost::posix_time::milliseconds(50);
    boost::posix_time::time_duration _long = boost::posix_time::seconds(10);
    boost::system::error_code ec;

    std::string hosts[] =
    {
      "www.baidu.com",
      "www.qq.com",
    };

    for (size_t i=0; i<sizeof(hosts)/sizeof(hosts[0]); i++)
    {
      cout << "resolving host: " << hosts[i] << endl;
      iter = res.resolve_host(hosts[i], "80", _short, ec);
      if (iter != boost::asio::ip::tcp::resolver::iterator())
      {
        cout << "resolved host: " << hosts[i] << "=" << iter->endpoint() << endl;
      }
      else
      {
        cout << "resolve host: " << hosts[i] << " failed: " << ec.message() << endl;
      }
      cout << endl;
    }

    for (size_t i=0; i<sizeof(hosts)/sizeof(hosts[0]); i++)
    {
      cout << "resolving host: " << hosts[i] << endl;
      iter = res.resolve_host(hosts[i], "80", _long, ec);
      if (iter != boost::asio::ip::tcp::resolver::iterator())
      {
        cout << "resolved host: " << hosts[i] << "=" << iter->endpoint() << endl;
      }
      else
      {
        cout << "resolve host: " << hosts[i] << " failed: " << ec.message() << endl;
      }
      cout << endl;
    }

    cout << "host_resolver_test ok" << endl;
    return 0;
  }

  int os_test()
  {
    cout << "os_test..." << endl;
    cout << "thread id: " << get_thread_id() << endl;
    cout << "host name: " << get_host_name() << endl;
    cout << "os_test ok" << endl;
    return 0;
  }

  int protocol_test()
  {
    cout << "protocol_test..." << endl;

    std::string bulk;
    RedisCommand command;
    RedisProtocol rp(host, port, timeout);

    // object
    VERIFY_MSG(rp.assure_connect(NULL), rp);
    VERIFY_MSG(rp.exec_command(&command, "set string nothing"), rp);
    VERIFY_MSG(rp.exec_command(&command, "object refcount string"), rp);
    VERIFY_MSG(rp.exec_command(&command, "object encoding string"), rp);
    VERIFY_MSG(rp.exec_command(&command, "object idletime string"), rp);


    // pub/sub
    VERIFY_MSG(rp.assure_connect(NULL), rp);
    VERIFY_MSG(rp.exec_command(&command, "SUBSCRIBE %s", "a"), rp);
    VERIFY_MSG(rp.assure_connect(NULL), rp);
    VERIFY_MSG(rp.exec_command(&command, "UNSUBSCRIBE %s", "a"), rp);
    VERIFY_MSG(rp.assure_connect(NULL), rp);
    VERIFY_MSG(rp.exec_command(&command, "PSUBSCRIBE %s", "a"), rp);
    VERIFY_MSG(rp.assure_connect(NULL), rp);
    VERIFY_MSG(rp.exec_command(&command, "PUNSUBSCRIBE %s", "a"), rp);

    //// will block
    // VERIFY_MSG(rp.assure_connect(NULL), rp);
    // rp.set_blocking_mode(true);
    // VERIFY_MSG(rp.exec_command(&command, "UNSUBSCRIBE"), rp);
    // VERIFY_MSG(rp.assure_connect(NULL), rp);
    // rp.set_blocking_mode(true);
    // VERIFY_MSG(rp.exec_command(&command, "PUNSUBSCRIBE"), rp);


    // transaction
    VERIFY_MSG(rp.assure_connect(NULL), rp);
    VERIFY_MSG(rp.exec_command(&command, "SET a a"), rp);
    VERIFY_MSG(rp.exec_command(&command, "SET b b"), rp);
    VERIFY_MSG(rp.exec_command(&command, "SET c c"), rp);
    VERIFY_MSG(rp.exec_command(&command, "WATCH a"), rp);
    VERIFY_MSG(rp.exec_command(&command, "WATCH b"), rp);
    VERIFY_MSG(rp.exec_command(&command, "WATCH c"), rp);

    VERIFY_MSG(rp.exec_command(&command, "MULTI"), rp);
    // "ERR WATCH inside MULTI is not allowed"
    VERIFY(!rp.exec_command(&command, "WATCH a"));
    VERIFY_MSG(rp.exec_command(&command, "UNWATCH"), rp);
    VERIFY_MSG(rp.exec_command(&command, "DISCARD"), rp);

    VERIFY_MSG(rp.exec_command(&command, "WATCH a"), rp);
    VERIFY_MSG(rp.exec_command(&command, "WATCH b"), rp);
    VERIFY_MSG(rp.exec_command(&command, "WATCH c"), rp);
    VERIFY_MSG(rp.exec_command(&command, "UNWATCH"), rp);

    VERIFY_MSG(rp.exec_command(&command, "MULTI"), rp);
    VERIFY_MSG(rp.exec_command(&command, "GET a"), rp);
    VERIFY_MSG(rp.exec_command(&command, "GET b"), rp);
    VERIFY_MSG(rp.exec_command(&command, "GET c"), rp);
    VERIFY_MSG(rp.exec_command(&command, "KEYS *"), rp);
    VERIFY_MSG(rp.exec_command(&command, "INFO"), rp);
    // got a special multi bulk
    VERIFY_MSG(rp.exec_command(&command, "EXEC"), rp);


    // script
    VERIFY_MSG(rp.assure_connect(NULL), rp);
    command.in.set_command(EVAL);
    command.in.clear_arg();
    command.in.push_arg("return redis.pcall('x')");
    command.in.push_arg("0");
    // "Unknown Redis command called from Lua script"
    VERIFY_MSG(!rp.exec_command(&command), rp);

    command.in.clear_arg();
    command.in.push_arg("return redis.call('x')");
    command.in.push_arg("0");
    // "ERR Error running script (call to f_bc800b29f7eb19fc5ef9364cf4afd4be5646137c):
    // Unknown Redis command called from Lua script"
    VERIFY_MSG(!rp.exec_command(&command), rp);

    command.in.clear_arg();
    command.in.push_arg("return {1,2,{3,'Hello World!',{1,{2},{1}}}}");
    command.in.push_arg("0");
    VERIFY_MSG(rp.exec_command(&command), rp);

    command.in.set_command(SCRIPT);
    command.in.clear_arg();
    command.in.push_arg("LOAD");
    command.in.push_arg("return redis.pcall('info')");
    VERIFY_MSG(rp.exec_command(&command), rp);
    VERIFY(command.out.get_bulk(&bulk));

    command.in.set_command(EVALSHA);
    command.in.clear_arg();
    command.in.push_arg(bulk);
    command.in.push_arg("0");
    VERIFY_MSG(rp.exec_command(&command), rp);


    // other
    VERIFY_MSG(rp.assure_connect(NULL), rp);
    VERIFY_MSG(rp.exec_command(&command, "ECHO 12345"), rp);
    VERIFY_MSG(rp.exec_command(&command, "QUIT"), rp);
    rp.close();
    VERIFY_MSG(rp.assure_connect(NULL), rp);
    VERIFY_MSG(rp.exec_command(&command, "CONFIG get pidfile"), rp);
    VERIFY_MSG(rp.exec_command(&command, "CONFIG get loglevel"), rp);
    VERIFY_MSG(rp.exec_command(&command, "config set loglevel notice"), rp);

    VERIFY_MSG(rp.exec_command(&command, "set string nothing"), rp);
    VERIFY_MSG(rp.exec_command(&command, "debug object string"), rp);
    VERIFY_MSG(rp.exec_command(&command, "debug reload"), rp);

    VERIFY_MSG(rp.exec_command(&command, "save"), rp);

    VERIFY_MSG(rp.exec_command(&command, "slowlog get"), rp);
    VERIFY_MSG(rp.exec_command(&command, "slowlog reset"), rp);
    VERIFY_MSG(rp.exec_command(&command, "slowlog len"), rp);

    VERIFY_MSG(rp.exec_command(&command, "TIME"), rp);

    cout << "protocol_test ok" << endl;
    return 0;
  }

  int basic_test(RedisBase2& r)
  {
    cout << "basic_test..." << endl;

    Redis2 * rs = dynamic_cast<Redis2 *>(&r);

    std::string s;
    int64_t i;
    double d;
    bool is_nil;
    mbulk_t mb;
    smbulk_t smb;
    string_vector_t keys, keys2, values, values2;

    ClearGuard<mbulk_t> mb_guard(&mb);
    ClearGuard<smbulk_t> smb_guard(&smb);

    VERIFY_MSG(r.flushall(), r);

    cout << "keys commands..." << endl;
    VERIFY_MSG(r.flushdb(), r);
    VERIFY_MSG(r.set("a", "a"), r);
    VERIFY_MSG(r.set("b", "b"), r);
    VERIFY_MSG(r.set("c", "c"), r);

    VERIFY_MSG(r.exists("a", &i), r);
    VERIFY(i == 1);
    VERIFY_MSG(r.exists("d", &i), r);
    VERIFY(i == 0);

    VERIFY_MSG(r.keys("*", &mb), r);
    VERIFY(mb.size() == 3);

    VERIFY_MSG(r.dump("c", &s, &is_nil), r);
    VERIFY(!is_nil);

    VERIFY_MSG(r.restore("d", 0, s), r);

    VERIFY_MSG(r.get("d", &s, &is_nil), r);
    VERIFY(s == "c");
    VERIFY(!is_nil);

    VERIFY_MSG(r.del("a", &i), r);
    VERIFY(i == 1);

    keys += "b", "c";
    VERIFY_MSG(r.del(keys, &i), r);
    VERIFY(i == 2);

    VERIFY_MSG(r.flushdb(), r);
    VERIFY_MSG(r.set("a", "a"), r);
    VERIFY_MSG(r.set("b", "b"), r);
    VERIFY_MSG(r.set("c", "c"), r);

    VERIFY_MSG(r.expire("a", 10, &i), r);
    VERIFY(i == 1);
    VERIFY_MSG(r.expireat("b", time(0)+10, &i), r);
    VERIFY(i == 1);
    VERIFY_MSG(r.expireat("d", 10, &i), r);
    VERIFY(i == 0);

    VERIFY_MSG(r.persist("b", &i), r);
    VERIFY(i == 1);
    VERIFY_MSG(r.persist("c", &i), r);
    VERIFY(i == 0);
    VERIFY_MSG(r.persist("d", &i), r);
    VERIFY(i == 0);

    if (rs)
    {
      VERIFY_MSG(r.flushdb(), r);
      VERIFY_MSG(r.set("a", "a"), r);
      VERIFY_MSG(r.set("b", "b"), r);
      VERIFY_MSG(r.set("c", "c"), r);

      VERIFY_MSG(rs->rename("a", "d"), *rs);
      VERIFY_MSG(rs->renamenx("b", "d", &i), *rs);
      VERIFY(i == 0);
      VERIFY_MSG(rs->renamenx("b", "e", &i), *rs);
      VERIFY(i == 1);
    }

    cout << "strings commands..." << endl;
    cout << "hashes commands..." << endl;
    cout << "lists commands..." << endl;
    cout << "sets commands..." << endl;
    cout << "sorted sets commands..." << endl;
    cout << "transactions commands..." << endl;
    cout << "connection commands..." << endl;
    cout << "server commands..." << endl;

    return 0;
  }
}

int main(int argc, char * argv[])
{
  try
  {
    namespace po = boost::program_options;
    po::options_description desc("Options");
    desc.add_options()
      ("help", "produce help message")
      ("host,h", po::value<std::string>()->default_value("localhost"), "redis host")
      ("port,p", po::value<std::string>()->default_value("6379"), "redis port")
      ("host_list", po::value<std::string>()->default_value("localhost"), "redis host list")
      ("port_list", po::value<std::string>()->default_value("6379"), "redis port list")
      ("db_index,i", po::value<int>()->default_value(5), "redis db index")
      ("timeout,t", po::value<int>()->default_value(2000), "timeout in ms");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
      cout << desc << endl;
      return 0;
    }

    host = vm["host"].as<std::string>();
    port = vm["port"].as<std::string>();
    host_list = vm["host_list"].as<std::string>();
    port_list = vm["port_list"].as<std::string>();
    db_index = vm["db_index"].as<int>();
    timeout = vm["timeout"].as<int>();
  }
  catch (std::exception& e)
  {
    cout << "caught: " << e.what() << endl;
    return 1;
  }

  // this is slow, do not test it
  // host_resolver_test();
  os_test();
  protocol_test();
  get_redis_version();

  {
    Redis2 r(host, port, db_index, timeout);
    basic_test(r);
  }

  {
    Redis2P r(host_list, port_list, db_index, timeout);
    basic_test(r);
  }

  //{
  //  RedisTss r(host, port, db_index, 1, timeout, kNormal);
  //  redis_tss_test(r);
  //}

  DUMP_TEST_RESULT();

  return 0;
}
