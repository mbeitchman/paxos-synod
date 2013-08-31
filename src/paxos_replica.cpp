#include <cassert>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cstdlib>
#include <string>
#include <iostream>

#include <signal.h>
#include <pthread.h>

#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/registry.hpp>
#include <xmlrpc-c/server_abyss.hpp>
#include <xmlrpc-c/client_simple.hpp>

#include "replicacommon.h"

using namespace std;

#define PEER_URL_SIZE 100

// global state and objects
int serverPort;
int proposerPort;

// fix this up and remember to reset
int prReceivedCount = 0;
int prAcceptedCount = 0;

Acceptor acceptor;
Proposer proposer;
Learner learner;
ReplicaState* replicaState;

pthread_mutex_t rpcMutex = PTHREAD_MUTEX_INITIALIZER;

// public RPC server methods
class clientProposeMethod : public xmlrpc_c::method 
{
public:
    clientProposeMethod() {}

    void execute(xmlrpc_c::paramList const& paramList, xmlrpc_c::value* const retvalP) 
    {   
        pthread_mutex_lock(&rpcMutex);

        int const val(paramList.getInt(0));

        paramList.verifyEnd(1);

        unsigned long pn = proposer.PrepareProposalNumber(val);

        cout << "Client issued new Synod round (called propose)" << endl; 
        cout << "\tNew proposal number " << pn << " value " << val << endl;

        proposer.SetState(PS_READY_TO_SEND_PREPARE);
        
        *retvalP = xmlrpc_c::value_int(val);
        
        pthread_mutex_unlock(&rpcMutex);
    }
};

class clientLearnMethod : public xmlrpc_c::method 
{
public:
    clientLearnMethod() {}

    void execute(xmlrpc_c::paramList const& paramList, xmlrpc_c::value* const retvalP) 
    {   
        pthread_mutex_lock(&rpcMutex);

        paramList.verifyEnd(1);        
        
        int retVal = (int)replicaState->latestAcceptedProposal.value;
        cout << "Client called learn on RPC Cluster - val:" << retVal << endl; 
        *retvalP = xmlrpc_c::value_int(retVal);

        pthread_mutex_unlock(&rpcMutex);
    }
};

// internal RPC server methods
// prepare method
class prepareMethod : public xmlrpc_c::method 
{
public:
    prepareMethod() {}

    void execute(xmlrpc_c::paramList const& paramList, xmlrpc_c::value* const retvalP) 
    {    
        pthread_mutex_lock(&rpcMutex);

        int const pn(paramList.getInt(0));
        int const port(paramList.getInt(1));

        proposerPort = port;

        paramList.verifyEnd(2);

        bool result = false;


        cout << "Prepare called on port " << serverPort << "\n\t Proposal number " << pn <<  endl;

        // if acceptor accepts the prepare, send 
        // check val against the state of the server to see if there's something newer
        if(acceptor.AcceptPrepare(pn, serverPort, &replicaState))
        {       
            cout << "Acceptor on port " << serverPort << " accepted the prepare request for proposal " <<  pn << endl;
            acceptor.SetState(AS_READY_TO_SEND_PREPARE_RESULT);
            result = true;
        }
        else
        {
            cout << "Accceptor on port: " << serverPort << ". REJECTED the prepare request for proposal " <<
            pn << ".\n\tTerminate replica for this round." << endl;
            proposer.SetState(PS_TERMINATED);
            acceptor.SetState(AS_TERMINATED);
            result = false;
        }

        *retvalP = xmlrpc_c::value_boolean(result);

        pthread_mutex_unlock(&rpcMutex);
    }
};

// prepare request method
class prepareResultMethod : public xmlrpc_c::method 
{
public:
    prepareResultMethod() {}

    void execute(xmlrpc_c::paramList const& paramList, xmlrpc_c::value* const retvalP) 
    {    
        pthread_mutex_lock(&rpcMutex);

        int const pnum(paramList.getInt(0));
        long int const val(paramList.getInt(1));
        
        paramList.verifyEnd(2);

        cout << "Prepare result received by proposer on port " << serverPort << endl;
        
            proposer.PrepareProposal(proposer.GetCurrentProposalID(), &replicaState);
            prReceivedCount++;

            if(prReceivedCount >= 2)
            {
                 proposer.SetState(PS_READY_TO_SEND_PROPOSAL);
            }

        *retvalP = xmlrpc_c::value_int(pnum);

        pthread_mutex_unlock(&rpcMutex);
    }
};

