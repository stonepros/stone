  $ stone-authtool kring --create-keyring --mode 0644
  creating kring

  $ stone-authtool kring --add-key 'AQAK7yxNeF+nHBAA0SgSdbs8IkJrxroDeJ6SwQ== 18446744073709551615'
  added entity client.admin auth(key=AQAK7yxNeF+nHBAA0SgSdbs8IkJrxroDeJ6SwQ==)

# cram makes matching escape-containing lines with regexps a bit ugly
  $ stone-authtool kring --list
  [client.admin]
  \tkey = AQAK7yxNeF+nHBAA0SgSdbs8IkJrxroDeJ6SwQ== (esc)

  $ cat kring
  [client.admin]
  \tkey = AQAK7yxNeF+nHBAA0SgSdbs8IkJrxroDeJ6SwQ== (esc)

Test --add-key with empty argument

  $ stone-authtool kring -C --name=mon.* --add-key= --cap mon 'allow *'
  Option --add-key requires an argument
  [1]

  $ stone-authtool test.keyring --create-keyring --mode 0644
  creating test.keyring

  $ stone-authtool test.keyring --name client.test --cap osd 'allow rwx' --cap mon 'allow r' --add-key 'AQAK7yxNeF+nHBAA0SgSdbs8IkJrxroDeJ6SwQ== 18446744073709551615'
  added entity client.test auth(key=AQAK7yxNeF+nHBAA0SgSdbs8IkJrxroDeJ6SwQ==)
  added 2 caps to entity client.test
