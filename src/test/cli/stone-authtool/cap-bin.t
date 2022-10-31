  $ stone-authtool kring --create-keyring --gen-key --mode 0644
  creating kring

  $ stone-authtool --cap osd 'allow rx pool=swimming' kring
  $ stone-authtool kring --list|grep -E '^[[:space:]]caps '
  \tcaps osd = "allow rx pool=swimming" (esc)
