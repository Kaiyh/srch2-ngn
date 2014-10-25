#include "LockingNotification.h"

#include "../ShardManager.h"
#include "../state_machine/StateMachine.h"
#include "core/util/SerializationHelper.h"
#include "core/util/Assert.h"
#include "../metadata_manager/Cluster_Writeview.h"
#include "../metadata_manager/Cluster.h"
#include "../metadata_manager/ResourceMetadataManager.h"
#include "../lock_manager/LockManager.h"


#include <sstream>

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


LockingNotification::LockingNotification(const ClusterShardId & srcShardId,
		const ClusterShardId & destShardId, const NodeOperationId & copyAgent, const bool releaseRequest):
		lockRequestType(LockRequestType_Copy),
		releaseRequestFlag(releaseRequest),
		blocking(false || releaseRequest){
	this->srcShardId = srcShardId;
	this->destShardId = destShardId;
	this->copyAgent = copyAgent;
}
LockingNotification::LockingNotification(const ClusterShardId & shardId,
		const NodeOperationId & srcMoveAgent, const NodeOperationId & destMoveAgent, const bool releaseRequest):
		lockRequestType(LockRequestType_Move),
		releaseRequestFlag(releaseRequest),
		blocking(false || releaseRequest){
	this->shardId = shardId;
	this->srcMoveAgent = srcMoveAgent;
	this->destMoveAgent = destMoveAgent;
}
LockingNotification::LockingNotification(const NodeOperationId & newNodeOpId,
		const vector<NodeId> & listOfOlderNodes,
		const LockLevel & lockLevel,
		const bool blocking,
		const bool releaseRequest):
		lockRequestType(LockRequestType_Metadata),
		releaseRequestFlag(releaseRequest),
		blocking(blocking || releaseRequest){
	this->newNodeOpId = newNodeOpId;
	this->listOfOlderNodes = listOfOlderNodes;
	this->metadataLockLevel = lockLevel;

    // debug print
    stringstream ss;
    for(vector<NodeId>::iterator nodeItr = this->listOfOlderNodes.begin(); nodeItr != this->listOfOlderNodes.end(); ++nodeItr){
        ss << *nodeItr << " - ";
    }
	Logger::debug("DETAILS : LockingNotification : newNodeOpId(%s), olderNodes(%s), lockLevel(%d)", newNodeOpId.toString().c_str(), ss.str().c_str(), metadataLockLevel);
}
LockingNotification::LockingNotification(const vector<string> & primaryKeys,
		const NodeOperationId & writerAgent, const ClusterPID & pid, const bool releaseRequest):
		lockRequestType(LockRequestType_PrimaryKey),
		releaseRequestFlag(releaseRequest),
		blocking(true || releaseRequest){
	this->primaryKeys = primaryKeys;
	this->writerAgent = writerAgent;
	this->pid = pid;
}

LockingNotification::LockingNotification(const ClusterShardId & shardId,
		const NodeOperationId & agent, const LockLevel & lockLevel):
		lockRequestType(LockRequestType_GeneralPurpose),
		releaseRequestFlag(false),
		blocking(false || releaseRequestFlag){
	this->generalPurposeShardId = shardId;
	this->generalPurposeAgent = agent;
	this->generalPurposeLockLevel = lockLevel;
}

LockingNotification::LockingNotification(const ClusterShardId & shardId,
		const NodeOperationId & agent):
		lockRequestType(LockRequestType_GeneralPurpose),
		releaseRequestFlag(true),
		blocking(true || releaseRequestFlag) { // release is always blocking
	this->generalPurposeShardId = shardId;
	this->generalPurposeAgent = agent;
}

LockingNotification::LockingNotification(const vector<ClusterShardId> & shardIdList,
		const NodeOperationId & shardIdListLockHolder, const LockLevel & lockLevel):
		lockRequestType(LockRequestType_ShardIdList),
		releaseRequestFlag(false),
		blocking(true || releaseRequestFlag) {
	this->shardIdList = shardIdList;
	this->shardIdListLockHolder = shardIdListLockHolder;
	this->shardIdListLockLevel = lockLevel;
}
LockingNotification::LockingNotification(const vector<ClusterShardId> & shardIdList,
		const NodeOperationId & shardIdListLockHolder):
		lockRequestType(LockRequestType_ShardIdList),
		releaseRequestFlag(true),
		blocking(true || releaseRequestFlag){ // release is always blocking
	this->shardIdList = shardIdList;
	this->shardIdListLockHolder = shardIdListLockHolder;
}

