/** @file
* @brief os adapter
* @author yafei.zhang@langtaojin.com
* @date
* @version
* inner header
*/
#ifndef _LANGTAOJIN_LIBREDIS_OS_H_
#define _LANGTAOJIN_LIBREDIS_OS_H_

#include "redis_common.h"

NAMESPACE_BEGIN

int get_thread_id();

std::string get_host_name();

NAMESPACE_END

#endif //_LANGTAOJIN_LIBREDIS_OS_H_
