# Distributed GUID2HomeServer Routing (kinda DHT)
## Home-server
* Home-servers store encrypted userdata, provide domain lookup via network
of routing servers.
* On registration user is assigned GUID.
* Users can also migrate between Home-servers.
* Home-server's domain name is considered it's ID (GHID = domain).
* Each home-server should also be a routing-server.

## Routing server
* Network of routing-servers (aka nodes) provide GUID2HomeServer lookup service for Home-servers.
* Each routing-servers generates ID (GRID) when joining network.
* Routing network is (assumed to be) a connected graph of nodes that store
their neigbours' GRIDs and domains/IPs

## Routing algorithm
##### Joining routing network
Find closest node/nodes with greedy search
##### Register GUID for lookup
From a few entry points to routing network few best (min_distance[GUID, GRID]) nodes via gready search.
##### GUID2HomeServer lookup
Search (greedy) for promising nodes (min_distance[GUID, GRID]).
##### Safely Leaving routing network
Acquaint neigbours (to prevent network fragmentation) 
##### Handling killed/unstable nodes
Do something, if node's lost too many neigbours?
##### Handling GUID conflicts & outdated info
Voting for correct answer?
## AsyncRoutingNode Implementation
NOT DISCUSSED YET

For now, code should be executed inside of sandbox that emulate network of 
routing servers, e.g.
```
class RoutingSandbox:
    RoutingNode* NewNode();
    void RunNode(GRID, int exec_every_n_ticks); // -1 means halt
    void RemoveNode(GRID); // call RoutingNode::Leave
    void KillNode(GRID); // kill without calling RoutingNode::Leave
    
    void RegisterGUID(GRID entry_points[], GUID dst_guid, Domain dst_domain);
    void RouteGUID(GRID entry_points[], GUID dst_guid, Domain expected_domain); 
    

class Connection:
    void Request(std:function& request_callback);
    void Accept(std:function& accept_callback);
    void Send(Data& data, std::function& send_callback);
    void Listen(std::function& listen_callback);

```
Connection class is used by
```
class RoutingNode:
    GRID id;

    // API
    void Join(ServerInfo entry_points[]); // start joining routing network
    void Leave(); // correctly leave routing network (acquaint neighbours) 
    
    // Callbacks that asyncronously handle routing 
    void OnSomeAsyncEvent(args);
    void OnSomeOtherAsyncEvent(other_args);
```
Callbacks are easy to replace with Qt-signals/slots.
## Remarks
* For now, ID-collisions are considered impossible
* Do we need username2domain routing instead?