LockingNotification::LockingNotification():
				lockRequestType(LockRequestType_GeneralPurpose),
				releaseRequestFlag(true),
				blocking(true || releaseRequestFlag){}

bool LockingNotification::resolveNotification(SP(ShardingNotification) _notif){
	ShardManager::getShardManager()->_getLockManager()->resolve(boost::dynamic_pointer_cast<LockingNotification>(_notif));
	return true;
}

void * LockingNotification::serializeBody(void * buffer) const{
	buffer = srch2::util::serializeFixedTypes(lockRequestType, buffer);
	buffer = srch2::util::serializeFixedTypes(blocking, buffer);
	buffer = srch2::util::serializeFixedTypes(releaseRequestFlag, buffer);
	switch(lockRequestType){
	case LockRequestType_Copy:
		buffer = srcShardId.serialize(buffer);
		buffer = destShardId.serialize(buffer);
		buffer = copyAgent.serialize(buffer);
		break;
	case LockRequestType_Move:
		buffer = shardId.serialize(buffer);
		buffer = srcMoveAgent.serialize(buffer);
		buffer = destMoveAgent.serialize(buffer);
		break;
	case LockRequestType_Metadata:
		buffer = newNodeOpId.serialize(buffer);
		buffer = srch2::util::serializeVectorOfFixedTypes(listOfOlderNodes, buffer);
		buffer = srch2::util::serializeFixedTypes(metadataLockLevel, buffer);
		break;
	case LockRequestType_PrimaryKey:
		buffer = srch2::util::serializeVectorOfString(primaryKeys , buffer);
		buffer = writerAgent.serialize(buffer);
		buffer = pid.serialize(buffer);
		break;
	case LockRequestType_GeneralPurpose:
		buffer = generalPurposeShardId.serialize(buffer);
		buffer = generalPurposeAgent.serialize(buffer);
		buffer = srch2::util::serializeFixedTypes(generalPurposeLockLevel, buffer);
		break;
	case LockRequestType_ShardIdList:
		buffer = srch2::util::serializeVectorOfDynamicTypes(shardIdList, buffer);
		buffer = shardIdListLockHolder.serialize(buffer);
		buffer = srch2::util::serializeFixedTypes(shardIdListLockLevel, buffer);
		break;
	}
	return buffer;
}
unsigned LockingNotification::getNumberOfBytesBody() const{
	unsigned numberOfBytes = 0 ;
	numberOfBytes += sizeof(lockRequestType);
	numberOfBytes += sizeof(blocking);
	numberOfBytes += sizeof(releaseRequestFlag);
	switch(lockRequestType){
	case LockRequestType_Copy:
		numberOfBytes += srcShardId.getNumberOfBytes();
		numberOfBytes += destShardId.getNumberOfBytes();
		numberOfBytes += copyAgent.getNumberOfBytes();
		break;
	case LockRequestType_Move:
		numberOfBytes += shardId.getNumberOfBytes();
		numberOfBytes += srcMoveAgent.getNumberOfBytes();
		numberOfBytes += destMoveAgent.getNumberOfBytes();
		break;
	case LockRequestType_Metadata:
		numberOfBytes += newNodeOpId.getNumberOfBytes();
		numberOfBytes += srch2::util::getNumberOfBytesVectorOfFixedTypes(listOfOlderNodes);
		numberOfBytes += sizeof(metadataLockLevel);
		break;
	case LockRequestType_PrimaryKey:
		numberOfBytes += srch2::util::getNumberOfBytesVectorOfString(primaryKeys);
		numberOfBytes += writerAgent.getNumberOfBytes();
		numberOfBytes += pid.getNumberOfBytes();
		break;
	case LockRequestType_GeneralPurpose:
		numberOfBytes += generalPurposeShardId.getNumberOfBytes();
		numberOfBytes += generalPurposeAgent.getNumberOfBytes();
		numberOfBytes += sizeof(generalPurposeLockLevel);
		break;
	case LockRequestType_ShardIdList:
		numberOfBytes += srch2::util::getNumberOfBytesVectorOfDynamicTypes(shardIdList);
		numberOfBytes += shardIdListLockHolder.getNumberOfBytes();
		numberOfBytes += sizeof(shardIdListLockLevel);
		break;
	}
	return numberOfBytes;
}
void * LockingNotification::deserializeBody(void * buffer){
	buffer = srch2::util::deserializeFixedTypes(buffer, lockRequestType);
	buffer = srch2::util::deserializeFixedTypes(buffer, blocking);
	buffer = srch2::util::deserializeFixedTypes(buffer, releaseRequestFlag);
	switch(lockRequestType){
	case LockRequestType_Copy:
		buffer = srcShardId.deserialize(buffer);
		buffer = destShardId.deserialize(buffer);
		buffer = copyAgent.deserialize(buffer);
		break;
	case LockRequestType_Move:
		buffer = shardId.deserialize(buffer);
		buffer = srcMoveAgent.deserialize(buffer);
		buffer = destMoveAgent.deserialize(buffer);
		break;
	case LockRequestType_Metadata:
		buffer = newNodeOpId.deserialize(buffer);
		buffer = srch2::util::deserializeVectorOfFixedTypes(buffer, listOfOlderNodes);
		buffer = srch2::util::deserializeFixedTypes(buffer, metadataLockLevel);
		break;
	case LockRequestType_PrimaryKey:
		buffer = srch2::util::deserializeVectorOfString( buffer, primaryKeys);
		buffer = writerAgent.deserialize(buffer);
		buffer = pid.deserialize(buffer);
		break;
	case LockRequestType_GeneralPurpose:
		buffer = generalPurposeShardId.deserialize(buffer);
		buffer = generalPurposeAgent.deserialize(buffer);
		buffer = srch2::util::deserializeFixedTypes(buffer, generalPurposeLockLevel);
		break;
	case LockRequestType_ShardIdList:
		buffer = srch2::util::deserializeVectorOfDynamicTypes(buffer, shardIdList);
		buffer = shardIdListLockHolder.deserialize(buffer);
		buffer = srch2::util::deserializeFixedTypes(buffer, shardIdListLockLevel);
		break;
	}
	return buffer;
}

