  $ stone-authtool kring --create-keyring --mode 0644
  creating kring

  $ stone-authtool kring --list

  $ stone-authtool kring --gen-key

# cram makes matching escape-containing lines with regexps a bit ugly
  $ stone-authtool kring --list
  [client.admin]
  \\tkey = [a-zA-Z0-9+/]+=* \(esc\) (re)

# synonym
  $ stone-authtool kring -l
  [client.admin]
  \\tkey = [a-zA-Z0-9+/]+=* \(esc\) (re)

  $ cat kring
  [client.admin]
  \\tkey = [a-zA-Z0-9+/]+=* \(esc\) (re)
