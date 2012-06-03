/** @file
 * @brief os basic functions
 * @author yafei.zhang@langtaojin.com
 * @date
 * @version
 *
 */
#include "os.h"
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <string.h>

LIBREDIS_NAMESPACE_BEGIN

int get_thread_id()
{
  return (int)syscall(SYS_gettid);
}

std::string get_host_name()
{
#ifndef HOST_NAME_MAX
# define HOST_NAME_MAX 255
#endif
  char buf[HOST_NAME_MAX+1];
  gethostname(buf, sizeof(buf));
  return std::string(buf);
}

std::string ec_2_string(int ec)
{
  char buf[64];
  return std::string(strerror_r(ec, buf, sizeof(buf)));
}

int available_bytes(int fd)
{
  int value = 0;
  int ret = ioctl(fd, FIONREAD, &value);

  if (ret==-1)
    return -1;

  return value;
}

int set_nonblock(int fd)
{
  int flags;

  if ((flags = fcntl(fd, F_GETFL))==-1)
    return -1;

  if ((flags & O_NONBLOCK)==0)
  {
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK)==-1)
      return -1;
  }

  return 0;
}

int set_block(int fd)
{
  int flags;

  if ((flags = fcntl(fd, F_GETFL))==-1)
    return -1;

  if ((flags & O_NONBLOCK)==1)
  {
    if (fcntl(fd, F_SETFL, flags & (~O_NONBLOCK))==-1)
      return -1;
  }

  return 0;
}

int is_open_slow(int fd)
{
  char c;
  int bytes;

  do bytes = recv(fd, &c, sizeof(c), MSG_PEEK|MSG_DONTWAIT);
  while (bytes==-1 && errno==EINTR);

  if (bytes==0
      || (bytes==-1 && errno!=EAGAIN && errno!=EWOULDBLOCK))
    return 0;

  return 1;
}

int is_open_fast(int fd)
{
  if (fd==-1)
    return 0;
  return 1;
}

int poll_read(int fd, struct timeval * timeout)
{
  fd_set fs_read;
  int ret;

  FD_ZERO(&fs_read);
  FD_SET(fd, &fs_read);

  do ret = select(fd+1, &fs_read, 0, 0, timeout);
  while (ret==-1 && errno==EINTR);
  if (ret<0)
  {
    return -1;
  }
  else if (ret==0)
  {
    errno = ETIMEDOUT;
    return 0;
  }
  else
  {
    return 1;
  }
}

int poll_write(int fd, struct timeval * timeout)
{
  fd_set fs_write;
  int ret;

  FD_ZERO(&fs_write);
  FD_SET(fd, &fs_write);

  do ret = select(fd+1, 0, &fs_write, 0, timeout);
  while (ret==-1 && errno==EINTR);
  if (ret<0)
  {
    return -1;
  }
  else if (ret==0)
  {
    errno = ETIMEDOUT;
    return 0;
  }
  else
  {
    return 1;
  }
}

int timed_connect(int fd,
    const struct sockaddr * addr,
    socklen_t addrlen, struct timeval * timeout)
{
  fd_set fs_read, fs_write;
  int ret;

  if (set_nonblock(fd)==-1)
    return -1;

  do ret = connect(fd, addr, addrlen);
  while (ret==-1 && errno==EINTR);
  if (ret==0)
    return 0;

  if (ret==-1 && errno!=EINPROGRESS)
    return -1;

  FD_ZERO(&fs_read);
  FD_SET(fd, &fs_read);
  FD_ZERO(&fs_write);
  FD_SET(fd, &fs_write);

  do ret = select(fd+1, &fs_read, &fs_write, 0, timeout);
  while (ret==-1 && errno==EINTR);
  if (ret<0)
  {
    return -1;
  }
  else if (ret==0)
  {
    errno = ETIMEDOUT;
    return -1;
  }
  else
  {
    int err;
    socklen_t error_len = sizeof(err);
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&err, &error_len)==-1)
      return -1;

    if (err!=0)
    {
      errno = err;
      return -1;
    }

    return 0;
  }
}

int timed_read(int fd, void * buf, size_t len, int flags, struct timeval * timeout)
{
  if (poll_read(fd, timeout)!=1)
    return -1;

  return recv(fd, buf, len, flags);
}

int timed_readn(int fd, void * cbuf, size_t len, int flags, struct timeval * timeout)
{
  int left;
  int nread;
  char * buf = (char *)cbuf;
  struct timeval _timeout;

  left = len;

  while (left>0)
  {
    _timeout = *timeout;/* make a copy */
    if (poll_read(fd, &_timeout)!=1)
      break;

    nread = recv(fd, buf, left, flags);
    if (nread<0)
    {
      if (errno==EINTR)
        nread = 0;/* call recv() again */
      else
        return -1;
    }
    else if (nread==0)
    {
      /* EOF */
      errno = 0;
      break;
    }

    left -= nread;
    buf += nread;
  }

  return len - left;
}

int timed_writen(int fd, const void * cbuf, size_t len, int flags, struct timeval * timeout)
{
  int left;
  int nwrite;
  const char * buf = (const char *)cbuf;
  struct timeval _timeout;

  left = len;

  while (left>0)
  {
    _timeout = *timeout;/* make a copy */
    if (poll_write(fd, &_timeout)!=1)
      break;

    nwrite = send(fd, buf, left, flags);
    if (nwrite<0)
    {
      if (errno==EINTR)
        nwrite = 0;/* call send() again */
      else
        return -1;
    }
    else if (nwrite==0)
    {
      /* EOF */
      errno = 0;
      break;
    }

    left -= nwrite;
    buf += nwrite;
  }

  return len - left;
}

int safe_close(int fd)
{
  int ret;
  do ret = close(fd);
  while (ret==-1 && errno==EINTR);
  return ret;
}

int resolve_host(const char * host, const char * service, struct net_endpoint * ep)
{
  struct addrinfo hints;
  struct addrinfo * result;
  int ret;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = ep->domain;
  hints.ai_socktype = ep->type;
  hints.ai_protocol = ep->protocol;

  ret = getaddrinfo(host, service, &hints, &result);
  if (ret!=0)
  {
    if (ret==EAI_BADFLAGS)
      errno = EINVAL;
    else if (ret==EAI_FAMILY)
      errno = EPROTONOSUPPORT;/* or ESOCKTNOSUPPORT ? */
    else if (ret==EAI_SOCKTYPE)
      errno = ESOCKTNOSUPPORT;
    else if (ret!=EAI_SYSTEM)
      errno = EHOSTUNREACH;
    return -1;
  }

  /* only get one result, do not traverse by 'rp->ai_next' */
  if (result->ai_family==AF_INET)
  {
    ep->domain = AF_INET;
    ep->address.in4 = *(struct sockaddr_in *)result->ai_addr;
  }
  else if (result->ai_family==AF_INET6)
  {
    ep->domain = AF_INET6;
    ep->address.in6 = *(struct sockaddr_in6 *)result->ai_addr;
  }

  freeaddrinfo(result);
  return 0;
}

LIBREDIS_NAMESPACE_END
