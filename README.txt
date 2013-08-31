Marc Beitchman
marcbei@uw.edu
1141400
CSEp551
Project 3

Sections:

I. Overview
II. Code Structure
III. Challenges
IV. Testing
V. Library Installation and Build Steps
VI. Running test script
VII. Running client manually
VIII. Running server manually
IX. Collaboration

I. Overview:

I wrote my Paxos implementing using C++ and the XML-RPC library from http://xmlrpc-c.sourceforge.net/. This RPC library uses XML as the data transfer format and transmits requests using http.  I spent time considering using rpcgen to generate my RPC stubs to get familiar with a classic RPC library but decided against it do the limitations of not being able to run my solution on attu. I did most of my development work on a MacBook pro and periodically ran my solution on attu to ensure compatability. 

II. Code Structure:

The client (paxos_client) I implemented provides the ability to call propose and learn on the paxos cluster. The client also implements 5 test cases. There is one additional RPC made by that client that is a RPC test hook. This call disables acceptor components of a server replica to ensure the cluster behaves correctly. The code all lives in main() and parses parameters to configure itself and run.

The server (paaxos_replica) component exposes the public RPC interface which consists of Propose and Learn() and exposes the private RPC interface that implements the following methods: Prepare(), PrepareResult(), Propose() and Decide(). The server also exposes the disable RPC to disable the acceptor on a replica.

The replica goes through set-up which includes parsing parameters, registering the RPC server, creating and initializing the replica from state if there exists a state log. Each replica writes durable state to a final when changing state. After set-up, the replica enters an event loop. In the loop, we block on a RunOnce() call to the RPC server. RunOnce() handles one RPC connection at a time. After an RPC call is received, I check the state of my proposer, acceptor and learner which are each implemented as separate classes in replicacommon.h and replicacommon.cpp. Actions are performed based on the state of each component.

III. Challenges:

I hit a bit of a time sync trying to find a good RPC library that works on attu. Rpcgen worked on my Mac but not on attu due to permissions issues. I would have had more development time if I started with XML-RPC. When I moved my code to attu, I hit bugs due to differences in things like what FILE*'s are set to after calling fclose and how the compiler sets initialized variables. These issues were interesting to find and solve but it would have been much more efficient had I found these bugs when I was developing the feature instead of during the final verification stage.

Another issue I had was I wanted to handle time outs and simulate the affects of a slow intermittent replica. I found out about async sockets too late to change my implementation. I also rolled my own time out mechanism but wasn't able to get it to work well so I decided to focus on stability and leave it out. 

I found the project really interesting and challenging and would consider working more on my solution to improve it.

IV. Testing:

I wrote the following 5 test cases. 

test 0: Proposes 4 values to 1 replica and learn and verify the result.

test 1: Proposes 2 values to 1 replica and 1 value to another replica and learn and verify the result.

test 2: Proposes 2 values to 1 replica, 2 values to another replica, and 1 value to final replica
        and learn and verify the result.

test 3: Disable one acceptor propose 5 values to 1 replica and learn and verify the result.

test 4: Disable two acceptors propose 3 values to 1 replica and learn and verify the result.

I also did some manual testing such as adding random sleeps and kill a replica during execution which exposed the issue that requests in my implementation do not timeout.

The easiest way to run the test cases are with the RunTests script which launch three replicas in the background and call the client to run each test. The call sequence is a bit unclear since all output is intermingled on one command prompt. The tests can be run manually by opening 4 command prompts and running each replica and the client to see the output of each component individually.

V. Library Installation and Build steps:

Do the following from the src directory:

1) run the install script to build and install XML-RPC 

ex: ./installxmlrpc

2) run make to build the client and the server

VI. Running test script

run tests using the following script a test number from 0-4 as the argument

ex: ./runtests 3

VII. Running client manually

Manual value proposal to cluster: 
./paxos_client -cluster_port1 [port of replica to contact] -proposal_value [val] -learn
ex: ./paxos_client -cluster_port1 8082 -proposal_value 99 -learn

Running tests
./paxos_client -cluster_port1 [portnum] -cluster_port2 [portnum] -cluster_port3 [portnum] -test [testid]
ex: ./paxos_client -cluster_port1 8080 -cluster_port2 8081 -cluster_port3 8082 -test 3

VIII. Running servers manually

./paxos_replica -hostport [host port of server] -peerport1 [peerport number] -peerport2 [peerport number]

3 replicas:
ex: 
./paxos_replica -hostport 8080 -peerport1 8081 -peerport2 8082
./paxos_replica -hostport 8081 -peerport1 8080 -peerport2 8082
./paxos_replica -hostport 8082 -peerport1 8081 -peerport2 8080

Note: This uses localhost.

IX. Collaboration

discussed the assignment with Brad Burkett