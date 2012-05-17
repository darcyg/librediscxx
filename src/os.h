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

LIBREDIS_NAMESPACE_BEGIN

int get_thread_id();

std::string get_host_name();

LIBREDIS_NAMESPACE_END

#endif //_LANGTAOJIN_LIBREDIS_OS_H_