// propose request method
class proposeMethod : public xmlrpc_c::method 
{
public:
    proposeMethod() {}

    void execute(xmlrpc_c::paramList const& paramList, xmlrpc_c::value* const retvalP) 
    {    
        pthread_mutex_lock(&rpcMutex);

        long int const pnumber(paramList.getInt(0));
        long int const val(paramList.getInt(1));
        bool retVal = false;
        paramList.verifyEnd(2);

        cout << "Propose received by acceptor on port " << serverPort << endl;

        long int pn = pnumber;
        long int v = val;
        if(acceptor.AcceptProposal(pn, serverPort, v, &replicaState))
        {
            cout << "\tProposal accepted (pn: " << pn << " v: " << v << ")" << endl;
            retVal = true;
        }
        else
        {
            cout << "\tProposal rejected (pn: " << pn << " v: " << v << ").\n\tTerminate replica for this round." << endl;
            proposer.SetState(PS_TERMINATED);
            acceptor.SetState(AS_TERMINATED);
        }

        *retvalP = xmlrpc_c::value_boolean(retVal);

        pthread_mutex_unlock(&rpcMutex);
    }
};

// decide request method
class decideMethod : public xmlrpc_c::method 
{
public:
    decideMethod() {}

    void execute(xmlrpc_c::paramList const& paramList, xmlrpc_c::value* const retvalP) 
    {    
        pthread_mutex_lock(&rpcMutex);

        long int const pnumber(paramList.getInt(0));
        long int const val(paramList.getInt(1));
        
        paramList.verifyEnd(2);

        long int pn = pnumber;
        long int v = val;

        cout << "Decide received by acceptor on port " << serverPort << endl;
        cout << "\t Learned values (pn: " << pn << " v: " << v << ")" << endl;
        learner.LearnAcceptedValue(pn, v, &replicaState);

        *retvalP = xmlrpc_c::value_boolean(true);

        pthread_mutex_unlock(&rpcMutex);
    }
};
//test rpc method
class disableMethod : public xmlrpc_c::method 
{
public:
    disableMethod() {}

    void execute(xmlrpc_c::paramList const& paramList, xmlrpc_c::value* const retvalP) 
    {    
        pthread_mutex_lock(&rpcMutex);

        long int const pnumber(paramList.getInt(0));
        
        paramList.verifyEnd(1);

        acceptor.Disable();

        *retvalP = xmlrpc_c::value_boolean(true);

        pthread_mutex_unlock(&rpcMutex);
    }
};

////////////////////
void ctrlcHandler(int sig)
{
    // clean-up
    delete replicaState;
    printf("\nClosing Replica. Goodbye!\n");
    exit(0);
}

void AddMethodsToRPCRegistry(xmlrpc_c::registry* myRegistry)
{
    // add rpc methods to the registry
    xmlrpc_c::methodPtr const clientProposeMethodP(new clientProposeMethod);
    xmlrpc_c::methodPtr const clientLearnMethodP(new clientLearnMethod);
    xmlrpc_c::methodPtr const prepareMethodP(new prepareMethod);
    xmlrpc_c::methodPtr const prepareResultMethodP(new prepareResultMethod);
    xmlrpc_c::methodPtr const proposeMethodP(new proposeMethod);
    xmlrpc_c::methodPtr const decideMethodP(new decideMethod);
    xmlrpc_c::methodPtr const disableMethodP(new disableMethod);

    myRegistry->addMethod("replica.public.propose", clientProposeMethodP);
    myRegistry->addMethod("replica.public.learn", clientLearnMethodP);
    myRegistry->addMethod("replica.private.prepare", prepareMethodP);
    myRegistry->addMethod("replica.private.prepareresult", prepareResultMethodP);
    myRegistry->addMethod("replica.private.propose", proposeMethodP);
    myRegistry->addMethod("replica.private.decide", decideMethodP);
    myRegistry->addMethod("replica.test.disable", disableMethodP);
}

void Usage()
{
    cout << "Paxos_Replica Usage:" << endl;
    cout << "./paxos_replica -hostport [host port of server] -peerport1 [peerport number] -peerport2 [peerport number]" << endl;
    cout << "./paxos_replica -hostport 8080 -peerport1 8081 -peerport2 8082" << endl;
} 