ShardingMessageType LockingNotification::messageType() const{
	return ShardingLockMessageType;
}

bool LockingNotification::operator==(const LockingNotification & lockingNotification){
	if(lockRequestType != lockingNotification.lockRequestType){
		return false;
	}
	if(blocking != lockingNotification.blocking){
		return false;
	}
	if(releaseRequestFlag != lockingNotification.releaseRequestFlag){
		return false;
	}
	switch(lockRequestType){
	case LockRequestType_Copy:
		return (srcShardId == lockingNotification.srcShardId) &&
				(destShardId == lockingNotification.destShardId) &&
				(copyAgent == lockingNotification.copyAgent);

	case LockRequestType_Move:
		return (shardId == lockingNotification.shardId) &&
				(srcMoveAgent == lockingNotification.srcMoveAgent) &&
				(destMoveAgent == lockingNotification.destMoveAgent);
	case LockRequestType_Metadata:
		return (newNodeOpId == lockingNotification.newNodeOpId) &&
				(listOfOlderNodes == lockingNotification.listOfOlderNodes) &&
				(metadataLockLevel == lockingNotification.metadataLockLevel);
	case LockRequestType_PrimaryKey:
		return (primaryKeys == lockingNotification.primaryKeys) &&
				(writerAgent == lockingNotification.writerAgent) &&
				(pid == lockingNotification.pid);

	case LockRequestType_GeneralPurpose:
		return (generalPurposeShardId == lockingNotification.generalPurposeShardId) &&
				(generalPurposeAgent == lockingNotification.generalPurposeAgent) &&
				(generalPurposeLockLevel == lockingNotification.generalPurposeLockLevel);

	case LockRequestType_ShardIdList:
		return (shardIdList == lockingNotification.shardIdList) &&
				(shardIdListLockHolder == lockingNotification.shardIdListLockHolder) &&
				(shardIdListLockLevel == lockingNotification.shardIdListLockLevel);
	}
	return false;
}

