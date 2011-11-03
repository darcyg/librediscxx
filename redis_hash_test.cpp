/** @file
* @brief libredis hash test
* @author yafei.zhang@langtaojin.com
* @date
* @version
*
*/
#include <stdio.h>
#include <gflags/gflags.h>
#include "redis_cmd.h"

DEFINE_string(key, "test string", "key.");
DEFINE_int32(group_number, 16, "group number.");

using namespace com::langtaojin::adgaga;

int main(int argc, char * argv[])
{
  google::ParseCommandLineFlags(&argc, &argv, true);

  uint32_t index = time33_hash_32(FLAGS_key) % FLAGS_group_number;
  printf("%s %% %d = %u\n",
    FLAGS_key.c_str(), FLAGS_group_number, index);

  google::ShutDownCommandLineFlags();
  return 0;
}
