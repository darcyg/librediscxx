import sys

env = Environment()
env = env.Clone()

def CheckBoost(context):
    context.Message('Checking for C++ library boost...')
    context.SetLIBS('boost_thread')
    result = context.TryLink(
    """
    #include <boost/thread.hpp>
    int main(int argc, char **argv){return 0;}
    """, '.cpp')
    context.Result(result)
    return result

def CheckGFlags(context):
    context.Message('Checking for C++ library gflags...')
    context.SetLIBS('gflags')
    result = context.TryLink(
    """
    #include <gflags/gflags.h>
    int main(int argc, char **argv){return 0;}
    """, '.cpp')
    context.Result(result)
    return result

conf = Configure(env, custom_tests = {'CheckBoost':CheckBoost, 'CheckGFlags':CheckGFlags})
conf.CheckCC()
have_boost = conf.CheckBoost()
have_gflags = conf.CheckGFlags()
if not have_boost:
    print 'Error: no boost'
    sys.exit()
if not have_gflags:
    print 'Warning: no gflags, test and monitor will not be compiled'
env = conf.Finish()

print 'Have gflags = %s' % have_gflags

env.Append(CCFLAGS = Split('-Wall -g -O2'))
env.Append(LIBPATH = '.')

COMMON_LIBS = [
    File('./libredis.a'),
    File('/usr/lib/libboost_thread.a'),
    File('/usr/lib/libboost_system.a'),
    File('/usr/lib/libboost_date_time.a'),
    'pthread',
]
if have_gflags:
    COMMON_LIBS += [File('/usr/local/lib/libgflags.a')]
env.Append(LIBS = COMMON_LIBS)

LibrarySource = [
    'blocking_tcp_client.cpp',
    'host_resolver.cpp',
    'os.cpp',
    'redis_base.cpp',
    'redis_cmd.cpp',
    'redis.cpp',
    'redis_partition.cpp',
    'redis_protocol.cpp',
    'redis_tss.cpp'
]

env.StaticLibrary(
    'redis',
    LibrarySource,
)

if have_gflags:
    env.Program('redis_test',
        Split('redis_test.cpp'),
    )

if have_gflags:
    env.Program('redis_hash_test',
        Split('redis_hash_test.cpp'),
    )

if have_gflags:
    env.Program('redis_monitor',
        Split('redis_monitor.cpp'),
    )
