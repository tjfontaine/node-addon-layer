{
  'targets': [
    {
      'target_name': 'addon-layer',
      'cflags': [ '-Wall', '-Werror' ],
      'type': 'static_library',
      'include_dirs': [ 'include' ],
      'sources': [
        'src/shim.h',
        'src/shim.cc',
      ],
    },
  ],
}
