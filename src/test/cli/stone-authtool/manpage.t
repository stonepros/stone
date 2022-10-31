  $ stone-authtool
  stone-authtool: -h or --help for usage
  [1]

# demonstrate that manpage examples fail without config
# TODO fix the manpage
  $ stone-authtool --create-keyring --name client.foo --gen-key keyring
  creating keyring

# work around the above
  $ touch stone.conf

To create a new keyring containing a key for client.foo:

  $ stone-authtool --create-keyring --id foo --gen-key keyring
  creating keyring

  $ stone-authtool --create-keyring --name client.foo --gen-key keyring
  creating keyring

To associate some capabilities with the key (namely, the ability to mount a Stone filesystem):

  $ stone-authtool -n client.foo --cap mds 'allow' --cap osd 'allow rw pool=data' --cap mon 'allow r' keyring

To display the contents of the keyring:

  $ stone-authtool -l keyring
  [client.foo]
  \\tkey = [a-zA-Z0-9+/]+=* \(esc\) (re)
  \tcaps mds = "allow" (esc)
  \tcaps mon = "allow r" (esc)
  \tcaps osd = "allow rw pool=data" (esc)
