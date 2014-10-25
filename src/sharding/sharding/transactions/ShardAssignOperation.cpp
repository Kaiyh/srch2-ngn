#include "ShardAssignOperation.h"

#include "core/util/SerializationHelper.h"
#include "core/util/Assert.h"
#include "../../configuration/ShardingConstants.h"
#include "../metadata_manager/Cluster_Writeview.h"
#include "../ShardManager.h"
#include "../metadata_manager/DataShardInitializer.h"
#include "../state_machine/node_iterators/ConcurrentNotifOperation.h"
#include "./AtomicRelease.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

ShardAssignOperation::ShardAssignOperation(const ClusterShardId & unassignedShard,
		ConsumerInterface * consumer):ProducerConsumerInterface(consumer), shardId(unassignedShard){
	this->locker = NULL;
	this->releaser = NULL;
	this->committer = NULL;
	this->successFlag = true;
	this->currentAction = PreStart;
}

ShardAssignOperation::~ShardAssignOperation(){
	if(this->locker != NULL){
		delete this->locker;
	}
	if(this->releaser != NULL){
		delete this->releaser;
	}
	if(this->committer != NULL){
		delete this->committer;
	}
}

void ShardAssignOperation::produce(){
	lock();
}

void ShardAssignOperation::lock(){ // ** start **
	this->locker = new AtomicLock(shardId , currentOpId, LockLevel_X, this);
	// locker calls all methods of LockResultCallbackInterface from us
	this->releaser = new AtomicRelease(shardId, currentOpId , this);
	// releaser calls all methods of BooleanCallbackInterface from us
	currentAction = Lock;
	this->locker->produce();
}
// for lock
void ShardAssignOperation::consume(bool granted){
	switch (currentAction) {
		case Lock:
			if(granted){
				commit();
			}else{
				this->successFlag = false;
				release();
			}
			break;
		case Commit:
			ASSERT(granted);
			release();
			break;
		case Release:
			ASSERT(granted);
			finalize();
			break;
		default:
			ASSERT(false);
			finalize();
			break;
	}
}
// ** if (granted)
void ShardAssignOperation::commit(){
	// start metadata commit
	// prepare the shard change
	Cluster_Writeview * writeview = ShardManager::getWriteview();
	string indexDirectory = ShardManager::getShardManager()->getConfigManager()->getShardDir(writeview->clusterName,
			writeview->cores[shardId.coreId]->getName(), &shardId);
	if(indexDirectory.compare("") == 0){
		indexDirectory = ShardManager::getShardManager()->getConfigManager()->createShardDir(writeview->clusterName,
				writeview->cores[shardId.coreId]->getName(), &shardId);
	}
	EmptyShardBuilder emptyShard(new ClusterShardId(shardId), indexDirectory);
	emptyShard.prepare();
	LocalPhysicalShard physicalShard(emptyShard.getShardServer(), emptyShard.getIndexDirectory(), "");
	// prepare the shard change
	ShardAssignChange * shardAssignChange = new ShardAssignChange(shardId, ShardManager::getCurrentNodeId(), 0);
	shardAssignChange->setPhysicalShard(physicalShard);
	currentAction = Commit;
	this->committer = new AtomicMetadataCommit(shardAssignChange,  vector<NodeId>(), this);
	this->committer->produce();
}
// ** end if
void ShardAssignOperation::release(){
	// release the locks
	currentAction = Release;
	this->releaser->produce();
}

void ShardAssignOperation::finalize(){ // ** return **
	this->getConsumer()->consume(this->successFlag);
}

}
}

