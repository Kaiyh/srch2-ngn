/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "CopyToMeNotification.h"
#include "LoadBalancingReport.h"
#include "MoveToMeNotification.h"
#include "../ShardManager.h"
#include "../state_machine/StateMachine.h"
#include "../metadata_manager/Cluster_Writeview.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

bool CopyToMeNotification::resolveNotification(SP(ShardingNotification) _copyNotif){
	// call migration manager to start transfering this shard.
	SP(CopyToMeNotification) copyNotif = boost::dynamic_pointer_cast<CopyToMeNotification>(_copyNotif);
	ClusterShardId replicaShardId = copyNotif->getReplicaShardId();
	ClusterShardId unassignedShardId = copyNotif->getUnassignedShardId();
	boost::unique_lock<boost::shared_mutex> xLock;
	Cluster_Writeview * writeview = ShardManager::getWriteview_write(xLock);
	if(writeview->localClusterDataShards.find(replicaShardId) == writeview->localClusterDataShards.end()){
		// NOTE: requested shard does not exist on this node.Notif
		ASSERT(false);
		return false;
	}
	// srch2Server does not need to be const
	boost::shared_ptr<Srch2Server> server = writeview->localClusterDataShards.at(replicaShardId).server;
	xLock.unlock();

	ShardManager::getShardManager()->getMigrationManager()->migrateShard(
			replicaShardId, server , unassignedShardId,
			copyNotif->getDest(), copyNotif->getSrc());

	SP(CopyToMeNotification::ACK) ack = ShardingNotification::create<CopyToMeNotification::ACK>();
	ack->setSrc(copyNotif->getDest());
	ack->setDest(copyNotif->getSrc());
	ShardingNotification::send(ack);
	return true;
}

void * CopyToMeNotification::serializeBody(void * buffer) const{
	buffer = replicaShardId.serialize(buffer);
    buffer = unassignedShardId.serialize(buffer);
	return buffer;
}
unsigned CopyToMeNotification::getNumberOfBytesBody() const{
	unsigned numberOfBytes = 0;
	numberOfBytes += replicaShardId.getNumberOfBytes();
    numberOfBytes += unassignedShardId.getNumberOfBytes();
	return numberOfBytes;
}
void * CopyToMeNotification::deserializeBody(void * buffer) {
	buffer = replicaShardId.deserialize(buffer);
    buffer = unassignedShardId.deserialize(buffer);
	return buffer;
}
ShardingMessageType CopyToMeNotification::messageType() const{
	return ShardingCopyToMeMessageType;
}
ClusterShardId CopyToMeNotification::getReplicaShardId() const{
	return replicaShardId;
}
ClusterShardId CopyToMeNotification::getUnassignedShardId() const{
    return unassignedShardId;
}
bool CopyToMeNotification::operator==(const CopyToMeNotification & right){
	return replicaShardId == right.replicaShardId && unassignedShardId == right.unassignedShardId;
}

bool CopyToMeNotification::ACK::resolveNotification(SP(ShardingNotification) _notif){
	ShardManager::getShardManager()->getStateMachine()->handle(_notif);
	return true;
}

bool LoadBalancingReport::resolveNotification(SP(ShardingNotification) _notif){
	ShardManager::getShardManager()->getStateMachine()->handle(_notif);
	return true;
}


ShardingMessageType LoadBalancingReport::messageType() const{
	return ShardingLoadBalancingReportMessageType;
}
void * LoadBalancingReport::serializeBody(void * buffer) const{
	buffer = srch2::util::serializeFixedTypes(load, buffer);
	return buffer;
}
unsigned LoadBalancingReport::getNumberOfBytesBody() const{
	unsigned numberOfBytes = 0;
	numberOfBytes += srch2::util::getNumberOfBytesFixedTypes(load);
	return numberOfBytes;
}
void * LoadBalancingReport::deserializeBody(void * buffer){
	buffer = srch2::util::deserializeFixedTypes(buffer, load);
	return buffer;
}
double LoadBalancingReport::getLoad() const{
	return this->load;
}


bool LoadBalancingReport::REQUEST::resolveNotification(SP(ShardingNotification) _notif){
	boost::unique_lock<boost::shared_mutex> xLock;
	Cluster_Writeview * writeview = ShardManager::getWriteview_write(xLock);
	SP(LoadBalancingReport) report =
			SP(LoadBalancingReport)(new LoadBalancingReport(writeview->getLocalNodeTotalLoad()));
	xLock.unlock();
	report->setDest(_notif->getSrc());
	report->setSrc(NodeOperationId(ShardManager::getCurrentNodeId()));
	ShardingNotification::send(report);
	return true;
}


bool MoveToMeNotification::resolveNotification(SP(ShardingNotification) _notif){

	SP(MoveToMeNotification) moveNotif = boost::dynamic_pointer_cast<MoveToMeNotification>(_notif);
	boost::unique_lock<boost::shared_mutex> xLock;
	Cluster_Writeview * writeview = ShardManager::getWriteview_write(xLock);

	if(writeview->localClusterDataShards.find(moveNotif->getShardId()) == writeview->localClusterDataShards.end()){
		// NOTE : requested shard does not exist on this node
		ASSERT(false);
		return false;
	}

	boost::shared_ptr<Srch2Server> server = writeview->localClusterDataShards.at(moveNotif->getShardId()).server;

	xLock.unlock();
	// call migration manager to transfer this shard
	ShardManager::getShardManager()->getMigrationManager()->migrateShard(moveNotif->getShardId(),
			server , moveNotif->getShardId(),moveNotif->getDest(), moveNotif->getSrc());

	SP(MoveToMeNotification::ACK) ack = ShardingNotification::create<MoveToMeNotification::ACK>();
	ack->setSrc(moveNotif->getDest());
	ack->setDest(moveNotif->getSrc());
	ShardingNotification::send(ack);

	return true;
}


bool MoveToMeNotification::ACK::resolveNotification(SP(ShardingNotification) _notif){

	ShardManager::getShardManager()->getStateMachine()->handle(_notif);
	return true;
}

bool MoveToMeNotification::CleanUp::resolveNotification(SP(ShardingNotification) _notif){
	// start the cleanup process
	// TODO
	return true;
}


}
}
