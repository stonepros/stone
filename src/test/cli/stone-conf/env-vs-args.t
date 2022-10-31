# we can use STONE_CONF to override the normal configuration file location.
  $ env STONE_CONF=from-env stone-conf -s foo bar
  did not load config file, using default settings.
  .* \-1 Errors while parsing config file! (re)
  .* \-1 can't open from-env: \(2\) (No such file or directory)? (re)
  .* \-1 Errors while parsing config file! (re)
  .* \-1 can't open from-env: \(2\) (No such file or directory)? (re)
  [1]

# command-line arguments should override environment
  $ env -u STONE_CONF stone-conf -c from-args
  global_init: unable to open config file from search list from-args
  [1]

