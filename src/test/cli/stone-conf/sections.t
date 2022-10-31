  $ cat >test.conf <<EOF
  > [bar]
  > bar = green
  > [foo]
  > bar = blue
  > [baz]
  > bar = yellow
  > [thud]
  > bar = yellow
  > EOF

  $ stone-conf -c test.conf -l bar
  bar

  $ stone-conf -c test.conf -l b
  bar
  baz

