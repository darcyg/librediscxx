/** @file
* @brief test
* @author yafei.zhang@langtaojin.com
* @date
* @version
*
*/
#include <namespace.h>
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

std::string host, port, host_list, port_list;
int db_index, timeout;

namespace
{
  size_t s_total = 0;
  size_t s_failed = 0;

#define VERIFY(exp)\
  do{\
  bool x = (exp);\
  s_total++;\
  if (!x)\
  {\
  s_failed++;\
  cout << __FILE__ << "(" << __LINE__ << "):\"" << #exp << "\" fails" << endl;\
  cout << __FUNCTION__ << " failed" << endl;\
  return 1;\
  }\
  }while(0)


#define VERIFY_MSG(exp, client)\
  do{\
  bool x = (exp);\
  s_total++;\
  if (!x)\
  {\
  s_failed++;\
  cout << __FILE__ << "(" << __LINE__ << "):\"" << #exp << "\" fails:" << client.last_c_error() << endl;\
  cout << __FUNCTION__ << " failed" << endl;\
  return 1;\
  }\
  }while(0)


#define DUMP_TEST_RESULT()\
  do\
  {\
  cout << "*************************" << endl;\
  cout << "Total Checking Points: " << s_total << endl;\
  cout << "Failure Checking Points: " << s_failed << endl;\
  cout << "*************************" << endl;\
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
//      "www.google.com",
//      "www.j.cn",
//      "www.baidu.com",
//      "www.qq.com",
//      "w.w.w",
      "redis1",
      "redis99",
      "sdl-redis16",
      "sdl-redis99",
    };

//    for (size_t i=0; i<sizeof(hosts)/sizeof(hosts[0]); i++)
//    {
//      cout << "resolving host: " << hosts[i] << endl;
//
//      iter = res.resolve_host(hosts[i], "80", ec);
//      if (iter != boost::asio::ip::tcp::resolver::iterator())
//      {
//        cout << "resolved host: " << hosts[i] << "=" << iter->endpoint() << endl;
//      }
//      else
//      {
//        cout << "resolve host: " << hosts[i] << " failed: " << ec.message() << endl;
//      }
//    }

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

    RedisCommand command;
    RedisProtocol rp(host, port, timeout);
    VERIFY_MSG(rp.assure_connect(NULL), rp);
    VERIFY_MSG(rp.exec_command(&command, "PING", 0), rp);

    //got a special multi bulk
    VERIFY_MSG(rp.assure_connect(NULL), rp);
    VERIFY_MSG(rp.exec_command(&command, "SUBSCRIBE %s", "a"), rp);
    VERIFY_MSG(rp.assure_connect(NULL), rp);
    VERIFY_MSG(rp.exec_command(&command, "UNSUBSCRIBE %s", "a"), rp);
    VERIFY_MSG(rp.assure_connect(NULL), rp);
    VERIFY_MSG(rp.exec_command(&command, "PSUBSCRIBE %s", "a"), rp);
    VERIFY_MSG(rp.assure_connect(NULL), rp);
    VERIFY_MSG(rp.exec_command(&command, "PUNSUBSCRIBE %s", "a"), rp);

    ////will block
    //VERIFY_MSG(rp.assure_connect(NULL), rp);
    //rp.set_blocking_mode(true);
    //VERIFY_MSG(rp.exec_command(&command, "UNSUBSCRIBE"), rp);
    //VERIFY_MSG(rp.assure_connect(NULL), rp);
    //rp.set_blocking_mode(true);
    //VERIFY_MSG(rp.exec_command(&command, "PUNSUBSCRIBE"), rp);

    //transaction
    VERIFY_MSG(rp.assure_connect(NULL), rp);
    VERIFY_MSG(rp.exec_command(&command, "SET a a"), rp);
    VERIFY_MSG(rp.exec_command(&command, "SET b b"), rp);
    VERIFY_MSG(rp.exec_command(&command, "SET c c"), rp);
    VERIFY_MSG(rp.exec_command(&command, "WATCH a"), rp);
    VERIFY_MSG(rp.exec_command(&command, "WATCH b"), rp);
    VERIFY_MSG(rp.exec_command(&command, "WATCH c"), rp);

    /************************************************************************/
    VERIFY_MSG(rp.exec_command(&command, "MULTI"), rp);
    VERIFY(!rp.exec_command(&command, "WATCH a"));//got an error
    cout << rp.last_error() << endl;
    VERIFY_MSG(rp.exec_command(&command, "UNWATCH"), rp);
    VERIFY_MSG(rp.exec_command(&command, "DISCARD"), rp);
    /************************************************************************/

    VERIFY_MSG(rp.exec_command(&command, "WATCH a"), rp);
    VERIFY_MSG(rp.exec_command(&command, "WATCH b"), rp);
    VERIFY_MSG(rp.exec_command(&command, "WATCH c"), rp);
    VERIFY_MSG(rp.exec_command(&command, "UNWATCH"), rp);

