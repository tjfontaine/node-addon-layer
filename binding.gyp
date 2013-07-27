{
  'targets': [
    {
      'target_name': 'addon-layer',
      'type': 'static_library',
      'include_dirs': [ 'include' ],
      'sources': [
        'src/shim.h',
        'src/shim.cc',
      ],
    },
  ],
}
