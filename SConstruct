env = Environment()
env = env.Clone()
#env.Append(CCFLAGS = Split('-Wall -g -O2 -finline-functions'))
env.Append(CCFLAGS = Split('-Wall -g'))
env.Append(LIBPATH = '.')

env.Append(LIBS = [
    File('./libredis.a'),
    File('/usr/local/lib/libgflags.a'),
    File('/usr/lib/libboost_thread.a'),
    File('/usr/lib/libboost_system.a'),
    File('/usr/lib/libboost_date_time.a'),
    'pthread',
])


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


env.Program('redis_test',
    Split('redis_test.cpp'),
)


env.Program('redis_hash_test',
    Split('redis_hash_test.cpp'),
)


env.Program('redis_monitor',
    Split('redis_monitor.cpp'),
)
