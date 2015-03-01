{
  'variables': {
    'proto_libdir' :      '<!(pkg-config --libs-only-L protobuf)',
    'zmq_libdir' :        '<!(pkg-config --libs-only-L libzmq)',
    'sodium_libdir':      '<!(./filedir_1.sh "libsodium.[ads]*" $HOME/libsodium-install)',
    'sodium_lib':         '<!(./if_exists.sh <(sodium_libdir) "-lsodium" -L/none/)',
    'has_sodium':         '<!(./file_exists.sh <(sodium_libdir))',
    'common_ldflags':   [
                          '<!(./libdir_1.sh "libprotobuf.[ads]*" $HOME/protobuf-install /usr/local/lib)',
                          '<!(./libdir_1.sh "libzmq.[ads]*" $HOME/libzmq-install /usr/local/lib)',
                          '<!(./libdir_1.sh "libsodium.[ads]*" $HOME/libsodium-install /usr/local/lib)',
                        ],
    'common_libs':      [
                          '<!@(pkg-config --libs-only-L --libs-only-l protobuf libzmq)',
                          '<!@(./genrpath.sh "<(proto_libdir)" "<(zmq_libdir)" )',
                          '<(sodium_lib)',
                        ],
    'cachedb_sources':
                        [
                          # cache db sources
                          'cachedb/hash_util.cc',           'cachedb/hash_util.hh',
                          # new cachedb sources
                          'cachedb/db.cc',                  'cachedb/db.hh',
                          'cachedb/storeable.cc',           'cachedb/storeable.hh',
                          'cachedb/column_data.cc',         'cachedb/column_data.hh',
                          'cachedb/query_column_block.cc',  'cachedb/query_column_block.hh',
                          'cachedb/query_table_log.cc',     'cachedb/query_table_log.hh',
                          'cachedb/query_table_block.cc',   'cachedb/query_table_block.hh',
                        ],
    'dsproxy_sources':  [
                          'dsproxy.hh',
                          'dsproxy/meta_proxy.cc',         'dsproxy/meta_proxy.hh',
                          'dsproxy/query_proxy.cc',        'dsproxy/query_proxy.hh',
                          'dsproxy/column_proxy.cc',       'dsproxy/column_proxy.hh',
                          'dsproxy/query_dispatcher.cc',   'dsproxy/query_dispatcher.hh',
                          'dsproxy/column_dispatcher.cc',  'dsproxy/column_dispatcher.hh',
                        ],
    'common_sources' :  [
                          # generic utils
                          'util.hh',                    'util/constants.hh',
                          'util/active_queue.hh',       'util/flex_alloc.hh',
                          'util/barrier.cc',            'util/barrier.hh',
                          'util/relative_time.cc',      'util/relative_time.hh',
                          'util/exception.hh',          'util/value_type.hh',
                          'util/net.cc',                'util/net.hh',
                          'util/hex_util.cc',           'util/hex_util.hh',
                          'util/zmq_utils.cc',          'util/zmq_utils.hh',
                          'util/async_worker.cc',       'util/async_worker.hh',
                          'util/compare_messages.hh',   'util/table_collector.hh',
                          'util/timer_service.cc',      'util/timer_service.hh',   
                          'util/utf8.cc',               'util/utf8.hh',
                          'util/value_type_reader.cc',  'util/value_type_reader.hh',
                          # logger support
                          'logger.hh',
                          'logger/macros.hh',        'logger/on_return.hh',
                          'logger/log_record.cc',    'logger/log_record.hh',
                          'logger/process_info.cc',  'logger/process_info.hh',
                          'logger/symbol_store.cc',  'logger/symbol_store.hh',
                          'logger/header_store.cc',  'logger/header_store.hh',
                          'logger/log_sink.cc',      'logger/log_sink.hh',
                          'logger/signature.cc',     'logger/signature.hh',
                          'logger/end_msg.hh',       'logger/variable.hh',
                          # connector helpers
                          'connector.hh',
                          'connector/server_base.cc',          'connector/server_base.hh',
                          'connector/client_base.cc',          'connector/client_base.hh',
                          'connector/sub_client.hh',           'connector/req_client.hh',
                          'connector/push_client.hh',          'connector/pub_server.hh',
                          'connector/pull_server.hh',          'connector/rep_server.hh',
                          'connector/column_client.cc',        'connector/column_client.hh',
                          'connector/column_server.cc',        'connector/column_server.hh',
                          'connector/config_client.cc',        'connector/config_client.hh',
                          'connector/config_server.cc',        'connector/config_server.hh',
                          'connector/db_config_client.cc',     'connector/db_config_client.hh',
                          'connector/db_config_server.cc',     'connector/db_config_server.hh',
                          'connector/endpoint_client.cc',      'connector/endpoint_client.hh',
                          'connector/endpoint_server.cc',      'connector/endpoint_server.hh',
                          'connector/ip_discovery_client.cc',  'connector/ip_discovery_client.hh',
                          'connector/ip_discovery_server.cc',  'connector/ip_discovery_server.hh',
                          'connector/log_record_client.cc',    'connector/log_record_client.hh',
                          'connector/log_record_server.cc',    'connector/log_record_server.hh',
                          'connector/meta_data_client.cc',     'connector/meta_data_client.hh',
                          'connector/meta_data_server.cc',     'connector/meta_data_server.hh',
                          'connector/query_client.cc',         'connector/query_client.hh',
                          'connector/query_server.cc',         'connector/query_server.hh',
                          # data helpers
                          'datasrc.hh',
                          'datasrc/column.cc',           'datasrc/column.hh',
                          'datasrc/int32_column.cc',     'datasrc/int32_column.hh',
                          'datasrc/date_column.cc',      'datasrc/date_column.hh',
                          'datasrc/time_column.cc',      'datasrc/time_column.hh',
                          'datasrc/datetime_column.cc',  'datasrc/datetime_column.hh',
                          'datasrc/string_column.cc',    'datasrc/string_column.hh',
                          'datasrc/utf8_column.cc',      'datasrc/utf8_column.hh',
                          'datasrc/bytes_column.cc',     'datasrc/bytes_column.hh',
                          'datasrc/float_column.cc',     'datasrc/float_column.hh',
                          'datasrc/double_column.cc',    'datasrc/double_column.hh',
                          'datasrc/int64_column.cc',     'datasrc/int64_column.hh',
                          'datasrc/pool.cc',             'datasrc/pool.hh',
                          # engine
                          'engine/chunk_store.cc',       'engine/chunk_store.hh',
                          'engine/column_chunk.cc',      'engine/column_chunk.hh',
                          'engine/data_chunk.cc',        'engine/data_chunk.hh',
                          'engine/data_handler.cc',      'engine/data_handler.hh',
                          'engine/expression.cc',        'engine/expression.hh',
                          'engine/query.cc',             'engine/query.hh',
                          'engine/receiver_thread.cc',   'engine/receiver_thread.hh',
                          'engine/collector.cc',         'engine/collector.hh',
                          'engine/feeder.cc',            'engine/feeder.hh',
                          'engine/util.hh',
                          # fault injection
                          'fault/injector.cc',           'fault/injector.hh',
                        ],
  },
  'target_defaults': {
    'default_configuration': 'Debug',
    'configurations': {
      'Debug': {
        'defines':  [ 'DEBUG', '_DEBUG', ],
        'cflags':   [ '-O0', '-g3', ],
        'ldflags':  [ '-g3', ],
        'xcode_settings': {
          'OTHER_CFLAGS':  [ '-O0', '-g3', ],
          'OTHER_LDFLAGS': [ '-g3', ],
        },
      },
      'Release': {
        'defines':  [ 'NDEBUG', 'RELEASE', ],
        'cflags':   [ '-O3', ],
        'xcode_settings': {
          # 'OTHER_LDFLAGS': [ '-O3', ],
        },
      },
    },
    'include_dirs': [
      '<!(pwd)',
      './',
      './cppzmq/',
      './proto/',
      './rocksdb/include/',
      './lz4/lib/',
      '/usr/local/include/',
      '/usr/include/',
      '<!@(pkg-config --variable=includedir protobuf libzmq)',
    ],
    'cflags': [
      '-Wall',
    ],
    'defines': [
      'PIC',
      'STD_CXX_11',
      '_THREAD_SAFE',
    ],
    'target_conditions': [
      ['_type=="shared_library"', {'cflags': ['-fPIC']}],
      ['_type=="static_library"', {'cflags': ['-fPIC']}],
      ['_type=="executable"',     {'cflags': ['-fPIC']}],
    ],
    'conditions': [
      ['OS=="mac"', {
        'defines':            [ 'COMMON_MAC_BUILD', ],
        'cflags':             [ '<!@(pkg-config --cflags protobuf libzmq)', '-I<!(pwd)/'],
        'xcode_settings':  {
          'GCC_ENABLE_CPP_EXCEPTIONS':   'YES',
          'OTHER_LDFLAGS':    [
                                '<!@(pkg-config --libs-only-L --libs-only-l protobuf libzmq)',
                                '<@(common_ldflags)',
                              ],
          'OTHER_CFLAGS':     [ '-I<!(pwd)/', '-I<!(pwd)/cppzmq/', ],
        },
      },],
      ['OS=="linux"', {
        'defines':            [ 'COMMON_LINUX_BUILD', ],
        'cflags':             [ '<!@(pkg-config --cflags protobuf libzmq)', ],
        'link_settings': {
          'ldflags':          [
                                '-Wl,--no-as-needed', 
                                '<@(common_ldflags)',
                              ],
          'libraries':        [
                                '<@(common_libs)',
                                '-lrt',
                              ],
        },
      },],
    ],
  },
  'targets' : [
    {
      'conditions': [
        ['OS=="mac"', {
          'all_dependent_settings': {
            'defines':            [ 'USING_LZ4_LIB', 'LZ4_MAC_BUILD', ],
            'xcode_settings': {
              'OTHER_LDFLAGS':    [ '<!(pwd)/lz4/lib/liblz4.a', ],
              'OTHER_CFLAGS':     [ '-std=c++11', ],
            },
          },
        },],
        ['OS=="linux"', {
          'all_dependent_settings': {
            'defines':            [ 'USING_LZ4_LIB', 'LZ4_LINUX_BUILD', ],
            'link_settings': {
              'ldflags':          [ '<@(common_ldflags)', ],
              'libraries':        [ './lz4/lib/liblz4.a', ],
            },
          },
        },],
      ],
      'target_name':       'lz4',
      'type':              'none',
      #'hard_dependency':   1,
      'direct_dependent_settings': { 'include_dirs': [ './lz4/lib', ], },
      'sources':           [
                             'lz4/lib/lz4.c',        'lz4/lib/lz4.h',
                             'lz4/lib/lz4hc.c',      'lz4/lib/lz4hc.h',
                             'lz4/lib/lz4frame.c',   'lz4/lib/lz4frame.h',
                             'lz4/lib/xxhash.c',     'lz4/lib/xxhash.h',
                             'lz4/lib/lz4frame_static.h',
                           ],
      'actions': [ {
        'action_name':     'lz4_build',
        'inputs':          [
                             'lz4/lib/lz4.c',        'lz4/lib/lz4.h',
                             'lz4/lib/lz4hc.c',      'lz4/lib/lz4hc.h',
                             'lz4/lib/lz4frame.c',   'lz4/lib/lz4frame.h',
                             'lz4/lib/xxhash.c',     'lz4/lib/xxhash.h',
                             'lz4/lib/lz4frame_static.h',
                           ],
        'outputs':         [ './lz4/lib/liblz4.a', './xlast-lz4.a', ],
        'action':          [ './build-lz4.sh', ],
      },],
    },
    {
      'conditions': [
        ['OS=="mac"', {
          'direct_dependent_settings': {
            'defines':            [ 'USING_SNAPPY_LIB', 'SNAPPY_MAC_BUILD', ],
            'xcode_settings': {
              'OTHER_LDFLAGS':    [ '<!(pwd)/snappy/.libs/libsnappy.a', ],
              'OTHER_CFLAGS':     [ '-std=c++11', ],
            },
          },
        },],
        ['OS=="linux"', {
          'direct_dependent_settings': {
            'defines':            [ 'USING_SNAPPY_LIB', 'SNAPPY_LINUX_BUILD', ],
            'link_settings': {
              'ldflags':          [ '<@(common_ldflags)', ],
              'libraries':        [ './snappy/.libs/libsnappy.a', ],
            },
          },
        },],
      ],
      'target_name':         'snappy',
      'type':                'none',
      #'hard_dependency':      1,
      'sources':           [ 'snappy/snappy.cc', 'snappy/snappy.h', ],
      'actions': [ {
        'action_name':   'snappy_build',
        'inputs':        [ 'snappy/snappy.cc', 'snappy/snappy.h', 'snappy/Makefile.am', ],
        'outputs':       [ './snappy/.libs/libsnappy.a', './xlast-snappy.a', ],
        'action':        [ './build-snappy.sh',  ],
      },],
    },
    {
      'conditions': [
        ['OS=="mac"', {
          'direct_dependent_settings': {
            'defines':            [ 'USING_ROCKSDB_LIB', 'ROCKSDB_MAC_BUILD', ],
            'xcode_settings': {
              'OTHER_LDFLAGS':    [ '<!(pwd)/rocksdb/librocksdb.a', ],
              'OTHER_CFLAGS':     [ '-std=c++11', ],
            },
          },
        },],
        ['OS=="linux"', {
          'direct_dependent_settings': {
            'defines':            [ 'USING_ROCKSDB_LIB', 'ROCKSDB_LINUX_BUILD', ],
            'link_settings': {
              'ldflags':          [ '<@(common_ldflags)', ],
              'libraries':        [
                                    './rocksdb/librocksdb.a',
                                    './xlast-lz4.a', 
                                    './xlast-snappy.a',
                                  ],
            },
          },
        },],
      ],
      'target_name':                  'rocksdb',
      'type':                         'none',
      'dependencies':               [ 'snappy', 'lz4', ],
      'direct_dependent_settings':  { 'include_dirs': [ './rocksdb/include', ], },
      'export_dependent_settings':  [ 'lz4', 'snappy', ],
      'sources':      [
                        'rocksdb/db/c.cc',
                        'rocksdb/include/rocksdb/db.h',
                        'rocksdb/include/rocksdb/env.h',
                        'rocksdb/include/rocksdb/cache.h',
                        'rocksdb/include/rocksdb/filter_policy.h',
                        'rocksdb/include/rocksdb/slice_transform.h',
                        'rocksdb/include/utilities/backupable_db.h',
                        'rocksdb/utilities/backupable/backupable_db.cc',
                        'rocksdb/table/block_based_table_builder.cc',
                      ],
      'variables': {
        'rocksdb_lib': './rocksdb/librocksdb.a',
      },
      'actions': [ {
        'action_name':  'rocksdb_build',
        'inputs':     [
                        'rocksdb/db/c.cc',
                        'rocksdb/include/rocksdb/db.h',
                        'rocksdb/include/rocksdb/env.h',
                        'rocksdb/include/rocksdb/cache.h',
                        'rocksdb/include/rocksdb/filter_policy.h',
                        'rocksdb/include/rocksdb/slice_transform.h',
                        'rocksdb/include/utilities/backupable_db.h',
                        'rocksdb/utilities/backupable/backupable_db.cc',
                      ],
        'outputs':    [ '<(rocksdb_lib)', ],
        'action':     [ './build-rocksdb.sh', ],
      } ],
    },
    {
      'conditions': [
        ['OS=="mac"', {
          'variables':  { 'common_root':  '<!(pwd)/../', },
          'direct_dependent_settings': {
            'defines':            [ 'USING_COMMON_LIB', 'COMMON_MAC_BUILD', ],
            'include_dirs':       [ '<(common_root)/', ],
            'xcode_settings': {
              'OTHER_CFLAGS':     [ '-std=c++11', ],
            },
          },
        },],
        ['OS=="linux"', {
          'direct_dependent_settings': {
            'defines':            [ 'USING_COMMON_LIB', 'COMMON_LINUX_BUILD', ],
            'include_dirs':       [ '.', ],
            'link_settings': {
              'ldflags':          [ '<@(common_ldflags)', ],
              'libraries':        [ '<@(common_libs)', '-lrt', ],
            },
          },
        },],
      ],
      'target_name':                   'common',
      'type':                          'static_library',
      'hard_dependency':                1,
      'dependencies':                [ 'lz4', 'proto/proto.gyp:*', ],
      'export_dependent_settings':   [ 'lz4', 'proto/proto.gyp:*', ],
      'cflags':                      [ '-std=c++11', '-Wall', ],
      'sources':                     [ '<@(common_sources)', ],
    },
    {
      'conditions': [
        ['OS=="mac"', {
          'variables':  { 'common_root':  '<!(pwd)/../', },
          'direct_dependent_settings': {
            'defines':            [ 'USING_COMMON_LIB', 'COMMON_MAC_BUILD', 'INJECT_FAULTS', ],
            'include_dirs':       [ '<(common_root)/', ],
            'xcode_settings': {
              'OTHER_CFLAGS':     [ '-std=c++11', ],
            },
          },
        },],
        ['OS=="linux"', {
          'direct_dependent_settings': {
            'defines':            [ 'USING_COMMON_LIB', 'COMMON_LINUX_BUILD', 'INJECT_FAULTS', ],
            'include_dirs':       [ '.', ],
            'link_settings': {
              'ldflags':          [ '<@(common_ldflags)', ],
              'libraries':        [ '<@(common_libs)', '-lrt', ],
            },
          },
        },],
      ],
      'target_name':                   'common_with_faults',
      'type':                          'static_library',
      'dependencies':                [ 'lz4', 'proto/proto.gyp:*', ],
      'export_dependent_settings':   [ 'lz4', 'proto/proto.gyp:*', ],
      'cflags':                      [ '-std=c++11', '-Wall', ],
      'sources':                     [ '<@(common_sources)', ],
    },
    {
      'conditions': [
        ['OS=="mac"', {
          'variables':  { 'cachedb_root':  '<!(pwd)/../', },
          'direct_dependent_settings': {
            'defines':            [ 'USING_CACHEDB_LIB', 'CACHEDB_MAC_BUILD', ],
            'include_dirs':       [ '<(cachedb_root)/', ],
            'xcode_settings': {
              'OTHER_CFLAGS':     [ '-std=c++11', ],
            },
          },
        },],
        ['OS=="linux"', {
          'direct_dependent_settings': {
            'defines':            [ 'USING_CACHEDB_LIB', 'CACHEDB_LINUX_BUILD', ],
            'include_dirs':       [ '.', ],
            'link_settings': {
              'ldflags':          [ '<@(common_ldflags)', ],
              'libraries':        [ '<@(common_libs)', '-lrt', ],
            },
          },
        },],
      ],
      'target_name':                   'cachedb',
      'type':                          'static_library',
      'dependencies':                [ 'rocksdb', 'common', ],
      'export_dependent_settings':   [ 'rocksdb', 'common', ],
      'cflags':                      [ '-std=c++11', '-Wall', ],
      'sources':                     [ '<@(cachedb_sources)', ],
    },
    {
      'conditions': [
        ['OS=="mac"', {
          'variables':  { 'cachedb_root':  '<!(pwd)/../', },
          'direct_dependent_settings': {
            'defines':            [ 'USING_CACHEDB_LIB', 'CACHEDB_MAC_BUILD', 'INJECT_FAULTS', ],
            'include_dirs':       [ '<(cachedb_root)/', ], # TODO: check this
            'xcode_settings': {
              'OTHER_CFLAGS':     [ '-std=c++11', ],
            },
          },
        },],
        ['OS=="linux"', {
          'direct_dependent_settings': {
            'defines':            [ 'USING_CACHEDB_LIB', 'CACHEDB_LINUX_BUILD', 'INJECT_FAULTS', ],
            'include_dirs':       [ '.', ],
            'link_settings': {
              'ldflags':          [ '<@(common_ldflags)', ],
              'libraries':        [ '<@(common_libs)', '-lrt', ],
            },
          },
        },],
      ],
      'target_name':                   'cachedb_with_faults',
      'type':                          'static_library',
      'dependencies':                [ 'rocksdb', 'common', ],
      'export_dependent_settings':   [ 'rocksdb', 'common', ],
      'cflags':                      [ '-std=c++11', '-Wall', ],
      'sources':                     [ '<@(cachedb_sources)', ],
    },
    {
      'conditions': [
        ['OS=="mac"', {
          'variables':  { 'dsproxy_root':  '<!(pwd)/../', },
          'direct_dependent_settings': {
            'defines':            [ 'USING_DSPROXY_LIB', 'DSPROXY_MAC_BUILD', ],
            'include_dirs':       [ '<(dsproxy_root)/', ],
            'xcode_settings': {
              'OTHER_CFLAGS':     [ '-std=c++11', ],
            },
          },
        },],
        ['OS=="linux"', {
          'direct_dependent_settings': {
            'defines':            [ 'USING_DSPROXY_LIB', 'DSPROXY_LINUX_BUILD', ],
            'include_dirs':       [ '.', ],
            'link_settings': {
              'ldflags':          [ '<@(common_ldflags)', ],
              'libraries':        [ '<@(common_libs)', '-lrt', ],
            },
          },
        },],
      ],
      'target_name':                   'dsproxy',
      'type':                          'static_library',
      'dependencies':                [ 'common', ],
      'export_dependent_settings':   [ 'common', ],
      'cflags':                      [ '-std=c++11', '-Wall', ],
      'sources':                     [ '<@(dsproxy_sources)', ],
    },
    {
      'conditions': [
        ['OS=="mac"', {
          'variables':  { 'dsproxy_root':  '<!(pwd)/../', },
          'direct_dependent_settings': {
            'defines':            [ 'USING_DSPROXY_LIB', 'DSPROXY_MAC_BUILD', 'INJECT_FAULTS', ],
            'include_dirs':       [ '<(dsproxy_root)/', ],
            'xcode_settings': {
              'OTHER_CFLAGS':     [ '-std=c++11', ],
            },
          },
        },],
        ['OS=="linux"', {
          'direct_dependent_settings': {
            'defines':            [ 'USING_DSPROXY_LIB', 'DSPROXY_LINUX_BUILD', 'INJECT_FAULTS', ],
            'include_dirs':       [ '.', ],
            'link_settings': {
              'ldflags':          [ '<@(common_ldflags)', ],
              'libraries':        [ '<@(common_libs)', '-lrt', ],
            },
          },
        },],
      ],
      'target_name':                 'dsproxy_with_faults',
      'type':                        'static_library',
      'dependencies':              [ 'common', ],
      'export_dependent_settings': [ 'common', ],
      'cflags':                    [ '-std=c++11', '-Wall', ],
      'sources':                   [ '<@(dsproxy_sources)', ],
    },
    {
      'target_name':       'gtest_main',
      'type':              'executable',
      'defines':           [ 'INJECT_FAULTS', ],
      'dependencies':      [
                             'gtest/gyp/gtest.gyp:gtest_lib',
                             'proto/proto.gyp:*',
                             'dsproxy_with_faults',
                             'cachedb_with_faults',
                           ],
      'include_dirs':      [ './gtest/include/', ],
      'cflags':            [ '-std=c++11', '-Wall', ],
      'sources':           [
                             'test/gtest_main.cc',
                             'test/value_type_test.cc',   'test/value_type_test.hh',
                             'test/logger_test.cc',       'test/logger_test.hh',
                             'test/util_test.cc',         'test/util_test.hh',
                             'test/connector_test.cc',    'test/connector_test.hh',
                             'test/datasrc_test.cc',      'test/datasrc_test.hh',
                             'test/engine_test.cc',       'test/engine_test.hh',
                             'test/fault_test.cc',        'test/fault_test.hh',
                             'test/cachedb_test.cc',      'test/cachedb_test.hh',
                           ],
    },
    {
      'target_name':       'cfgsvc_mock',
      'type':              'executable',
      'dependencies':      [ 'common', 'proto/proto.gyp:*', ],
      'include_dirs':      [ './gtest/include/', ],
      'cflags':            [ '-std=c++11', '-Wall', ],
      'sources':           [ 'test/cfgsvc_mock.cc', ],
    },
    {
      'target_name':       'murmur3',
      'type':              'static_library',
      'sources':           [ 'murmur3/murmur3.c', 'murmur3/murmur3.h', ],
    },
  ],
}
