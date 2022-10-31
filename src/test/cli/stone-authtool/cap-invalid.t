  $ stone-authtool kring --create-keyring --gen-key --mode 0644
  creating kring

# TODO is this nice?
  $ stone-authtool --cap osd 'broken' kring
  $ stone-authtool kring --list|grep -E '^[[:space:]]caps '
  \tcaps osd = "broken" (esc)

# TODO is this nice?
  $ stone-authtool --cap xyzzy 'broken' kring
  $ stone-authtool kring --list|grep -E '^[[:space:]]caps '
  \tcaps xyzzy = "broken" (esc)
