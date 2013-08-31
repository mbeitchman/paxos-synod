#include <cstdlib>
#include <iostream>
#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/client_simple.hpp>
#include <stdio.h>
#include <string.h>

using namespace std;

#define PEER_URL_SIZE 100

void Usage()
{   

    cout << "Paxos_Client Usage:" << endl << endl;
    cout << "Manual value proposal to cluster:" << endl;
    cout << "./paxos_client -cluster_port1 [port of replica to contact] -proposal_value [val] -learn" << endl;
    cout << "ex: ./paxos_client -cluster_port1 8082 -proposal_value 99 -learn" << endl << endl;

    cout << "Running tests:" << endl;
    cout << "./paxos_client -cluster_port1 [portnum] -cluster_port2 [portnum] -cluster_port3 [portnum] -test [testid]" << endl;
    cout << "ex: ./paxos_client -cluster_port1 8080 -cluster_port2 8081 -cluster_port3 8082 -test 3" << endl;
}

int main(int argc, char **argv) 
{
    int cport1 = -1;
    int cport2 = -1;
    int cport3 = -1;
    unsigned int testID = -1;
    int proposalValue = -1;
    char* chost = NULL;
    char cURL1[PEER_URL_SIZE];
    char cURL2[PEER_URL_SIZE];
    char cURL3[PEER_URL_SIZE];
    bool blearn = false;
    bool bproposevalue = false;

    if(argc > 10)
    {
        Usage();
        return 1;
    }

    // parse arguments
    for(int i = 1; i < argc; i+=2)
    {
        if(strcasecmp(argv[i], "-cluster_port1") == 0)
        {
            cport1 = atoi(argv[i+1]);
        }
        else if(strcasecmp(argv[i], "-cluster_port2") == 0)
        {
            cport2 = atoi(argv[i+1]);
        }
        else if(strcasecmp(argv[i], "-cluster_port3") == 0)
        {
            cport3 = atoi(argv[i+1]);
        }
        else if(strcasecmp(argv[i], "-cluster_server") == 0)
        {
            chost = argv[i+1];
        }
        else if(strcasecmp(argv[i], "-proposal_value") == 0)
        {
            bproposevalue = true;
            proposalValue = atoi(argv[i+1]);
        }
        else if(strcasecmp(argv[i], "-learn") == 0)
        {
            blearn = true;
        }
        else if(strcasecmp(argv[i], "-test") == 0)
        {
            testID = atoi(argv[i+1]);
        }
        else
        {
            Usage();
            return 1;
        }
    }

    // verify at least one port is specified
    if(cport1 < 0)
    {
        Usage();
        return 1;
    }

    // determine the url of the rpc sever based on whether 
    // we have been given a host name or just the port number
    if(chost)
    {
        sprintf(cURL1, "http://%s:%d/RPC2", chost, cport1);
        sprintf(cURL2, "http://%s:%d/RPC2", chost, cport2);
        sprintf(cURL3, "http://%s:%d/RPC2", chost, cport3);
    }
    else
    {
        sprintf(cURL1, "http://localhost:%d/RPC2", cport1);
        sprintf(cURL2, "http://localhost:%d/RPC2", cport2);
        sprintf(cURL3, "http://localhost:%d/RPC2", cport3);
    }

    // declare the rpc methods to call on the cluster
    string const paxosClusterPropose("replica.public.propose");
    string const paxosClusterLearn("replica.public.learn");
    string const paxosClusterDisableAcceptor("replica.test.disable");
    
    // rpc cliente
    xmlrpc_c::clientSimple myClient;
    
    // xml-rpc return values are immutable and cannot be reused
    xmlrpc_c::value presult1;
    xmlrpc_c::value presult2;
    xmlrpc_c::value presult3;
    xmlrpc_c::value presult4;
    xmlrpc_c::value presult5;
    xmlrpc_c::value presult6;
    xmlrpc_c::value presult7;
    xmlrpc_c::value lresult;

    // run the client
    try
    {
        // manual mode
        if(testID == -1)
        {
            // propose a value to one replica of the cluster 
            if(bproposevalue)
            {
    	       cout << "Proposing value " << proposalValue << " to the Paxos Cluster" << endl;
    	       myClient.call(cURL1, paxosClusterPropose, "i", &presult1, proposalValue);

                sleep(2);
            }

            // learn the value that is chosen by the paxos cluster
            if(blearn)
            {
                myClient.call(cURL1, paxosClusterLearn, "i", &lresult, proposalValue);
                int const lr((xmlrpc_c::value_int(lresult)));
                cout << "Learned value " <<  lr << " from the Paxos cluster." << endl;
            }
        }
        else
        {
            // test mode
            cout << "Running test id " << testID << endl;
            int result;

            switch(testID)
            {
                // propose many values to 1 replica and learn and verify the result
                case 0:

                    if(cport1 == -1)
                    {
                        cout << "This test requires that 1 port is specified!" << endl;
                        return 1;
                    }

                    cout << "This test makes multiple prepare requests to the same replica. And verifies the correct value is learned." << endl;
                    
                    cout << "Proposing value " << 100 << " to the Paxos Cluster at replica " << cURL1 << endl;
                    myClient.call(cURL1, paxosClusterPropose, "i", &presult1, 100);
                    sleep(1);

                    cout << "Proposing value " << 10 << " to the Paxos Cluster at replica " << cURL1 << endl;
                    myClient.call(cURL1, paxosClusterPropose, "i", &presult2, 10);
                    sleep(1);

                    cout << "Proposing value " << 20 << " to the Paxos Cluster at replica " << cURL1 << endl;
                    myClient.call(cURL1, paxosClusterPropose, "i", &presult3, 20);
                    sleep(1);

                    cout << "Proposing value " << 30 << " to the Paxos Cluster at replica " << cURL1 << endl;
                    myClient.call(cURL1, paxosClusterPropose, "i", &presult4, 30);
                    sleep(1);

                    myClient.call(cURL1, paxosClusterLearn, "i", &lresult, proposalValue);
                    result = xmlrpc_c::value_int(lresult);
                    
                    cout << "Learned value " <<  result << " from the Paxos cluster." << endl;

                    if(result == 100)
                    {
                        cout << "Test passed. (100 == 100)" << endl;
                    }
                    else
                    {
                        cout << "Test FAILED. (100 !=" << result << ")" << endl;
                    }

                    break;
                
                // propose 2 values to 1 replica and 1 value to another replic and learn and verify the result
                case 1:

                    if(cport2 == -1 || cport1 == -1)
                    {
                        cout << "This test requires that 2 ports are specified!" << endl;
                        return 1;
                    }

                    cout << "This test makes multiple prepare requests to one replica and then one prepare request to another replica to verify the correct value is learned." << endl;
                    
                    cout << "Proposing value " << 100 << " to the Paxos Cluster " << cURL1 << endl;
                    myClient.call(cURL1, paxosClusterPropose, "i", &presult1, 100);
                    sleep(1);

                    cout << "Proposing value " << 20 << " to the Paxos Cluster " << cURL2 << endl;
                    myClient.call(cURL2, paxosClusterPropose, "i", &presult2, 20);
                    sleep(1);

                    cout << "Proposing value " << 10 << " to the Paxos Cluster " << cURL2 << endl;
                    myClient.call(cURL2, paxosClusterPropose, "i", &presult3, 10);
                    sleep(1);

                    myClient.call(cURL1, paxosClusterLearn, "i", &lresult, proposalValue);
                    result = xmlrpc_c::value_int(lresult);
                    
                    cout << "Learned value " <<  result << " from the Paxos cluster." << endl;

                    if(result == 100)
                    {
                        cout << "Test passed. (100 == 100)" << endl;
                    }
                    else
                    {
                        cout << "Test FAILED. (100 !=" << result << ")" << endl;
                    }

                    break;
                
                // propose 2 values to 1 replica, 2 values to another replic, and 1 value to final repolica
                // and learn and verify the result
                case 2:

                    if(cport2 == -1 || cport1 == -1)
                    {
                        cout << "This test requires that 2 ports are specified!" << endl;
                        return 1;
                    }

                    cout << "This test makes multiple prepare requests to one replica and and then multiple prepare requests to another replica and verifies the correct value is learned." << endl;
                    
                    cout << "Proposing value " << 3 << " to the Paxos Cluster to replica at " << cURL1 << endl;
                    myClient.call(cURL1, paxosClusterPropose, "i", &presult1, 3);
                    sleep(1);

                    cout << "Proposing value " << 15 << " to the Paxos Cluster to replica at " << cURL1 << endl;
                    myClient.call(cURL1, paxosClusterPropose, "i", &presult2, 10);
                    sleep(1);

                    cout << "Proposing value " << 20 << " to the Paxos Cluster to replica at " << cURL2 << endl;
                    myClient.call(cURL2, paxosClusterPropose, "i", &presult3, 20);
                    sleep(1);
                    
                    cout << "Proposing value " << 50 << " to the Paxos Cluster to replica at " << cURL2 << endl;
                    myClient.call(cURL2, paxosClusterPropose, "i", &presult4, 50);
                    sleep(1);

                    cout << "Proposing value " << 200 << " to the Paxos Cluster to replica at " << cURL3 << endl;
                    myClient.call(cURL3, paxosClusterPropose, "i", &presult5, 200);
                    sleep(1);

                    myClient.call(cURL1, paxosClusterLearn, "i", &lresult, proposalValue);
                    result = xmlrpc_c::value_int(lresult);
                    
                    if(result == 3)
                    {
                        cout << "Test passed. (3 == 3)" << endl;
                    }
                    else
                    {
                        cout << "Test FAILED. (3 !=" << result << ")" << endl;
                    }

                    cout << "Learned value " <<  result << " from the Paxos cluster." << endl;

                    break;

                // Disable one acceptor propose 5 values to 1 replica and learn and verify the result.
                case 3:

                    if(cport3 == -1 || cport2 == -1 || cport1 == -1)
                    {
                        cout << "This test requires that 3 ports are specified!" << endl;
                        return 1;
                    }

                    cout << "This test makes 1 of the 3 acceptors always return false for prepare. The result is a value is never learned.";

                    cout << "disable acceptor on  " << cURL1 << endl;
                    myClient.call(cURL1, paxosClusterDisableAcceptor, "i", &presult1, 100);
                    sleep(1);

                    cout << "Proposing value " << 10 << " to the Paxos Cluster at replica " << cURL1 << endl;
                    myClient.call(cURL1, paxosClusterPropose, "i", &presult2, 10);
                    sleep(1);

                    cout << "Proposing value " << 40 << " to the Paxos Cluster at replica " << cURL1 << endl;
                    myClient.call(cURL1, paxosClusterPropose, "i", &presult3, 40);
                    sleep(1);

                    cout << "Proposing value " << 41 << " to the Paxos Cluster at replica " << cURL1 << endl;
                    myClient.call(cURL1, paxosClusterPropose, "i", &presult4, 41);
                    sleep(1);
                                        
                    cout << "Proposing value " << 42 << " to the Paxos Cluster at replica " << cURL1 << endl;
                    myClient.call(cURL1, paxosClusterPropose, "i", &presult5, 42);
                    sleep(1);
                    
                    cout << "Proposing value " << 43 << " to the Paxos Cluster at replica " << cURL1 << endl;
                    myClient.call(cURL1, paxosClusterPropose, "i", &presult6, 43);
                    sleep(1);

                    myClient.call(cURL1, paxosClusterLearn, "i", &lresult, proposalValue);
                    result = xmlrpc_c::value_int(lresult);
                    
                    if(result == 10)
                    {
                        cout << "Test passed. (10 == 10)" << endl;
                    }
                    else
                    {
                        cout << "Test FAILED. (10 !=" << result << ")" << endl;
                    }

                    cout << "Learned value " <<  result << " from the Paxos cluster." << endl;

                    break;

                // Disable two acceptors propose 3 values to 1 replica and learn and verify the result.
                case 4:

                    if(cport3 == -1 || cport2 == -1 || cport1 == -1)
                    {
                        cout << "This test requires that 3 ports are specified!" << endl;
                        return 1;
                    }

                    cout << "This test makes 2 of the 3 acceptors always return false for prepare. The result is a value is never learned.";

                    cout << "disable acceptor on  " << cURL2 << endl;
                    myClient.call(cURL2, paxosClusterDisableAcceptor, "i", &presult1, 100);
                    sleep(1);

                    cout << "disable acceptor on  " << cURL3 << endl;
                    myClient.call(cURL3, paxosClusterDisableAcceptor, "i", &presult2, 100);
                    sleep(1);

                    cout << "Proposing value " << 10 << " to the Paxos Cluster at replica " << cURL1 << endl;
                    myClient.call(cURL1, paxosClusterPropose, "i", &presult3, 10);
                    sleep(1);

                    cout << "Proposing value " << 20 << " to the Paxos Cluster at replica " << cURL1 << endl;
                    myClient.call(cURL1, paxosClusterPropose, "i", &presult4, 20);
                    sleep(1);

                    cout << "Proposing value " << 30 << " to the Paxos Cluster at replica " << cURL1 << endl;
                    myClient.call(cURL1, paxosClusterPropose, "i", &presult5, 30);
                    sleep(1);

                    myClient.call(cURL1, paxosClusterLearn, "i", &lresult, proposalValue);
                    result = xmlrpc_c::value_int(lresult);
                    
                    if(result == -1)
                    {
                        cout << "Learned value DONT_KNOW from the Paxos cluster." << endl;
                        cout << "Test passed." << endl;
                    }
                    else
                    {
                        cout << "Learned value "<< result <<" from the Paxos cluster." << endl;
                        cout << "Test FAILED." << endl;
                    }

                    break;

                default:
                    cout << "Unknown test id!" << endl;
                    break;
            }
        }
    } 
    catch (exception const& e) 
    {
        cerr << "Could not connect to Paxos Cluster!" << endl;
    }

    return 0;
}
