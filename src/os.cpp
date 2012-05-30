/** @file
 * @brief os adapter
 * @author yafei.zhang@langtaojin.com
 * @date
 * @version
 *
 */
#include "os.h"
#if defined _WIN32
# define WIN32_LEAN_AND_MEAN
# include <Windows.h>
# include <WinSock2.h>
# pragma comment(lib, "ws2_32.lib")
#else
# include <sys/syscall.h>
# include <unistd.h>
#endif

LIBREDIS_NAMESPACE_BEGIN

#if defined _WIN32
int get_thread_id()
{
  return (int)GetCurrentThreadId();
}
#else
int get_thread_id()
{
  return (int)syscall(SYS_gettid);
}
#endif


std::string get_host_name()
{
#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif
  // POSIX.1-2001 guarantees that "Host names
  // (not including the terminating null byte) are limited to HOST_NAME_MAX bytes
  // SUSv2 guarantees that "Host names are limited to 255 bytes".
  char buf[HOST_NAME_MAX+1];
  gethostname(buf, sizeof(buf));
  return std::string(buf);
}

LIBREDIS_NAMESPACE_END
