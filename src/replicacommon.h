#include <stdio.h>

using namespace std;

struct Proposal
{
	unsigned long int proposalNumber;
	unsigned int proposerPortNumber;
	long int  value;
};

struct PrepareRequest
{
	unsigned long int proposalNumber;
	unsigned long int proposerPortNumber;
};

 class ReplicaState
 {
 // methods 
 public:
 	ReplicaState(int RID);
 	~ReplicaState();
 	bool Initialize();  // reads state from file
 	bool Flush(); 	    // writes state to file
 	PrepareRequest latestPrepareRequestRespondedTo;
	Proposal latestAcceptedProposal;

// data
private:
 	int rid;
 	FILE* stateLog;
 };

enum AcceptorState
{
	AS_TERMINATED = 0,
	AS_READY_TO_SEND_PREPARE_RESULT = 1,
	AS_WAITING_ON_PROPOSAL = 2
};

enum ProposerState
{
	PS_TERMINATED = 0,
	PS_READY_TO_SEND_PREPARE = 1,
	PS_WAITING_ON_PREPARE_QUORUM = 2,
	PS_WAITING_ON_PREPARE_RESULT_QUORUM = 3,
	PS_READY_TO_SEND_PROPOSAL = 4,
	PS_WAITINGON_PROPOSE_ACCEPTANCE = 5
};

class Acceptor
{
public:
	Acceptor();
	~Acceptor();
	bool AcceptPrepare(int proposalNumber, unsigned int port, ReplicaState** replicaState);
	void SetState(AcceptorState astate);
	AcceptorState GetState();
	bool AcceptProposal(long int pnumber, unsigned int port, long int val, ReplicaState** replicaState);
	void Disable();

private:
	AcceptorState as;
	bool disabled;
};

 class Proposer
 {
 public:
 	Proposer();
 	~Proposer();
 	unsigned long int PrepareProposalNumber(long int val);
 	unsigned long int GetCurrentProposalID();
 	void SetState(ProposerState pstate);
	ProposerState GetState();
	bool PrepareProposal(int pnum, ReplicaState** replicaState);
	Proposal GetProposal();
	int preparecount;

 private:
 	ProposerState ps;
 	unsigned long int id; 
 	Proposal currentProposal;
 	long int lastAcceptedPN;
 	int pnCount;
 };

 class Learner
 {
 public:
 	Learner();
 	~Learner();
 	void LearnAcceptedValue(long int pnumber, long int val, ReplicaState** replicaState);
 };