string LockingNotification::toString(){
	stringstream ss;
	ss << "LockingNotification (";
	ss << "blocking:" << blocking << "): ";
	if(! releaseRequestFlag){
		ss << "LOCK : ";
	}else{
		ss << "RLS : ";
	}
	switch(lockRequestType){
	case LockRequestType_Copy:
		ss << "cp" << srcShardId.toString() << " to " << destShardId.toString() << " by " << copyAgent.toString();
		break;
	case LockRequestType_Move:
		ss << "mv" << shardId.toString() << " from " << srcMoveAgent.toString() << " to " << destMoveAgent.toString();
		break;
	case LockRequestType_Metadata:
		switch(metadataLockLevel){
		case LockLevel_S:
			ss << "S";
			break;
		case LockLevel_X:
			ss << "X";
			break;
		}
		ss << " lock for ";
		ss << newNodeOpId.toString() << " on metadata, list of older nodes : (";
		for(unsigned i = 0 ; i < listOfOlderNodes.size() ; ++i){
			ss << listOfOlderNodes.at(i);
			if(i != listOfOlderNodes.size() -1){
				ss << ",";
			}
		}
		ss << ")";
		break;
	case LockRequestType_PrimaryKey:
		ss << writerAgent.toString() << " requesting lock for (";
		for(unsigned i = 0 ; i < primaryKeys.size(); ++i){
			if(i != 0){
				ss << ",";
			}
			ss << primaryKeys.at(i);
		}
		ss << ") from partition " << pid.toString() << ".";
		break;
	case LockRequestType_GeneralPurpose:
		ss << generalPurposeAgent.toString() << " requesting ";
		switch(generalPurposeLockLevel){
		case LockLevel_S:
			ss << "S";
			break;
		case LockLevel_X:
			ss << "X";
			break;
		}
		ss << " lock for " << generalPurposeShardId.toString() << ", general purpose.";
		break;
	case LockRequestType_ShardIdList:
		ss << "List of shardIds : ";
		for(unsigned i = 0 ; i < shardIdList.size(); ++i){
			if(i != 0){
				ss << ",";
			}
			ss << shardIdList.at(i).toString();
		}
		ss << " requested by " << shardIdListLockHolder.toString() << " requesting ";
		switch(shardIdListLockLevel){
		case LockLevel_S:
			ss << "S";
			break;
		case LockLevel_X:
			ss << "X";
			break;
		}
		break;
	}
	return ss.str();
}


void LockingNotification::getLockRequestInfo(ClusterShardId & srcShardId, ClusterShardId & destShardId, NodeOperationId & copyAgent) const{
	srcShardId = this->srcShardId;
	destShardId = this->destShardId;
	copyAgent = this->copyAgent;
}
void LockingNotification::getLockRequestInfo(ClusterShardId & shardId, NodeOperationId & srcMoveAgent, NodeOperationId & destMoveAgent) const{
	shardId = this->shardId;
	srcMoveAgent = this->srcMoveAgent;
	destMoveAgent = this->destMoveAgent;
}
void LockingNotification::getLockRequestInfo(NodeOperationId & newNodeOpId,
		vector<NodeId> & listOfOlderNodes, LockLevel & lockLevel) const{
	newNodeOpId = this->newNodeOpId;
	listOfOlderNodes = this->listOfOlderNodes;
	lockLevel = this->metadataLockLevel;
}
void LockingNotification::getLockRequestInfo(vector<string> & primaryKeys, NodeOperationId & writerAgent, ClusterPID & pid) const{
	primaryKeys = this->primaryKeys;
	writerAgent = this->writerAgent;
	pid = this->pid;
}

void LockingNotification::getLockRequestInfo(ClusterShardId & shardId, NodeOperationId & agent, LockLevel & lockLevel) const{
	shardId = this->generalPurposeShardId;
	agent = this->generalPurposeAgent;
	lockLevel = this->generalPurposeLockLevel;
}

