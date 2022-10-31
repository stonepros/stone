# Testing

## To test the k8sevents module  
enable the module with `stone mgr module enable k8sevents`  
check that it's working `stone k8sevents status`, you should see something like this;  
```
[root@stone-mgr ~]# stone k8sevents status
Kubernetes
- Hostname : https://localhost:30443
- Namespace: stone
Tracker Health
- EventProcessor : OK
- StoneConfigWatcher : OK
- NamespaceWatcher : OK
Tracked Events
- namespace  :   5
- stone events:   0

```  
Now run some commands to generate healthchecks and admin level events;  
- ```stone osd set noout```
- ```stone osd unset noout```
- ```stone osd pool create mypool 4 4 replicated```
- ```stone osd pool delete mypool mypool --yes-i-really-really-mean-it```  

In addition to tracking audit, healthchecks and configuration changes if you have the environment up for >1 hr you should also see and event that shows the clusters health and configuration overview.

As well as status, you can use k8sevents to see event activity in the target kubernetes namespace
```
[root@rhcs4-3 kube]# stone k8sevents ls 
Last Seen (UTC)       Type      Count  Message                                              Event Object Name
2019/09/20 04:33:00   Normal        1  Pool 'mypool' has been removed from the cluster      mgr.ConfigurationChangeql2hj
2019/09/20 04:32:55   Normal        1  Client 'client.admin' issued: stone osd pool delete   mgr.audit.osd_pool_delete_
2019/09/20 04:13:23   Normal        2  Client 'mds.rhcs4-2' issued: stone osd blacklist      mgr.audit.osd_blacklist_
2019/09/20 04:08:28   Normal        1  Stone log -> event tracking started                   mgr.k8sevents-moduleq74k7
Total :   4
```  
or, focus on the stone specific events(audit & healthcheck) that are being tracked by the k8sevents module.
```
[root@rhcs4-3 kube]# stone k8sevents stone
Last Seen (UTC)       Type      Count  Message                                              Event Object Name
2019/09/20 04:32:55   Normal        1  Client 'client.admin' issued: stone osd pool delete   mgr.audit.osd_pool_delete_
2019/09/20 04:13:23   Normal        2  Client 'mds.rhcs4-2' issued: stone osd blacklist      mgr.audit.osd_blacklist_
Total :   2
```

## Sending events from a standalone Stone cluster to remote Kubernetes cluster
To test interaction from a standalone stone cluster to a kubernetes environment, you need to make changes on the kubernetes cluster **and** on one of the mgr hosts.
### kubernetes (minikube)
We need some basic RBAC in place to define a serviceaccount(and token) that we can use to push events into kubernetes. The `rbac_sample.yaml` file provides a quick means to create the required resources. Create them with `kubectl create -f rbac_sample.yaml`
  
Once the resources are defined inside kubernetes, we need a couple of things copied over to the Stone mgr's filesystem.
### stone admin host
We need to run some commands against the cluster, so you'll needs access to a stone admin host. If you don't have a dedicated admin host, you can use a mon or mgr machine. We'll need the root ca.crt of the kubernetes API, and the token associated with the service account we're using to access the kubernetes API.  

1. Download/fetch the root ca.crt for the kubernetes cluster (on minikube this can be found at ~/minikube/ca.crt)
2. Copy the ca.crt to your stone admin host
3. Extract the token from the service account we're going to use
```
kubectl -n stone get secrets -o jsonpath="{.items[?(@.metadata.annotations['kubernetes\.io/service-account\.name']=='stone-mgr')].data.token}"|base64 -d > mytoken
```  
4. Copy the token to your stone admin host
5. On the stone admin host, enable the module with `stone mgr module enable k8sevents`
6. Set up the configuration
```
stone k8sevents set-access cacrt -i <path to ca.crt file>
stone k8sevents set-access token -i <path to mytoken>
stone k8sevents set-config server https://<kubernetes api host>:<api_port>
stone k8sevents set-config namespace stone
```
7. Restart the module with `stone mgr module disable k8sevents && stone mgr module enable k8sevents`
8. Check state with the `stone k8sevents status` command
9. Remove the ca.crt and mytoken files from your admin host

To remove the configuration keys used for external kubernetes access, run the following command
```
stone k8sevents clear-config  
```

## Networking
You can use the above approach with a minikube based target from a standalone stone cluster, but you'll need to have a tunnel/routing defined from the mgr host(s) to the minikube machine to make the kubernetes API accessible to the mgr/k8sevents module. This can just be a simple ssh tunnel.    
