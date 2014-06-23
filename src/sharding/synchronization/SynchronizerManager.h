/*
 * SynchronizerManager.h
 *
 *  Created on: Apr 20, 2014
 *      Author: srch2
 */

#ifndef __SHARDING_SYNCHRONIZERMANAGER_H__
#define __SHARDING_SYNCHRONIZERMANAGER_H__

#include "configuration/ConfigManager.h"
#include "transport/TransportManager.h"
#include <utility>
#include <queue>
#include "util/Assert.h"
#include <boost/thread.hpp>
#include <boost/unordered_map.hpp>
#include <iostream>
using namespace std;

namespace srch2 {
namespace httpwrapper {

#define FETCH_UNSIGNED(x) *((unsigned *)(x))
#define MSG_QUEUE_ARRAY_SIZE 1024

class SMCallBackHandler;
class MessageHandler;


/*
 *   Entry point for the synchronizer thread. void * => Synchronizer *
 */
void *bootSynchronizer(void *arg) ;

/*
 *  This is a main class in SM module which provide synchronization facilities.
 *
 *  1- Node failure detection
 *  2- Master election on master node failure
 *  3- Keeping consistent cluster information among all nodes in cluster.
 *
 */
class SyncManager {
	friend class ClientMessageHandler;
	friend class MasterMessageHandler;
public:
	/*
	 *  Initialize internal state.
	 */
	SyncManager(ConfigManager& cm, TransportManager& tm);
	/*
	 *   Free resources.
	 */
	virtual ~SyncManager();
	/*
	 *  Main entry point for SyncManager. Starts Master/Client heartbeat logic.
	 *  Note: should be called in new thread otherwise current thread will block
	 */
	void run();


	// Moved from ConfigManager to here by Jamshid, will move to MM
	// this code removes node from the cluster
	void removeNodeFromCluster(unsigned nodeId) {
		Cluster * cluster = config.getClusterWriteView();
		std::map<Node *, std::vector<CoreShardContainer * > > * shardInformation =
				cluster->getShardInformation();
		for(std::map<Node *, std::vector<CoreShardContainer * > >::iterator shardInfoItr = shardInformation->begin();
				shardInfoItr != shardInformation->end(); ++shardInfoItr){
			if(shardInfoItr->first->getId() == nodeId){
				// delete node
				delete shardInfoItr->first;
				for(unsigned cid = 0 ; cid < shardInfoItr->second.size(); ++cid){
					delete shardInfoItr->second.at(cid);
				}
				shardInformation->erase(shardInfoItr);
				// commit provides the changes writeview to readers
				config.commitClusterMetadata();
				return;
			}
		}
		ASSERT(false);
	}
private:
	///
	///  Private member functions start here.
	///
	/*
	 *   fetch timeout interval for SM messages.
	 */
	unsigned getTimeout() { return initialTimeout ; }      // temp for V0 replace with pingTimeout in V1
	void resetTimeout() {// temp for V0
		initialTimeout = pingTimeout;
	}
	//void refresh() {};
	//unsigned findNextEligibleMaster();
	/*
	 *   Send heartbeat request to all nodes in cluster
	 */
	void sendHeartBeatToAllNodesInCluster();

	/*
	 *   Wrapper around TM route function. Sets SM flag
	 */
	void route(NodeId node, Message *msg);

	/*
	 *   Check whether master node belongs to majority portion of the
	 *   cluster.
	 */
	bool hasMajority();

	///
	///  Private member variables start here.
	///

	bool isCurrentNodeMaster;
	unsigned currentNodeId;
	unsigned pingInterval;
	unsigned pingTimeout;
	unsigned masterNodeId;
	unsigned initialTimeout;
//	Cluster *cluster; // commented out by Jamshid and replaced with next line
	boost::shared_ptr<const Cluster> cluster;
	TransportManager& transport;
	SMCallBackHandler *callBackHandler;
	MessageHandler *messageHandler;
	ConfigManager& config;
	std::vector<Node> nodesInCluster;

};

class SMCallBackHandler : public CallBackHandler{
public:
	/*
	 *   The function gets Message form TM and process it based on the message type.
	 *
	 *   1. Master heart beat message is stored with it arrival timestamp.
	 *   2. Client message is stored in a per message queue array.
	 *
	 */
	void resolveMessage(Message * msg, NodeId node);

	/*
	 *  Constructor
	 */
	SMCallBackHandler(bool isMaster);

