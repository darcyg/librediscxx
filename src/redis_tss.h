/** @file
* @brief RedisTss: thread specific storage support,
*                  also can be used as a connection pool
* @author yafei.zhang@langtaojin.com
* @date
* @version
*
*/
#ifndef _LANGTAOJIN_LIBREDIS_REDIS_TSS_H_
#define _LANGTAOJIN_LIBREDIS_REDIS_TSS_H_

#include "redis_base.h"

NAMESPACE_BEGIN

enum kRedisClientType
{
  kNormal = 0,//Redis2
  kPartition//Redis2P
};


class RedisTss
{
private:
  class Impl;
  Impl * impl_;

public:
  RedisTss(const std::string& host, int port,
    int db_index = 0, int partitions = 1,
    int timeout_ms = 50, kRedisClientType type = kPartition,
    size_t pool_size = 30, size_t check_interval = 120);

  RedisTss(const std::string& host, const std::string& port,
    int db_index = 0, int partitions = 1,
    int timeout_ms = 50, kRedisClientType type = kPartition,
    size_t pool_size = 30, size_t check_interval = 120);

  ~RedisTss();

  //may throw, do not return NULL
  RedisBase2 * get();
};

NAMESPACE_END

#endif //_LANGTAOJIN_LIBREDIS_REDIS_TSS_H_
