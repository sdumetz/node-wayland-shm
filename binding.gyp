{
  'targets': [
    {
      'target_name': 'shm',
      'sources': [ 'src/init.c' ],
      'cflags!': [ '-fno-exceptions', '-Wall', '-Wextra', '-Wno-unused-parameter' ],
      'cflags_cc!': [ '-fno-exceptions' ],
    }
  ]
}