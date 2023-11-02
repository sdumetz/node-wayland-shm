{
  'targets': [
    {
      'target_name': 'shm',
      'sources': [ 'src/shm.cc' ],
      'cflags!': [ '-fno-exceptions' ],
      'cflags_cc!': [ '-fno-exceptions' ],
    }
  ]
}