    /************************************************************************/
    VERIFY_MSG(rp.exec_command(&command, "MULTI"), rp);
    VERIFY_MSG(rp.exec_command(&command, "GET a"), rp);
    VERIFY_MSG(rp.exec_command(&command, "GET b"), rp);
    VERIFY_MSG(rp.exec_command(&command, "GET c"), rp);
    VERIFY_MSG(rp.exec_command(&command, "KEYS *"), rp);
    VERIFY_MSG(rp.exec_command(&command, "INFO"), rp);
    //got a special multi bulk
    VERIFY_MSG(rp.exec_command(&command, "EXEC"), rp);
    /************************************************************************/

    cout << "protocol_test ok" << endl;
    return 0;
  }


  int basic_test(RedisBase2& r)
  {
    cout << "basic_test..." << endl;

    std::string s;
    int64_t i;
    double d;
    bool is_nil;
    mbulk_t mb;
    smbulk_t smb;
    string_vector_t keys, keys2, values, values2;

    ClearGuard<mbulk_t> mb_guard(&mb);
    ClearGuard<smbulk_t> smb_guard(&smb);

    /************************************************************************/
    VERIFY_MSG(r.flushall(), r);

    VERIFY_MSG(r.set("a", "1"), r);
    VERIFY_MSG(r.set("b", "2"), r);
    VERIFY_MSG(r.set("c", "3"), r);
    VERIFY_MSG(r.get("z", &s, &is_nil), r);
    VERIFY(is_nil);
    VERIFY_MSG(r.get("a", &s, &is_nil), r);
    VERIFY(!is_nil && s == "1");

    VERIFY_MSG(r.exists("a", &i), r);
    VERIFY(i == 1);
    VERIFY_MSG(r.exists("z", &i), r);
    VERIFY(i == 0);

    VERIFY_MSG(r.expire("a", 60, &i), r);
    VERIFY(i == 1);
    VERIFY_MSG(r.expire("z", 60, &i), r);
    VERIFY(i == 0);
    VERIFY_MSG(r.expireat("a", time(0) + 60, &i), r);
    VERIFY(i == 1);
    VERIFY_MSG(r.expireat("z", time(0) + 60, &i), r);
    VERIFY(i == 0);

    VERIFY_MSG(r.set("e1", "1"), r);
    VERIFY_MSG(r.set("e2", "2"), r);
    VERIFY_MSG(r.set("e3", "3"), r);
    VERIFY_MSG(r.set("e4", "4"), r);
    VERIFY_MSG(r.set("e5", "5"), r);
    keys2.clear();
    keys2 += "e1","e2","e3","e4","e5";
    VERIFY_MSG(r.keys("e*", &mb), r);
    convert(&mb, &keys);
    std::sort(keys.begin(), keys.end());
    std::sort(keys2.begin(), keys2.end());
    VERIFY(keys == keys2);

    values.resize(keys.size(), "1");
    VERIFY_MSG(r.mset(keys, values), r);
    VERIFY_MSG(r.mget(keys, &mb), r);
    convert(&mb, &values2);
    VERIFY(values == values2);

    VERIFY_MSG(r.move("a", 1, &i), r);
    VERIFY(i == 1);
    VERIFY_MSG(r.move("b", 2, &i), r);
    VERIFY(i == 1);
    VERIFY_MSG(r.move("c", 3, &i), r);
    VERIFY(i == 1);
    VERIFY_MSG(r.move("z", 4, &i), r);
    VERIFY(i == 0);

    /************************************************************************/
    VERIFY_MSG(r.flushall(), r);

    VERIFY_MSG(r.set("a", "a"), r);
    VERIFY_MSG(r.hset("b", "b", "b", &i), r);
    VERIFY(i == 1);
    VERIFY_MSG(r.lpush("c", "c", &i), r);
    VERIFY(i == 1);
    VERIFY_MSG(r.sadd("d", "d", &i), r);
    VERIFY(i == 1);
    VERIFY_MSG(r.zadd("e", 1.0, "e", &i), r);
    VERIFY(i == 1);

    VERIFY_MSG(r.object_refcount("a", &i), r);
    VERIFY_MSG(r.object_refcount("b", &i), r);
    VERIFY_MSG(r.object_encoding("a", &s), r);
    cout << s << endl;
    VERIFY_MSG(r.object_encoding("b", &s), r);
    cout << s << endl;
    VERIFY_MSG(r.object_encoding("c", &s), r);
    cout << s << endl;
    VERIFY_MSG(r.object_encoding("d", &s), r);
    cout << s << endl;
    VERIFY_MSG(r.object_idletime("a", &i), r);
    VERIFY_MSG(r.object_idletime("b", &i), r);

    VERIFY_MSG(r.expire("a", 60, &i), r);
    VERIFY(i == 1);

    VERIFY_MSG(r.persist("a", &i), r);
    VERIFY(i == 1);
    VERIFY_MSG(r.persist("b", &i), r);
    VERIFY(i == 0);

    VERIFY_MSG(r.randomkey(&s, &is_nil), r);

    VERIFY_MSG(r.sort("c", NULL, &mb), r);
    VERIFY_MSG(r.sort("d", NULL, &mb), r);
    VERIFY_MSG(r.sort("e", NULL, &mb), r);

    VERIFY_MSG(r.ttl("a", &i), r);
    VERIFY_MSG(r.type("a", &s), r);
    cout << s << endl;
    VERIFY_MSG(r.type("b", &s), r);
    cout << s << endl;
    VERIFY_MSG(r.type("c", &s), r);
    cout << s << endl;
    VERIFY_MSG(r.type("d", &s), r);
    cout << s << endl;
    VERIFY_MSG(r.type("e", &s), r);
    cout << s << endl;
    VERIFY_MSG(r.type("f", &s), r);
    cout << s << endl;

    VERIFY_MSG(r.append("string", "Hello", &i), r);
    VERIFY(i == 5);
    VERIFY_MSG(r.append("string", " World", &i), r);
    VERIFY(i == 11);

    VERIFY_MSG(r.decr("number", &i), r);
    VERIFY(i == -1);
    VERIFY_MSG(r.decrby("number", 10, &i), r);
    VERIFY(i == -11);
    VERIFY_MSG(r.incr("number", &i), r);
    VERIFY(i == -10);
    VERIFY_MSG(r.incrby("number", 2, &i), r);
    VERIFY(i == -8);

    VERIFY_MSG(r.setbit("number", 0, 1, &i), r);
    VERIFY_MSG(r.getbit("number", 0, &i), r);
    VERIFY(i == 1);

    VERIFY_MSG(r.getrange("number", 0, -1, &s, &is_nil), r);
    VERIFY_MSG(r.getrange("string", 0, -1, &s, &is_nil), r);
    VERIFY(!is_nil && s == "Hello World");
    VERIFY_MSG(r.getrange("string", 1, 2, &s, &is_nil), r);
    VERIFY(!is_nil && s == "el");

    VERIFY_MSG(r.getset("string", "xx", &s, &is_nil), r);
    VERIFY(!is_nil && s == "Hello World");
    VERIFY_MSG(r.getset("string2", "xx", &s, &is_nil), r);
    VERIFY(is_nil);

    VERIFY_MSG(r.setex("string2", 60, "xx"), r);

    VERIFY_MSG(r.setnx("string2", "xx", &i), r);
    VERIFY(i == 0);
    VERIFY_MSG(r.setnx("string3", "xx", &i), r);
    VERIFY(i == 1);

    VERIFY_MSG(r.setrange("string3", 2, "yyzz", &i), r);
    VERIFY(i == 6);

    VERIFY_MSG(r.strlen("string2", &i), r);
    VERIFY(i == 2);
    VERIFY_MSG(r.strlen("string4", &i), r);
    VERIFY(i == 0);

    /************************************************************************/
    VERIFY_MSG(r.flushall(), r);

    VERIFY_MSG(r.hset("h1", "f1", "1"), r);
    VERIFY_MSG(r.hset("h1", "f2", "2"), r);
    VERIFY_MSG(r.hsetnx("h1", "f3", "3", &i), r);
    VERIFY(i == 1);
    VERIFY_MSG(r.hsetnx("h1", "f3", "3", &i), r);
    VERIFY(i == 0);

    VERIFY_MSG(r.hset("h2", "1", "1"), r);
    VERIFY_MSG(r.hset("h2", "a", "a"), r);

    VERIFY_MSG(r.hget("h1", "f1", &s, &is_nil), r);
    VERIFY(!is_nil && s == "1");
    VERIFY_MSG(r.hget("h1", "f2", &s, &is_nil), r);
    VERIFY(!is_nil && s == "2");
    VERIFY_MSG(r.hget("h1", "f3", &s, &is_nil), r);
    VERIFY(!is_nil && s == "3");
    VERIFY_MSG(r.hget("h1", "z", &s, &is_nil), r);
    VERIFY(is_nil);
    VERIFY_MSG(r.hget("z", "z", &s, &is_nil), r);
    VERIFY(is_nil);

    VERIFY_MSG(r.hgetall("h1", &mb), r);
    VERIFY(mb.size() == 6);
    VERIFY_MSG(r.hgetall("z", &mb), r);
    VERIFY(mb.empty());

    VERIFY_MSG(r.hdel("h1", "f1", &i), r);
    VERIFY(i == 1);
    VERIFY_MSG(r.hdel("h1", "z", &i), r);
    VERIFY(i == 0);

    keys.clear();
    keys += "f2","f3","z";
    VERIFY_MSG(r.hdel("h1", keys, &i), r);
    VERIFY(i == 2);

    VERIFY_MSG(r.hincr("h2", "1", &i), r);
    VERIFY(i == 2);
    VERIFY_MSG(r.hincrby("h2", "1", 10, &i), r);
    VERIFY(i == 12);
    VERIFY_MSG(!r.hincr("h2", "a", &i), r);//got an error
    cout << r.last_error() << endl;

    keys.clear();
    keys += "1","2","3";
    values.clear();
    values += "1","2","3";
    VERIFY_MSG(r.hmset("h3", keys, values), r);
    VERIFY_MSG(r.hmget("h3", keys, &mb), r);
    convert(&mb, &values2);
    VERIFY(values == values2);

    VERIFY_MSG(r.hexists("h3", "1", &i), r);
    VERIFY(i == 1);
    VERIFY_MSG(r.hexists("h3", "z", &i), r);
    VERIFY(i == 0);
    VERIFY_MSG(r.hexists("z", "z", &i), r);
    VERIFY(i == 0);

    VERIFY_MSG(r.hlen("h3", &i), r);
    VERIFY(i == 3);

    VERIFY_MSG(r.hkeys("h3", &mb), r);
    VERIFY_MSG(r.hvals("h3", &mb), r);

    /************************************************************************/
    VERIFY_MSG(r.flushall(), r);

    VERIFY_MSG(r.lpush("l", "4", &i), r);
    VERIFY(i == 1);
    VERIFY_MSG(r.lpush("l", "3", &i), r);
    VERIFY(i == 2);
    VERIFY_MSG(r.lpush("l", "2", &i), r);
    VERIFY(i == 3);
    VERIFY_MSG(r.lpush("l", "1", &i), r);
    VERIFY(i == 4);
    VERIFY_MSG(r.rpush("l", "5", &i), r);
    VERIFY(i == 5);
    VERIFY_MSG(r.rpush("l", "6", &i), r);
    VERIFY(i == 6);

    VERIFY_MSG(r.llen("l", &i), r);
    VERIFY(i == 6);

    VERIFY_MSG(r.lindex("l", 0, &s, &is_nil), r);
    VERIFY(!is_nil && s == "1");
    VERIFY_MSG(r.lindex("l", -1, &s, &is_nil), r);
    VERIFY(!is_nil && s == "6");

    values.clear();
    values += "1","2","3","4","5","6";
    VERIFY_MSG(r.lrange("l", 0, -1, &mb), r);
    convert(&mb, &values2);
    VERIFY(values == values2);

    VERIFY_MSG(r.lpop("l", &s, &is_nil), r);
    VERIFY(!is_nil && s == "1");
    VERIFY_MSG(r.lpop("z", &s, &is_nil), r);
    VERIFY(is_nil);
    VERIFY_MSG(r.rpop("l", &s, &is_nil), r);
    VERIFY(!is_nil && s == "6");
    VERIFY_MSG(r.rpop("z", &s, &is_nil), r);
    VERIFY(is_nil);

    VERIFY_MSG(r.lpushx("l", "1", &i), r);
    VERIFY(i == 5);
    VERIFY_MSG(r.rpushx("l", "6", &i), r);
    VERIFY(i == 6);
    VERIFY_MSG(r.lpushx("z", "1", &i), r);
    VERIFY(i == 0);
    VERIFY_MSG(r.rpushx("z", "6", &i), r);
    VERIFY(i == 0);

    VERIFY_MSG(r.ltrim("l", 1, -1), r);

    VERIFY_MSG(r.lpush("l", "2", &i), r);
    VERIFY(i == 6);
    VERIFY_MSG(r.lset("l", 0, "1"), r);

    VERIFY_MSG(r.linsert("l", true, "2", "1", &i), r);
    VERIFY(i == 7);
    VERIFY_MSG(r.linsert("l", false, "2", "3", &i), r);
    VERIFY(i == 8);

    VERIFY_MSG(r.lrem("l", 0, "1", &i), r);
    VERIFY(i == 2);
    VERIFY_MSG(r.lrem("l", 0, "2", &i), r);
    VERIFY(i == 1);
    VERIFY_MSG(r.lrem("l", 0, "3", &i), r);
    VERIFY(i == 2);

    /************************************************************************/
    VERIFY_MSG(r.flushall(), r);

    VERIFY_MSG(r.sadd("s", "1", &i), r);
    VERIFY(i == 1);
    VERIFY_MSG(r.sadd("s", "2", &i), r);
    VERIFY(i == 1);
    VERIFY_MSG(r.sadd("s", "3", &i), r);
    VERIFY(i == 1);
    VERIFY_MSG(r.sadd("s", "4", &i), r);
    VERIFY(i == 1);

    VERIFY_MSG(r.scard("s", &i), r);
    VERIFY(i == 4);

    VERIFY_MSG(r.sismember("s", "1", &i), r);
    VERIFY(i == 1);
    VERIFY_MSG(r.sismember("s", "z", &i), r);
    VERIFY(i == 0);
    VERIFY_MSG(r.sismember("z", "z", &i), r);
    VERIFY(i == 0);

    values.clear();
    values += "1","2","3","4";
    VERIFY_MSG(r.smembers("s", &mb), r);
    convert(&mb, &values2);
    std::sort(values2.begin(), values2.end());
    VERIFY(values == values2);

    VERIFY_MSG(r.spop("s", &s, &is_nil), r);
    VERIFY(!is_nil);
    VERIFY_MSG(r.spop("z", &s, &is_nil), r);
    VERIFY(is_nil);

    VERIFY_MSG(r.srandmember("s", &s, &is_nil), r);
    VERIFY(!is_nil);
    VERIFY_MSG(r.srandmember("z", &s, &is_nil), r);
    VERIFY(is_nil);

    VERIFY_MSG(r.srem("s", "1", &i), r);
    VERIFY_MSG(r.srem("s", "2", &i), r);
    VERIFY_MSG(r.srem("s", "3", &i), r);
    VERIFY_MSG(r.srem("s", "4", &i), r);

    /************************************************************************/
    VERIFY_MSG(r.flushall(), r);

    VERIFY_MSG(r.zadd("z", 1.0, "1", &i), r);
    VERIFY(i == 1);
    VERIFY_MSG(r.zadd("z", 2.0, "2", &i), r);
    VERIFY(i == 1);
    VERIFY_MSG(r.zadd("z", 3.0, "3", &i), r);
    VERIFY(i == 1);
    VERIFY_MSG(r.zadd("z", 4.0, "4", &i), r);
    VERIFY(i == 1);

    VERIFY_MSG(r.zcard("z", &i), r);
    VERIFY(i == 4);

    VERIFY_MSG(r.zcount("z", "-inf", "+inf", &i), r);
    VERIFY(i == 4);
    VERIFY_MSG(r.zcount("z", "1.0", "4.0", &i), r);
    VERIFY(i == 4);
    VERIFY_MSG(r.zcount("z", "(1.0", "(4.0", &i), r);
    VERIFY(i == 2);

    VERIFY_MSG(r.zscore("z", "1", &d, &is_nil), r);
    VERIFY(!is_nil && fabs(d - 1.0) < 0.0001);
    VERIFY_MSG(r.zscore("z", "zz", &d, &is_nil), r);
    VERIFY(is_nil);
    VERIFY_MSG(r.zscore("zz", "1", &d, &is_nil), r);
    VERIFY(is_nil);

    VERIFY_MSG(r.zadd("z", 4.0, "5", &i), r);
    VERIFY(i == 1);
    VERIFY_MSG(r.zincrby("z", 1.0, "5", &d), r);
    VERIFY(fabs(d - 5.0) < 0.0001);

    VERIFY_MSG(r.zrange("z", 0, -1, true, &mb), r);
    VERIFY_MSG(r.zrange("z", 0, -1, false, &mb), r);
    VERIFY_MSG(r.zrevrange("z", 0, -1, true, &mb), r);
    VERIFY_MSG(r.zrevrange("z", 0, -1, false, &mb), r);

    ZRangebyscoreLimit limit;
    limit.offset = 0;
    limit.count = 65536;
    VERIFY_MSG(r.zrangebyscore("z", "-inf", "+inf", true, &limit, &mb), r);
    VERIFY_MSG(r.zrangebyscore("z", "-inf", "+inf", true, NULL, &mb), r);
    VERIFY_MSG(r.zrangebyscore("z", "-inf", "+inf", false, &limit, &mb), r);
    VERIFY_MSG(r.zrangebyscore("z", "-inf", "+inf", false, NULL, &mb), r);
    VERIFY_MSG(r.zrevrangebyscore("z", "+inf", "-inf", true, &limit, &mb), r);
    VERIFY_MSG(r.zrevrangebyscore("z", "+inf", "-inf", true, NULL, &mb), r);
    VERIFY_MSG(r.zrevrangebyscore("z", "+inf", "-inf", false, &limit, &mb), r);
    VERIFY_MSG(r.zrevrangebyscore("z", "+inf", "-inf", false, NULL, &mb), r);

    VERIFY_MSG(r.zrank("z", "1", &i, &is_nil), r);
    VERIFY(!is_nil && i == 0);
    VERIFY_MSG(r.zrank("z", "2", &i, &is_nil), r);
    VERIFY(!is_nil && i == 1);
    VERIFY_MSG(r.zrank("z", "10", &i, &is_nil), r);
    VERIFY(is_nil);

    VERIFY_MSG(r.zrevrank("z", "1", &i, &is_nil), r);
    VERIFY(!is_nil && i == 4);
    VERIFY_MSG(r.zrevrank("z", "2", &i, &is_nil), r);
    VERIFY(!is_nil && i == 3);
    VERIFY_MSG(r.zrevrank("z", "10", &i, &is_nil), r);
    VERIFY(is_nil);

    VERIFY_MSG(r.zrem("z", "5", &i), r);
    VERIFY(i == 1);
    VERIFY_MSG(r.zrem("z", "10", &i), r);
    VERIFY(i == 0);
    VERIFY_MSG(r.zrem("zz", "zz", &i), r);
    VERIFY(i == 0);

    VERIFY_MSG(r.zremrangebyrank("z", 0, 1, &i), r);
    VERIFY(i == 2);

    VERIFY_MSG(r.zremrangebyscore("z", "3", "4", &i), r);
    VERIFY(i == 2);

    VERIFY_MSG(r.flushall(), r);

    cout << "basic_test ok" << endl;
    return 0;
  }


  int basic_single_test(RedisBase2Single& r)
  {
    cout << "basic_single_test..." << endl;

    std::string s;
    int64_t i;
    bool is_nil;
    mbulk_t mb;
    string_vector_t keys, values, values2;

    std::string key;
    std::string member;
    bool expired;

    ClearGuard<mbulk_t> mb_guard(&mb);

    /************************************************************************/
    VERIFY_MSG(r.flushall(), r);
    VERIFY_MSG(r.flushall(), r);

    VERIFY_MSG(r.set("a", "a"), r);
    VERIFY_MSG(r.set("b", "b"), r);
    VERIFY_MSG(r.rename("a", "b"), r);
    VERIFY_MSG(r.get("b", &s, &is_nil), r);
    VERIFY(!is_nil && s == "a");

    VERIFY_MSG(r.set("a", "a"), r);
    VERIFY_MSG(r.set("b", "b"), r);
    VERIFY_MSG(r.renamenx("a", "b", &i), r);
    VERIFY(i == 0);
    VERIFY_MSG(!r.renamenx("z", "a", &i), r);//got an error
    VERIFY_MSG(r.renamenx("a", "z", &i), r);
    VERIFY(i == 1);

    /************************************************************************/
    VERIFY_MSG(r.flushall(), r);

    VERIFY_MSG(r.lpush("a", "1", &i), r);
    VERIFY(i == 1);
    VERIFY_MSG(r.lpush("a", "2", &i), r);
    VERIFY(i == 2);
    VERIFY_MSG(r.lpush("b", "3", &i), r);
    VERIFY(i == 1);
    VERIFY_MSG(r.lpush("b", "4", &i), r);
    VERIFY(i == 2);
    VERIFY_MSG(r.rpush("c", "5", &i), r);
    VERIFY(i == 1);
    VERIFY_MSG(r.rpush("c", "6", &i), r);
    VERIFY(i == 2);

    keys.clear();
    keys += "a";
    VERIFY_MSG(r.blpop(keys, 1, &key, &member, &expired), r);
    VERIFY(!expired);
    keys.clear();
    keys += "d", "e", "f", "g";
    VERIFY_MSG(r.blpop(keys, 1, &key, &member, &expired), r);
    VERIFY(expired);

    keys.clear();
    keys += "a";
    VERIFY_MSG(r.brpop(keys, 1, &key, &member, &expired), r);
    VERIFY(!expired);
    keys.clear();
    keys += "d", "e", "f", "g";
    VERIFY_MSG(r.brpop(keys, 1, &key, &member, &expired), r);
    VERIFY(expired);

    VERIFY_MSG(r.brpoplpush("a", "b", 1, &key, &expired), r);
    VERIFY(expired);
    VERIFY_MSG(r.brpoplpush("b", "a", 1, &key, &expired), r);
    VERIFY(!expired);

    VERIFY_MSG(r.rpoplpush("a", "b", &s, &is_nil), r);
    VERIFY(!is_nil);
    VERIFY_MSG(r.rpoplpush("a", "b", &s, &is_nil), r);
    VERIFY(is_nil);

    /************************************************************************/
    VERIFY_MSG(r.flushall(), r);

    VERIFY_MSG(r.sadd("a", "1", &i), r);
    VERIFY_MSG(r.sadd("a", "2", &i), r);
    VERIFY_MSG(r.sadd("a", "3", &i), r);
    VERIFY_MSG(r.sadd("a", "4", &i), r);
    VERIFY_MSG(r.sadd("a", "5", &i), r);

    VERIFY_MSG(r.sadd("b", "1", &i), r);
    VERIFY_MSG(r.sadd("b", "3", &i), r);
    VERIFY_MSG(r.sadd("b", "5", &i), r);
    VERIFY_MSG(r.sadd("b", "7", &i), r);
    VERIFY_MSG(r.sadd("b", "9", &i), r);

    VERIFY_MSG(r.sadd("c", "2", &i), r);
    VERIFY_MSG(r.sadd("c", "4", &i), r);
    VERIFY_MSG(r.sadd("c", "6", &i), r);
    VERIFY_MSG(r.sadd("c", "8", &i), r);
    VERIFY_MSG(r.sadd("c", "10", &i), r);

    keys.clear();
    keys += "a","b";
    VERIFY_MSG(r.sdiff(keys, &mb), r);
    convert(&mb, &values);
    values2.clear();
    values2 += "2","4";
    VERIFY(values == values2);
    VERIFY_MSG(r.sdiffstore("d", keys, &i), r);
    VERIFY(i == 2);

    keys.clear();
    keys += "a","c";
    VERIFY_MSG(r.sdiff(keys, &mb), r);
    convert(&mb, &values);
    values2.clear();
    values2 += "1","3","5";
    VERIFY(values == values2);
    VERIFY_MSG(r.sdiffstore("e", keys, &i), r);
    VERIFY(i == 3);

    keys.clear();
    keys += "a","b";
    VERIFY_MSG(r.sinter(keys, &mb), r);
    convert(&mb, &values);
    values2.clear();
    values2 += "1","3","5";
    VERIFY(values == values2);
    VERIFY_MSG(r.sinterstore("f", keys, &i), r);
    VERIFY(i == 3);

    keys.clear();
    keys += "a","c";
    VERIFY_MSG(r.sinter(keys, &mb), r);
    convert(&mb, &values);
    values2.clear();
    values2 += "2","4";
    VERIFY(values == values2);
    VERIFY_MSG(r.sinterstore("g", keys, &i), r);
    VERIFY(i == 2);

    keys.clear();
    keys += "a","b";
    VERIFY_MSG(r.sunion(keys, &mb), r);
    convert(&mb, &values);
    values2.clear();
    values2 += "1","2","3","4","5","7","9";
    VERIFY(values == values2);
    VERIFY_MSG(r.sunionstore("h", keys, &i), r);
    VERIFY(i == 7);

    keys.clear();
    keys += "a","c";
    VERIFY_MSG(r.sunion(keys, &mb), r);
    convert(&mb, &values);
    values2.clear();
    values2 += "1","2","3","4","5","6","8","10";
    VERIFY(values == values2);
    VERIFY_MSG(r.sunionstore("i", keys, &i), r);
    VERIFY(i == 8);

    VERIFY_MSG(r.smove("a", "b", "1", &i), r);
    VERIFY(i == 1);
    VERIFY_MSG(r.smove("a", "b", "2", &i), r);
    VERIFY(i == 1);
    VERIFY_MSG(r.smove("a", "b", "-1", &i), r);
    VERIFY(i == 0);

    /************************************************************************/
    VERIFY_MSG(r.flushall(), r);

    VERIFY_MSG(r.zadd("a", 0, "1", &i), r);
    VERIFY_MSG(r.zadd("a", 0, "2", &i), r);
    VERIFY_MSG(r.zadd("a", 0, "3", &i), r);
    VERIFY_MSG(r.zadd("a", 0, "4", &i), r);
    VERIFY_MSG(r.zadd("a", 0, "5", &i), r);

    VERIFY_MSG(r.zadd("b", 0, "1", &i), r);
    VERIFY_MSG(r.zadd("b", 0, "3", &i), r);
    VERIFY_MSG(r.zadd("b", 0, "5", &i), r);
    VERIFY_MSG(r.zadd("b", 0, "7", &i), r);
    VERIFY_MSG(r.zadd("b", 0, "9", &i), r);

    std::vector<double> weights;
    weights.clear();
    weights += 2.0,1.0;

    keys.clear();
    keys += "a","b";

    VERIFY_MSG(r.zinterstore("c", keys, &weights, kSum, &i), r);
    VERIFY(i == 3);
    VERIFY_MSG(r.zinterstore("c", keys, &weights, kMin, &i), r);
    VERIFY(i == 3);
    VERIFY_MSG(r.zinterstore("c", keys, &weights, kMax, &i), r);
    VERIFY(i == 3);
    VERIFY_MSG(r.zinterstore("c", keys, NULL, kSum, &i), r);
    VERIFY(i == 3);
    VERIFY_MSG(r.zinterstore("c", keys, NULL, kMin, &i), r);
    VERIFY(i == 3);
    VERIFY_MSG(r.zinterstore("c", keys, NULL, kMax, &i), r);
    VERIFY(i == 3);

    keys.clear();
    keys += "a","b";
    VERIFY_MSG(r.zunionstore("c", keys, &weights, kSum, &i), r);
    VERIFY(i == 7);
    VERIFY_MSG(r.zunionstore("c", keys, &weights, kMin, &i), r);
    VERIFY(i == 7);
    VERIFY_MSG(r.zunionstore("c", keys, &weights, kMax, &i), r);
    VERIFY(i == 7);
    VERIFY_MSG(r.zunionstore("c", keys, NULL, kSum, &i), r);
    VERIFY(i == 7);
    VERIFY_MSG(r.zunionstore("c", keys, NULL, kMin, &i), r);
    VERIFY(i == 7);
    VERIFY_MSG(r.zunionstore("c", keys, NULL, kMax, &i), r);
    VERIFY(i == 7);

    VERIFY_MSG(r.flushall(), r);

    cout << "basic_single_test ok" << endl;
    return 0;
  }


  int pipeline_test(RedisBase2Single& r)
  {
    cout << "pipeline_test..." << endl;

    VERIFY_MSG(r.flushall(), r);
    VERIFY_MSG(r.set("a", "a"), r);

    const int iteration = 1000;

    //non pipeline
    {
      std::string value;
      RedisCommand c(GET);
      c.push_arg("a");

      boost::posix_time::ptime begin, end;
      begin = boost::posix_time::microsec_clock::local_time();

      for (int i=0; i<iteration; i++)
      {
        VERIFY_MSG(r.exec_command(&c), r);
        VERIFY(c.out.get_bulk(&value));
      }

      end = boost::posix_time::microsec_clock::local_time();
      boost::posix_time::time_duration td = end - begin;
      cout << iteration << " iterations(non-pipeline) cost "
        << td.total_milliseconds() << " ms" << endl;
    }

    //pipeline
    {
      std::string value;
      redis_command_vector_t cmds;
      RedisCommand c(GET);
      c.push_arg("a");

      for (int i=0; i<iteration; i++)
      {
        cmds.push_back(&c);
      }

      boost::posix_time::ptime begin, end;
      begin = boost::posix_time::microsec_clock::local_time();

      VERIFY_MSG(r.exec_pipeline(&cmds), r);

      end = boost::posix_time::microsec_clock::local_time();
      boost::posix_time::time_duration td = end - begin;

      cout << iteration << " iterations(pipeline) cost "
        << td.total_milliseconds() << " ms" << endl;
    }

    cout << "pipeline_test ok" << endl;
    return 0;
  }


  int transaction_test(RedisBase2Single& r)
  {
    cout << "transaction_test..." << endl;

    VERIFY_MSG(r.flushall(), r);
    VERIFY_MSG(r.multi(), r);

    RedisCommand c;
    redis_command_vector_t cmdv;

    VERIFY_MSG(r.add_command(&c, "SET a a"), r);
    VERIFY_MSG(r.add_command(&c, "SET b b"), r);
    VERIFY_MSG(r.add_command(&c, "SET c c"), r);
    VERIFY_MSG(r.add_command(&c, "GET a"), r);
    VERIFY_MSG(r.add_command(&c, "GET b"), r);
    VERIFY_MSG(r.add_command(&c, "GET c"), r);
    VERIFY_MSG(r.exec(&cmdv), r);

    VERIFY(cmdv.size() == 6);
    VERIFY(cmdv[0]->out.is_status_ok());
    VERIFY(cmdv[1]->out.is_status_ok());
    VERIFY(cmdv[2]->out.is_status_ok());
    VERIFY(*cmdv[3]->out.ptr.bulk == "a");
    VERIFY(*cmdv[4]->out.ptr.bulk == "b");
    VERIFY(*cmdv[5]->out.ptr.bulk == "c");

    clear_commands(&cmdv);
    cout << "transaction_test ok" << endl;
    return 0;
  }


  int redis_tss_test_thread(RedisTss * r)
  {
    RedisBase2 * redis_handle = r->get();
    assert(redis_handle);
    redis_handle->set("a", "b");
    return 0;
  }


  int redis_tss_test(RedisTss& r)
  {
    cout << "redis_tss_test..." << endl;

    const int iteration = 100;
    const int thread_number = 10;

    for (int i=0; i<iteration; i++)
    {
      boost::thread_group tg;
      for (int j=0; j<thread_number; j++)
      {
        tg.create_thread(boost::bind(redis_tss_test_thread, &r));
      }
      tg.join_all();
    }

    cout << "redis_tss_test ok" << endl;
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

  host_resolver_test();
//  os_test();
//  protocol_test();
//  get_redis_version();
//
//  {
//    Redis2 r(host, port, db_index, timeout);
//    basic_test(r);
//    basic_single_test(r);
//    pipeline_test(r);
//    transaction_test(r);
//  }
//
//  {
//    Redis2P r(host_list, port_list, db_index, timeout);
//    basic_test(r);
//  }
//
//  {
//    RedisTss r(host, port, db_index, 1, timeout, kNormal);
//    redis_tss_test(r);
//  }

  DUMP_TEST_RESULT();

  return 0;
}
