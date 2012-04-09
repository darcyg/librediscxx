import sys

env = Environment()
env = env.Clone()

def CheckBoost(context):
    context.Message('Checking for C++ library boost...')
#    context.SetLIBS('boost_thread')
    result = context.TryCompile(
    """
    #include <boost/thread.hpp>
    int main(int argc, char **argv){return 0;}
    """, '.cpp')
    context.Result(result)
    return result

conf = Configure(env, custom_tests = {'CheckBoost':CheckBoost})
conf.CheckCC()
have_boost = conf.CheckBoost()
if not have_boost:
    print 'Error: no boost'
    sys.exit()
env = conf.Finish()

env.Append(CCFLAGS = Split('-Wall -g'))
env.Append(CPPPATH = 'src')
env.Append(LIBPATH = '.')

COMMON_LIBS = [
    File('./libredis.a'),
    File('/usr/lib/libboost_thread.a'),
    File('/usr/lib/libboost_system.a'),
    File('/usr/lib/libboost_date_time.a'),
    File('/usr/lib/libboost_program_options.a'),
    'pthread',
]
env.Append(LIBS = COMMON_LIBS)

LibrarySource = [
    'src/blocking_tcp_client.cpp',
    'src/host_resolver.cpp',
    'src/os.cpp',
    'src/redis_base.cpp',
    'src/redis_cmd.cpp',
    'src/redis.cpp',
    'src/redis_partition.cpp',
    'src/redis_protocol.cpp',
    'src/redis_tss.cpp'
]

env.StaticLibrary(
    'redis',
    LibrarySource,
)

env.Program('redis_test',
    Split('test/redis_test.cpp'),
)

env.Program('redis_hash_test',
    Split('tools/redis_hash_test.cpp'),
)

env.Program('redis_monitor',
    Split('tools/redis_monitor.cpp'),
)