	/*
	 *  Remove message from node's queue.
	 */
	void removeMessageFromQueue(unsigned nodeId);

	/*
	 *  Remove message from node's queue.
	 */
	void getHeartBeatMessages(Message**msg);

	/*
	 *  Get heartbeat message's timestamp.
	 */
	std::time_t getHeartBeatMessageTime();

private:
	bool isMaster;
	// Last timestamp when heartbeat message was recieved from master
	std::time_t heartbeatMessageTimeEntry;
	// should keep the latest one only.
	Message* heartbeatMessage;
	boost::mutex hbLock;
public:
	// per Client entry
	struct MessageQ{
		std::queue<Message *> messageQueue;
		boost::mutex qGuard;
	} messageQArray[MSG_QUEUE_ARRAY_SIZE];

	MessageAllocator msgAllocator;
};

/*
 *   The abstract class which provides a general interface for handling
 *   messages.
 */
class MessageHandler {
public:
	MessageHandler(SyncManager* sm) { _syncMgrObj = sm; }

	/*
	 *   Any kind of failure should be handled in this function.
	 */
	virtual void handleFailure(Message *message) = 0;
	/*
	 *   Timeout case should be handled in this function.
	 */
	virtual void handleTimeOut(Message *message)  = 0;
	/*
	 *   Success case should be handled in this function.
	 */
	virtual void handleMessage(Message *message)  = 0;
	/*
	 *   The function should handle main logic of processing
	 *   messages delivered by TM.
	 */
	virtual void lookForCallbackMessages(SMCallBackHandler*) = 0;

	virtual ~MessageHandler() {}
protected:
	SyncManager *_syncMgrObj;
};

enum ClientNodeState {
	MASTER_AVAILABLE,
	MASTER_UNAVAILABLE
};

/*
 *   This class implements the client node's message handling
 */
class ClientMessageHandler : public MessageHandler{
public:
	ClientMessageHandler(SyncManager* sm);
	/*
	 *   The function handles the main logic of processing
	 *   messages delivered by TM.
	 */
	void lookForCallbackMessages(SMCallBackHandler*);

	void handleFailure(Message *message) {
		ASSERT(false);
	}

	virtual void handleTimeOut(Message *message);

	virtual void handleMessage(Message *message) ;

private:
	void startMasterElection();

	void processHeartBeat(Message *message);

	void handleElectionRequest(Message *message);

	void handleMasterProposalReject(Message *message) {
		// Proposed master rejected the message. Possible reasons.
		// 1. May be it was not eligible for master
		// 2. It already has a master
		// 3. Could not find enough vote in a given time to be elected as master.
		// For V0 do nothing  TODO: v1
		ASSERT(false);
	}
	void updateClusterState(Message *message);

	ClientNodeState nodeState;
	MessageAllocator cMessageAllocator;
	bool itselfInitiatedMasterElection;
	std::set<unsigned> voters;
};

class MasterMessageHandler : public MessageHandler{
public:
	MasterMessageHandler(SyncManager *sm): MessageHandler(sm) { firstTime = true;}
	/*
	 *   The function should handle main logic of processing
	 *   messages delivered by TM.
	 */
	void lookForCallbackMessages(SMCallBackHandler*);

	void handleFailure(Message *message) {
		ASSERT(false);
	}
	virtual void handleTimeOut(Message *message) {
		ASSERT(false);
	}

	virtual void handleMessage(Message *message);

private:
	bool firstTime;  // temp for V0
	void updateNodeInCluster(Message *message);
	void handleNodeFailure(unsigned nodeId);
	// key = Node id , Value = Latest time when message was received from this node.
	boost::unordered_map<unsigned, unsigned>  perNodeTimeStampEntry;

};

//class SerializerInterface {
//	virtual void serialize(void *object, char **byte, unsigned* size) = 0;
//	virtual void * Deserialize(char *byte, unsigned size) = 0;
//	virtual ~SerializerInterface() {}
//};
//
//class NodeSerializer : public SerializerInterface {
//	virtual void serialize(void *object, char **byte, unsigned* size){
//		Node * node = (Node *) object;
//
//	}
//};
//
//class ClusterSerializer : public SerializerInterface {
//
//};
//
//class ShardSerializer : public SerializerInterface {
//
//};





} /* namespace instantsearch */
} /* namespace srch2 */
#endif /* SYNCHRONIZERMANAGER_H_ */