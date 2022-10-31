  $ stone balancer off
  $ stone balancer mode none
  $ stone osd pool create balancer_opt 128
  pool 'balancer_opt' created
  $ stone osd pool application enable balancer_opt rados
  enabled application 'rados' on pool 'balancer_opt'
  $ rados bench -p balancer_opt 50 write --no-cleanup > /dev/null
  $ stone balancer on
  $ stone balancer mode crush-compat
  $ stone balancer ls
  []
  $ stone config set osd.* target_max_misplaced_ratio .07
  $ stone balancer eval
  current cluster score [0-9]*\.?[0-9]+.* (re)
# Turn off active balancer to use manual commands
  $ stone balancer off
  $ stone balancer optimize test_plan balancer_opt
  $ stone balancer ls
  [
      "test_plan"
  ]
  $ stone balancer execute test_plan
  $ stone balancer eval
  current cluster score [0-9]*\.?[0-9]+.* (re)
# Plan is gone after execution ?
  $ stone balancer execute test_plan
  Error ENOENT: plan test_plan not found
  [2]
  $ stone osd pool rm balancer_opt balancer_opt --yes-i-really-really-mean-it
  pool 'balancer_opt' removed
