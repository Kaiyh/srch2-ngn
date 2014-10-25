
#include "NodeJoiner.h"
#include "../../metadata_manager/MetadataInitializer.h"
#include "../AtomicMetadataCommit.h"
#include "../AtomicLock.h"
#include "../AtomicRelease.h"
#include "../../state_machine/StateMachine.h"
#include "../../state_machine/node_iterators/ConcurrentNotifOperation.h"

#include "core/util/Logger.h"

using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {



void NodeJoiner::join(){
	NodeJoiner * joiner = new NodeJoiner();
	Transaction::startTransaction(joiner);
}


NodeJoiner::~NodeJoiner(){
	if(locker != NULL){
		delete locker;
	}
	if(releaser != NULL){
		delete releaser;
	}
	if(metadataChange != NULL){
		delete metadataChange;
	}
	if(committer != NULL){
		delete committer;
	}
    Logger::debug("DETAILS : Node Joiner is deleting.");
}

NodeJoiner::NodeJoiner(){
	this->finalizedFlag = false;
	this->selfOperationId = NodeOperationId(ShardManager::getCurrentNodeId(), this->getTID());
	Logger::debug("DETAILS : Join Op ID : %s", this->selfOperationId.toString().c_str());
	this->randomNodeToReadFrom = 0;
	this->locker = NULL;
	this->readMetadataNotif = SP(MetadataReport::REQUEST)(new MetadataReport::REQUEST());
	this->releaser = NULL;
	this->metadataChange = NULL;
	this->committer = NULL;
	this->currentOperation = PreStart;
}


Transaction * NodeJoiner::getTransaction() {
	return this;
}

void NodeJoiner::initSession(){
	TransactionSession * session = new TransactionSession();
	ShardManager::getReadview(session->clusterReadview);
	session->response = new JsonResponseHandler();
}

ShardingTransactionType NodeJoiner::getTransactionType(){
	return ShardingTransactionType_NodeJoin;
}
bool NodeJoiner::run(){
	lock();
	return true;
}


void NodeJoiner::lock(){ // locks the metadata to be safe to read it
    Logger::debug("STEP : locking the metadata.");
	vector<NodeId> olderNodes;
	getOlderNodesList(olderNodes);
	//lock should be acquired on all nodes
	locker = new AtomicLock(selfOperationId, this, olderNodes); // X-locks metadata by default
	releaser = new AtomicRelease(selfOperationId, this);
	Logger::debug("Acquiring lock on metadata ...");
	this->currentOperation = Lock;
	locker->produce();
}

// coming back from lock
void NodeJoiner::consume(bool granted){
    switch (this->currentOperation) {
        case Lock:
            if(! granted){
                ASSERT(false);
                Logger::error("New node could not join the cluster.");
                finalize(false);
            }else{
                readMetadata();
            }
            break;
        case ReadMetadata:
            ASSERT(false);
            break;
        case Commit:
            if(! granted){
                Logger::debug("New node booting a fresh cluster because commit operation failed.");
                ASSERT(false);
                finalize(false);
                return;
            }else{
                release();
            }
            break;
        case Release:
            if(! granted){
                Logger::debug("New node booting a fresh cluster because release operation failed.");
                ASSERT(false);
                finalize(false);
                return;
            }else{
                finalize();
            }
            break;
        default:
            Logger::debug("New node booting a fresh cluster because of unknown reason.");
            ASSERT(false); //
            finalize(false);
            break;
    }
}

void NodeJoiner::readMetadata(){ // read the metadata of the cluster
	Logger::debug("STEP : Reading metadata writeview ...");
	// send read_metadata notification to the smallest node id
	vector<NodeId> olderNodes;
	getOlderNodesList(olderNodes);

	if(olderNodes.size() == 0){
		Logger::info("New node booting up a fresh cluster ...");
		finalize(false);
		return;
	}

	srand(time(NULL));
	this->randomNodeToReadFrom = olderNodes.at(rand() % olderNodes.size());

	Logger::debug("DETAILS : Node joiner going to read metadata from node %d ...", this->randomNodeToReadFrom);
	ConcurrentNotifOperation * reader = new ConcurrentNotifOperation(readMetadataNotif,
			ShardingNewNodeReadMetadataReplyMessageType, randomNodeToReadFrom, this);
	this->currentOperation = ReadMetadata;
	// committer is deallocated in state-machine so we don't have to have the
	// pointer. Before deleting committer, state-machine calls it's getMainTransactionId()
	// which calls lastCallback from its consumer
	ShardManager::getShardManager()->getStateMachine()->registerOperation(reader);
}

bool NodeJoiner::shouldAbort(const NodeId & failedNode){
	if(randomNodeToReadFrom == failedNode){
		readMetadata();
		return true;
	}
	return false;
}
// if returns true, operation must stop and return null to state_machine
void NodeJoiner::end_(map<NodeOperationId, SP(ShardingNotification) > & replies){
	if(replies.size() != 1){
		ASSERT(false);
		finalize(false);
		return;
	}
	ASSERT(replies.begin()->first == randomNodeToReadFrom);
	Logger::debug("STEP : Node Joiner : metadata is read from node %d", randomNodeToReadFrom);
	SP(MetadataReport) metadataReport = boost::dynamic_pointer_cast<MetadataReport>(replies.begin()->second);
	Cluster_Writeview * clusterWriteview = metadataReport->getWriteview();
	if(clusterWriteview == NULL){
		ASSERT(false);
		//use metadata initializer to make sure no partition remains unassigned.
		finalize(false);
		return;
	}
	Cluster_Writeview * currentWriteview = ShardManager::getWriteview();
	// attach data pointers of current writeview to the new writeview coming from
	// the minID node.
	currentWriteview->fixClusterMetadataOfAnotherNode(clusterWriteview);

	// new writeview is ready, replace current writeview with the new one
	ShardManager::getShardManager()->getMetadataManager()->setWriteview(clusterWriteview);
	ShardManager::getShardManager()->getMetadataManager()->commitClusterMetadata();
	Logger::debug("STEP : Node joiner : Metadata initialized from the cluster.");
	// ready to commit.
	commit();
}

void NodeJoiner::commit(){
	Logger::debug("STEP : Node Joiner : Committing the new node change to the cluster ...");
	// prepare the commit operation
	Cluster_Writeview * writeview = ShardManager::getWriteview();
	vector<ClusterShardId> localClusterShards;
	vector<NodeShardId> nodeShardIds;
	ClusterShardId id;
	NodeShardId nodeShardId;
	ShardState state;
	bool isLocal;
	NodeId nodeId;
	LocalPhysicalShard physicalShard;
	double load;
	ClusterShardIterator cShardItr(writeview);
	cShardItr.beginClusterShardsIteration();
	while(cShardItr.getNextLocalClusterShard(id, load, physicalShard)){
		localClusterShards.push_back(id);
	}

	NodeShardIterator nShardItr(writeview);
	nShardItr.beginNodeShardsIteration();
	while(nShardItr.getNextLocalNodeShard(nodeShardId, load, physicalShard)){
		nodeShardIds.push_back(nodeShardId);
	}

	NodeAddChange * nodeAddChange =
			new NodeAddChange(ShardManager::getCurrentNodeId(),localClusterShards, nodeShardIds);
	vector<NodeId> olderNodes;
	getOlderNodesList(olderNodes);
	this->committer = new AtomicMetadataCommit(nodeAddChange,  olderNodes, this, true);
	this->currentOperation = Commit;
	this->committer->produce();
}

void NodeJoiner::release(){ // releases the lock on metadata
	ASSERT(! this->releaseModeFlag);
	this->currentOperation = Release;
    Logger::debug("STEP : Node Joiner : Releasing lock ...");
	this->releaser->produce();
}

void NodeJoiner::finalize(bool result){
	if(! result){
		ShardManager::getShardManager()->initFirstNode();
		Logger::error("New node booting up as a single node.");
	}else{
	// release is also done.
	// just setJoined and done.
	this->finalizedFlag = true;
	ShardManager::getShardManager()->setJoined();
	Logger::info("Joined to the cluster.");
	}
	// so state machine will deallocate this transaction
	this->setFinished();
}

void NodeJoiner::getOlderNodesList(vector<NodeId> & olderNodes){
	Cluster_Writeview * currentWriteview = ShardManager::getWriteview();
	for(map<NodeId, std::pair<ShardingNodeState, Node *> >::const_iterator nodeItr = currentWriteview->nodes.begin();
			nodeItr != currentWriteview->nodes.end(); ++nodeItr){
		if(nodeItr->first >= currentWriteview->currentNodeId){
			continue;
		}
		if(nodeItr->second.first == ShardingNodeStateFailed){
			continue;
		}
		ASSERT(nodeItr->second.second != NULL); // we must have seen this node
		olderNodes.push_back(nodeItr->first);
	}
	std::sort(olderNodes.begin(), olderNodes.end());

	stringstream ss;
	for(unsigned i = 0 ; i < olderNodes.size(); ++i){
		if(i != 0){
			ss << ",";
		}
		ss << olderNodes.at(i);
	}
	Logger::debug("Sending list of nodes : %s with the lock request.", ss.str().c_str());

	ASSERT(olderNodes.size() > 0 &&
			olderNodes.at(olderNodes.size()-1) < ShardManager::getCurrentNodeId());
}

}
}