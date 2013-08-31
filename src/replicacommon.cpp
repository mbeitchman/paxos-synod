#include "replicacommon.h"
#include <iostream>
#include <string.h>
#include <stdlib.h>

ReplicaState::ReplicaState(int RID)
{

	latestAcceptedProposal.proposalNumber = 0;
	latestAcceptedProposal.proposerPortNumber = 0;
	latestAcceptedProposal.value = -1;

	latestPrepareRequestRespondedTo.proposalNumber = 0;
	latestPrepareRequestRespondedTo.proposerPortNumber = 0;

	rid = RID;

	stateLog = NULL;
}

ReplicaState::~ReplicaState()
{
	if(stateLog)
	{
		fclose(stateLog);
		stateLog = NULL;
	}
}

bool ReplicaState::Initialize()
{
	char logName[100];
	sprintf(logName, "%d.log", rid);
	int i = 0;

	// try opening ane existing file for reading and writing
	stateLog = fopen(logName, "r+");
	if(stateLog != NULL)
	{
		char vals[100];
		fgets(vals, 100, stateLog);

		char* pch = strtok(vals,",");
		while(pch != NULL)
		{
			// latestPrepareRequestRespondedTo
			if(i == 0)
			{
				latestPrepareRequestRespondedTo.proposalNumber = atoi(pch);
			}
			else if(i == 1)
			{
				latestPrepareRequestRespondedTo.proposerPortNumber = atoi(pch);
			}
			else if(i == 2)
			{
				latestAcceptedProposal.proposalNumber = atoi(pch); 
			}
			else if(i == 3)
			{
				latestAcceptedProposal.proposerPortNumber = atoi(pch); 
			}
			else if(i == 4)
			{
				latestAcceptedProposal.value = atoi(pch);
			}

			pch = strtok(NULL,",");
			i++;
		}

		return true;
	}

	// create a new file
	Flush();
}

bool ReplicaState::Flush()
{
	char logName[100];
	sprintf(logName, "%d.log", rid);
	
	// clear file contents
	if(stateLog != NULL)
	{
		fclose(stateLog);
		stateLog = NULL;
	}

	stateLog = fopen(logName, "w+");

	char values[100];
	
	sprintf(values, "%ld,%ld,%ld,%d,%ld\n", latestPrepareRequestRespondedTo.proposalNumber,
				latestPrepareRequestRespondedTo.proposerPortNumber,
				latestAcceptedProposal.proposalNumber, 
				latestAcceptedProposal.proposerPortNumber,
				latestAcceptedProposal.value);
	
	fputs(values,stateLog);
	
	if (stateLog != NULL)
	{
		fclose(stateLog);
		stateLog = NULL;		
	}

	return true;
}

///////
Acceptor::Acceptor()
{
	as = AS_TERMINATED;
	disabled = false;
}

Acceptor::~Acceptor()
{

}

void Acceptor::Disable()
{
	disabled = true;
}

void Acceptor::SetState(AcceptorState astate)
{
	as = astate;
}

AcceptorState Acceptor::GetState()
{
	return as;
}

bool Acceptor::AcceptPrepare(int proposalNumber, unsigned int port, ReplicaState** replicaState)
{
	if(disabled)
	{
		cout << "Acceptor disabled!" << endl;
		return false;
	}

	// compare current proposal number to the last prepare request responded to
	if(proposalNumber > (*replicaState)->latestPrepareRequestRespondedTo.proposalNumber)
	{
		(*replicaState)->latestPrepareRequestRespondedTo.proposalNumber = proposalNumber;
		(*replicaState)->Flush();
		return true;
	}
	// handle proposal number ties by comparing port numbers
	else if(proposalNumber == (*replicaState)->latestPrepareRequestRespondedTo.proposalNumber &&
		port > (*replicaState)->latestPrepareRequestRespondedTo.proposerPortNumber)
	{
		(*replicaState)->latestPrepareRequestRespondedTo.proposalNumber = proposalNumber;
		(*replicaState)->Flush();
		return true;
	}

	return false;
}

bool Acceptor::AcceptProposal(long int pnumber, unsigned int port, long int val, ReplicaState** replicaState)
{
	if(disabled)
	{
		cout << "Acceptor disabled!" << endl;
		return false;
	}

	if(pnumber > (*replicaState)->latestAcceptedProposal.proposalNumber)
	{
		(*replicaState)->latestAcceptedProposal.proposalNumber = pnumber;
		(*replicaState)->latestAcceptedProposal.value = val;
		(*replicaState)->latestAcceptedProposal.proposerPortNumber = port;
		(*replicaState)->Flush();
		return true;
	}
	else if(pnumber == ((*replicaState)->latestAcceptedProposal.proposalNumber) && 
		port > (*replicaState)->latestAcceptedProposal.proposerPortNumber)
	{
		(*replicaState)->latestAcceptedProposal.proposalNumber = pnumber;
		(*replicaState)->latestAcceptedProposal.value = val;
		(*replicaState)->latestAcceptedProposal.proposerPortNumber = port;
		(*replicaState)->Flush();
		return true;
	}

	return false;
} 

////////
Proposer::Proposer()
{
	ps = PS_TERMINATED;
	id = 0;
	preparecount = 0;
	lastAcceptedPN = -1;
	pnCount = 0;
}

Proposer::~Proposer()
{
	
}

bool Proposer::PrepareProposal(int pnum, ReplicaState** replicaState)
{
	
	// need to store state and pick the value of the highest-numbered pre-parerequest 
	if(pnum >= (*replicaState)->latestAcceptedProposal.proposalNumber )
	{
		if((*replicaState)->latestAcceptedProposal.value == -1 && (*replicaState)->latestAcceptedProposal.proposalNumber == 0)
		{
			// already set
		}
		else
		{
			lastAcceptedPN = (*replicaState)->latestAcceptedProposal.proposalNumber;

			currentProposal.value = (*replicaState)->latestAcceptedProposal.value;
		}

		if(pnCount == 2)
		{
			lastAcceptedPN = 0;
			pnCount = 0;
		}

		pnCount++;

		return true;
	}

	return false;
}

Proposal Proposer::GetProposal()
{
	return currentProposal;
}

unsigned long int Proposer::PrepareProposalNumber(long int val)
{
	// update the state object for the replica??
	currentProposal.proposalNumber = ++id;
	currentProposal.value = val;
	return currentProposal.proposalNumber;
}

unsigned long int Proposer::GetCurrentProposalID()
{
	return id;
}

void Proposer::SetState(ProposerState pstate)
{
	ps = pstate;
}

ProposerState Proposer::GetState()
{
	return ps;
}

////
Learner::Learner()
{	
	// nothing to do
}

Learner::~Learner()
{	
	// nothing to do
}

void Learner::LearnAcceptedValue(long int pnumber, long int val, ReplicaState** replicaState)
{
	(*replicaState)->latestAcceptedProposal.proposalNumber = pnumber;
	(*replicaState)->latestAcceptedProposal.value = val;
	(*replicaState)->Flush();
}