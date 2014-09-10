{
  'target_defaults': {
    'default_configuration': 'Debug',
    'configurations': {
      'Debug': {
        'defines':  ['DEBUG', '_DEBUG', ],
        'cflags':   ['-O0', '-g3', ],
        'ldflags':  ['-g3', ],
        'xcode_settings': {
          'OTHER_CFLAGS':  [ '-O0', '-g3', ],
          'OTHER_LDFLAGS': [ '-g3', ],
        },
      },
      'Release': {
        'defines': ['NDEBUG', 'RELEASE', ],
        'cflags': ['-O3', ],
        'xcode_settings': {
          'OTHER_LDFLAGS': [ '-O3', ],
        },
      },
    },
    'include_dirs': [
      '<!(pwd)',
      './',
      './cppzmq/',
      './proto/',
      '/usr/local/include/',
      '/usr/include/',
      '<!@(pkg-config --variable=includedir protobuf libzmq)',
    ],
    'cflags': [
      '-std=c++11',
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
        'defines':                       [ 'COMMON_MAC_BUILD', ],
        'cflags':                        [ '<!@(pkg-config --cflags protobuf libzmq)', '-I<!(pwd)/'],
        'xcode_settings':  {
          'GCC_ENABLE_CPP_EXCEPTIONS':   'YES',
          'OTHER_LDFLAGS':               [ '<!@(pkg-config --libs-only-L --libs-only-l protobuf libzmq)', ],
          'OTHER_CFLAGS':                [ '-std=c++11', '-I<!(pwd)/', '-I<!(pwd)/cppzmq/', ],
        },
      },],
      ['OS=="linux"', {
        'defines':                       [ 'COMMON_LINUX_BUILD', ],
        'cflags':                        [ '<!@(pkg-config --cflags protobuf libzmq)', ],
        'link_settings': {
          'ldflags':                     [ '-Wl,--no-as-needed', ],
          'libraries':                   [ '<!@(pkg-config --libs-only-L --libs-only-l protobuf libzmq)', ],
        },
      },],
    ],
  },
  'targets' : [
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
          },
        },],
      ],
      'target_name':       'common',
      'type':              'static_library',
      'dependencies':      [ 'proto/proto.gyp:*', ],
      'sources':           [
                             # generic utils
                             'util.hh',                  'util/constants.hh',
                             'util/active_queue.hh',     'util/flex_alloc.hh',
                             'util/barrier.cc',          'util/barrier.hh',
                             'util/relative_time.cc',    'util/relative_time.hh',
                             'util/exception.hh',        'util/value_type.hh',
                             'util/net.cc',              'util/net.hh',
                             'util/zmq_utils.cc',        'util/zmq_utils.hh',
                             'util/async_worker.cc',     'util/async_worker.hh',
                             'util/compare_messages.hh',
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
                           ],
    },
    {
      'target_name':       'gtest_main',
      'type':              'executable',
      'dependencies':      [ 'proto/proto.gyp:proto', 'gtest/gyp/gtest.gyp:gtest_lib', 'common', ],
      'include_dirs':      [ './gtest/include/', ],
      'sources':           [
                             'test/gtest_main.cc',
                             'test/value_type_test.cc',   'test/value_type_test.hh',
                             'test/logger_test.cc',       'test/logger_test.hh',
                             'test/util_test.cc',         'test/util_test.hh',
                             'test/connector_test.cc',    'test/connector_test.hh',
                           ],
    },
    {
      'target_name':       'netinfo',
      'type':              'executable',
      'dependencies':      [ 'common', 'proto/proto.gyp:proto', ],
      'include_dirs':      [ './', ],
      'sources':           [
                             'test/netinfo.cc',
                           ],
    },
  ],
}