void LockingNotification::getLockRequestInfo(vector<ClusterShardId> & shardIdList, NodeOperationId & shardIdListLockHolder,
		LockLevel & shardIdListLockLevel) const{
	shardIdList = this->shardIdList;
	shardIdListLockHolder = this->shardIdListLockHolder;
	shardIdListLockLevel = this->shardIdListLockLevel;
}

vector<string> & LockingNotification::getPrimaryKeys(){
	return primaryKeys;
}
NodeOperationId LockingNotification::getWriterAgent() const{
	return writerAgent;
}

bool LockingNotification::isReleaseRequest() const{
	return releaseRequestFlag;
}

bool LockingNotification::isBlocking() const{
	return blocking;
}

LockRequestType LockingNotification::getType() const{
	return lockRequestType;
}


void LockingNotification::getInvolvedNodes(vector<NodeId> & participants) const{
	participants.clear();
	Cluster_Writeview * writeview = ShardManager::getShardManager()->getWriteview();
//	switch (lockRequestType) {
//	case LockRequestType_Copy:
//	{
//		// only those nodes that have a replica of this partition
//		writeview->getPatitionInvolvedNodes(srcShardId, participants);
//		if(std::find(participants.begin(), participants.end(), writeview->currentNodeId) == participants.end()){
//			participants.push_back(writeview->currentNodeId);
//		}
//		break;
//	}
//	case LockRequestType_Move:
//	{
//		writeview->getPatitionInvolvedNodes(shardId, participants);
//		if(std::find(participants.begin(), participants.end(), writeview->currentNodeId) == participants.end()){
//			participants.push_back(writeview->currentNodeId);
//		}
//		break;
//	}
//	case LockRequestType_PrimaryKey:
//		//TODO participants must be given from outside because it must work based on readview
//		break;
//	case LockRequestType_Metadata:
//	{
//		writeview->getArrivedNodes(participants, true);
//		break;
//	}
//	case LockRequestType_GeneralPurpose:
//	{
//		writeview->getPatitionInvolvedNodes(generalPurposeShardId, participants);
//		break;
//	}
//	}
	writeview->getArrivedNodes(participants, true);; // TODO : for now, we send the lock request to every node
}

LockingNotification::ACK::ACK(bool grantedFlag){
	this->granted = grantedFlag;
	this->indexOfLastGrantedItem = 0;
};

bool LockingNotification::ACK::resolveNotification(SP(ShardingNotification) ack){
	ShardManager::getShardManager()->getStateMachine()->handle(ack);
	return true;
}

ShardingMessageType LockingNotification::ACK::messageType() const{
	return ShardingLockACKMessageType;
}

void * LockingNotification::ACK::serializeBody(void * buffer) const{
	buffer = srch2::util::serializeFixedTypes(granted, buffer);
	buffer = srch2::util::serializeFixedTypes(indexOfLastGrantedItem, buffer);
	return buffer;
}
unsigned LockingNotification::ACK::getNumberOfBytesBody() const{
	unsigned numberOfBytes = 0;
	numberOfBytes += sizeof(granted);
	numberOfBytes += sizeof(indexOfLastGrantedItem);
	return numberOfBytes;
}
void * LockingNotification::ACK::deserializeBody(void * buffer){
	buffer = srch2::util::deserializeFixedTypes(buffer, granted);
	buffer = srch2::util::deserializeFixedTypes(buffer, indexOfLastGrantedItem);
	return buffer;
}

bool LockingNotification::ACK::isGranted() const{
	return granted;
}

void LockingNotification::ACK::setGranted(bool granted){
	this->granted = granted;
}

unsigned LockingNotification::ACK::getIndexOfLastGrantedItem() const{
	return indexOfLastGrantedItem;
}
void LockingNotification::ACK::setIndexOfLastGrantedItem(const unsigned index){
	this->indexOfLastGrantedItem = index;
}

bool LockingNotification::ACK::operator==(const LockingNotification::ACK & right){
	return granted == right.granted;
}

}
}