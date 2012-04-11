import sys
import os

def FindStaticLib(libname, LIBRARY_PATH=os.environ.get('LIBRARY_PATH', '')):
    path_list=['.']
    if LIBRARY_PATH:
        path_list += LIBRARY_PATH.split('::')
    path_list += ['/lib', '/usr/lib', '/usr/local/lib']
    for path in path_list:
        fn = path + '/' + 'lib' + libname + '.a';
        if os.path.isfile(fn):
            return File(fn)
    return libname

env = Environment()
env = env.Clone()

conf = Configure(env)
conf.CheckCC()
conf.CheckCXX()
if not conf.CheckLibWithHeader('boost_thread', 'boost/thread.hpp', 'C++', autoadd=0):
    print 'Error: no boost'
    sys.exit()
env = conf.Finish()

env.Append(CCFLAGS = Split('-Wall -g -O2'))
env.Append(CPPPATH = 'src')

COMMON_LIBS = [
    File('./libredis.a'),
    FindStaticLib('boost_thread'),
    FindStaticLib('boost_system'),
    FindStaticLib('boost_date_time'),
    FindStaticLib('boost_program_options'),
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