int main(int argc, char ** argv) 
{
    bool bDefaultHosts = true;
    int peer1Port = 0;
    int peer2Port = 0;

    char* host1 = NULL;
    char* host2 = NULL;
    char* host3 = NULL;

    // parse arguments
    if(argc < 7 || argc > 13)
    {
        Usage();
        return 1;
    }
    else
    {
        for(int i = 1; i < argc; i+=2)
        {
            if(strcasecmp("-hostport", argv[i]) == 0)
            {
                serverPort = atoi(argv[i+1]);
            }
            else if(strcasecmp("-peerport1", argv[i]) == 0)
            {
                peer1Port = atoi(argv[i+1]);   
            }
            else if(strcasecmp("-peerport2", argv[i]) == 0)
            {
                peer2Port = atoi(argv[i+1]);   
            }
            else if(strcasecmp("-hosts", argv[i]) == 0)
            {
                host1 = argv[i+1];
            }
            else if(strcasecmp("-peers1", argv[i]) == 0)
            {
                host2 = argv[i+1];   
            }
            else if(strcasecmp("-peers2", argv[i]) == 0)
            {
                host3 = argv[i+1];
            }
            else
            {
                Usage();
                return 1;
            }
        }

    }

    signal(SIGINT, ctrlcHandler);

    if(host1 != NULL && host2 != NULL && host3 != NULL)
    {
        bDefaultHosts = false;
    }

    //
    // Set-up Replics
    //

    // set-up the registry for the rpc server
    xmlrpc_c::registry myRegistry;

    AddMethodsToRPCRegistry(&myRegistry);

    // create the rpc server
    xmlrpc_c::serverAbyss myAbyssServer(myRegistry, serverPort, "xmlrpc_log");

    // set-up 'client' varibles needed for calling other replicas
    // todo: do i need multiple clients here?
    xmlrpc_c::clientSimple myClient1;
    xmlrpc_c::clientSimple myClient2;
    xmlrpc_c::clientSimple myClient3;

    // set-up urls to contact replics
    char sURL[PEER_URL_SIZE];
    char p1URL[PEER_URL_SIZE];
    char p2URL[PEER_URL_SIZE];
    
    if(bDefaultHosts)
    {
        sprintf(sURL, "http://localhost:%d/RPC2", serverPort);
        sprintf(p1URL, "http://localhost:%d/RPC2", peer1Port);
        sprintf(p2URL, "http://localhost:%d/RPC2", peer2Port);
    }
    else
    {
        sprintf(sURL, "http://%s:%d/RPC2", host1, serverPort);
        sprintf(p1URL, "http://%s:%d/RPC2", host2, peer1Port);
        sprintf(p2URL, "http://%s:%d/RPC2", host3, peer2Port);
    }

    string const selfURL(sURL);
    string const peer1Url(p1URL);
    string const peer2Url(p2URL);

    string const publicPropseMethod("replica.public.propose");
    string const publicLearnMethod("replica.public.learn");
    string const privatePrepareMethod("replica.private.prepare");
    string const privatePrepareResultMethod("replica.private.prepareresult");
    string const privateProposeMethod("replica.private.propose");
    string const privateDecideMethod("replica.private.decide");
   
    // initialize replica state and agents
    replicaState = new ReplicaState(serverPort);
    if(!(replicaState->Initialize()))
    {
        raise(SIGINT);
    }

    while (true) 
    {
        xmlrpc_c::value result1;
        xmlrpc_c::value result2;
        xmlrpc_c::value result3;
        xmlrpc_c::value result4;
        xmlrpc_c::value result5;
        xmlrpc_c::value result6;
        xmlrpc_c::value result7;
        Proposal p;
        string propURL;
        bool p1PAcceptedResult;
        bool p2PAcceptedResult;
        int pcount = 0;

        cout << "Waiting for next RPC..." << endl;

        /* This waits for the next connection, accepts it, reads the
           HTTP POST request, executes the indicated RPC, and closes
           the connection.
        */
        myAbyssServer.runOnce();

        try
        {
            // check state of each agent and determine what to do
            switch(proposer.GetState())
            {

                // send prepare requests to other replicas
                case PS_READY_TO_SEND_PREPARE:
                                                                   

                    myClient2.call(peer1Url, privatePrepareMethod, "ii", &result2, proposer.GetCurrentProposalID(), serverPort);
                    myClient3.call(peer2Url, privatePrepareMethod, "ii", &result3, proposer.GetCurrentProposalID(), serverPort);

                    if(result2.cValue())
                    {
                        pcount++;
                    }

                    if(result3.cValue())
                    {
                        pcount++;
                    }

                    // local acceptor
                    if(acceptor.AcceptPrepare(proposer.GetCurrentProposalID(), serverPort, &replicaState))
                    {
                       cout << "Acceptor on port " << serverPort << " accepted the prepare request for proposal " <<
                            proposer.GetCurrentProposalID() << endl;
                        pcount++;
                    }

                    if(pcount >= 2 )
                    {
                        cout << "Prepare requets accpted" << endl;
                        proposer.SetState(PS_WAITING_ON_PREPARE_QUORUM);
                    }
                    else
                    {
                        cout << "Prepare requets NOT accpted" << endl;
                        proposer.SetState(PS_TERMINATED);
                        acceptor.SetState(AS_TERMINATED);
                    }

                    break;

                case PS_READY_TO_SEND_PROPOSAL:

                    proposer.PrepareProposal(proposer.GetCurrentProposalID(),
                                            &replicaState);

                    p = proposer.GetProposal();
                    // prepare the proposal which is current proposal number, highest previous proposal value
                    cout << "ready to send proposal \n\tproposal number: " << p.proposalNumber << " value: " << p.value << endl;
                    myClient1.call(peer1Url, privateProposeMethod, "ii", &result2, p.proposalNumber, p.value);
                    myClient3.call(peer2Url, privateProposeMethod, "ii", &result3, p.proposalNumber, p.value);

                    // check return value to see if accepted
                    // if both accepted
                    // send decide to learners
                    if(acceptor.AcceptProposal(p.proposalNumber, serverPort, p.value, &replicaState))
                    {
                        prAcceptedCount++;
                    }

                    p1PAcceptedResult = result2.cValue();
                    if(p1PAcceptedResult)
                    {
                        prAcceptedCount++;
                    }

                    p2PAcceptedResult = result3.cValue();
                    if(p2PAcceptedResult)
                    {
                        prAcceptedCount++;
                    }

                    if(prAcceptedCount >= 2)
                    {
                        cout << "Proposal accepted by proposer, decide sent to learners" << endl;
                        myClient1.call(peer1Url, privateDecideMethod, "ii", &result6, p.proposalNumber, p.value);
                        myClient3.call(peer2Url, privateDecideMethod, "ii", &result7, p.proposalNumber, p.value);
                        learner.LearnAcceptedValue(p.proposalNumber, p.value, &replicaState);
                        cout << "Decide received by acceptor on port " << serverPort << 
                        "\n\t Learned Values (pn: " << p.proposalNumber << " v: " << p.value << ")" << endl;
                        replicaState->Flush();
                    }
                    else
                    {
                        cout << "not a mjorit for peropisal number " << p.proposalNumber << endl;
                    }

                    proposer.SetState(PS_TERMINATED);
                    acceptor.SetState(AS_TERMINATED);

                    break;


                default:
                    break;
            }

            switch(acceptor.GetState())
            {
                case AS_READY_TO_SEND_PREPARE_RESULT:

                    char pURL[PEER_URL_SIZE];
                    sprintf(pURL, "http://localhost:%d/RPC2", proposerPort);
                    propURL = pURL;

                    cout << "Prepare Result called by acceptor on port " << serverPort << endl;
                    // we need to send the value of the highest number pn, val pair where pn < currentproposal id
                    myClient2.call(propURL, privatePrepareResultMethod, "ii", &result4, proposer.GetCurrentProposalID(), 
                        replicaState->latestAcceptedProposal.value);

                    prReceivedCount++;

                    acceptor.SetState(AS_WAITING_ON_PROPOSAL);

                    break;

                default:
                    break;
            }

            if(proposer.GetState() == PS_TERMINATED)
            {
                prReceivedCount = 0;
                prAcceptedCount = 0;
                pcount = 0;
            }

            if(acceptor.GetState() == AS_TERMINATED)
            {
                prAcceptedCount = 0;  
                prAcceptedCount = 0; 
                pcount = 0;
            }
       } 
       catch (exception const& e) 
        {
            cerr << "Exception: unable to connect to peers. Ending round! "<< endl;
            proposer.SetState(PS_TERMINATED);
            acceptor.SetState(AS_TERMINATED);
            prAcceptedCount = 0;
            prReceivedCount = 0;
        }
    }

    return 0;
}