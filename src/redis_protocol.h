/** @file
* @brief redis protocol operation
* @author yafei.zhang@langtaojin.com
* @date
* @version
*
*/
#ifndef _LANGTAOJIN_LIBREDIS_REDIS_PROTOCOL_H_
#define _LANGTAOJIN_LIBREDIS_REDIS_PROTOCOL_H_

#include "redis_cmd.h"

NAMESPACE_BEGIN

class TcpClient;

class RedisProtocol
{
public:
  //timeout_ms is used for TCP connecting, sending and receiving
  RedisProtocol(const std::string& host, const std::string& port, int timeout_ms);
  ~RedisProtocol();

  //status is optional
  //*status == 0, if it is open
  //*status == 1, if it is not open and connected
  //*status == -1, if it is not open and not connected
  bool assure_connect(int * status);
  bool connect();
  void close();
  bool available()const;
  bool is_open()const;
  bool check_connect();

  std::string get_host()const
  {
    return host_;
  }

  std::string get_port()const
  {
    return port_;
  }

  void last_error(const std::string& err)
  {
    error_ = err;
  }

  std::string last_error()const
  {
    return error_;
  }

  const char * last_c_error()const
  {
    return error_.c_str();
  }

  bool get_blocking_mode()const
  {
    return blocking_mode_;
  }

  void set_blocking_mode(bool blocking_mode)
  {
    blocking_mode_ = blocking_mode;
  }

  bool get_transaction_mode()const
  {
    return transaction_mode_;
  }

  //execute a command
  bool exec_command(RedisCommand * command);
  //execute a command with format and va
  //NOTICE: format string is textual without spaces, binary data does not work!!!
  bool exec_commandv(RedisCommand * command, const char * format, va_list ap);
  bool exec_command(RedisCommand * command, const char * format, ...);

  //execute a set of commands in pipeline mode: write all and read all
  bool exec_pipeline(redis_command_vector_t * commands);

  //like exec_command(v), but only write command to redis server
  bool write_command(RedisCommand * command);
  bool write_commandv(RedisCommand * command, const char * format, va_list ap);
  bool write_command(RedisCommand * command, const char * format, ...);

  bool read_reply(RedisCommand * command);
  bool read_line(std::string * line);
  bool read(size_t count, std::string * line);

private:
  bool __read_reply(RedisCommand * command, RedisOutput * output,
    bool check_reply_type);
  //_2 means part 2
  //in part 1 we invoke read_line to read the first line
  //in part 2 we pass the header by
  bool read_bulk_2(const std::string& header,
    std::string * bulk, std::string ** out_bulk);
  bool read_multi_bulk_2(const std::string& header,
    mbulk_t * mbulks, mbulk_t ** out_mbulks);
  //read a whole bulk, when a bulk is expected (in multi bulks' body)
  //*bulk is out on heap or NULL (nil object)
  bool read_bulk(std::string ** bulk);

  static bool parse_integer(const std::string& line, int64_t * i);

  bool check_argc(RedisCommand * command, int given_argc);

  const std::string host_;
  const std::string port_;
  std::string error_;
  TcpClient * tcp_client_;
  boost::posix_time::time_duration timeout_;
  boost::posix_time::time_duration pos_infin_;

  //commands like BLPOP,SUBSCRIBE ... may block clients,
  //so in reading operations pos_infin_ rather than timeout_ is used
  bool blocking_mode_;

  //after MULTI, set transaction_mode_ true
  //after DISCARD or EXEC, set transaction_mode_ false
  bool transaction_mode_;
};

NAMESPACE_END

#endif //_LANGTAOJIN_LIBREDIS_REDIS_PROTOCOL_H_
