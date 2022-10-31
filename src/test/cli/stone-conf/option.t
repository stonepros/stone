  $ cat >test.conf <<EOF
  > [bar]
  > bar = green
  > [foo]
  > bar = blue
  > [baz]
  > bar = yellow
  > [thud]
  > bar = red
  > [nobar]
  > other = 42
  > EOF

  $ stone-conf -c test.conf bar -s foo
  blue

# test the funny "equals sign" argument passing convention
  $ stone-conf --conf=test.conf bar -s foo
  blue

  $ stone-conf --conf=test.conf -L
  bar
  baz
  foo
  nobar
  thud

  $ stone-conf --conf=test.conf --list-all-sections
  bar
  baz
  foo
  nobar
  thud

  $ stone-conf --conf=test.conf --list_all_sections
  bar
  baz
  foo
  nobar
  thud

# TODO man page stops in the middle of a sentence

  $ stone-conf -c test.conf bar -s xyzzy
  [1]

  $ stone-conf -c test.conf bar -s xyzzy
  [1]

  $ stone-conf -c test.conf bar -s xyzzy -s thud
  red

  $ stone-conf -c test.conf bar -s nobar -s thud
  red

  $ stone-conf -c test.conf bar -s thud -s baz
  red

  $ stone-conf -c test.conf bar -s baz -s thud
  yellow

  $ stone-conf -c test.conf bar -s xyzzy -s nobar -s thud -s baz
  red

