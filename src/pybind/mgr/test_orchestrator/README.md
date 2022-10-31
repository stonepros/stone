# Activate module
You can activate the Stone Manager module by running:
```
$ stone mgr module enable test_orchestrator
$ stone orch set backend test_orchestrator
```

# Check status
```
stone orch status
```

# Import dummy data
```
$ stone test_orchestrator load_data -i ./dummy_data.json
```
