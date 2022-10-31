  $ cat >test.conf <<EOF
  > [bar]
  > bar = green
  > EOF

# TODO output an error
  $ stone-conf -c test.conf broken
  [1]

  $ stone-conf -c test.conf --name total.garbage
  error parsing 'total.garbage': expected string of the form TYPE.ID, valid types are: auth, mon, osd, mds, mgr, client
  [1]

  $ stone-conf -c test.conf -s bar
  You must give an action, such as --lookup or --list-all-sections.
  Pass --help for more help.
  [1]
