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
#include "AclAttributeReadNotification.h"
#include "core/util/Assert.h"
#include "core/util/Logger.h"
#include "../../sharding/ShardManager.h"
#include "sharding/processor/DistributedProcessorInternal.h"
#include "../state_machine/StateMachine.h"

using namespace std;
namespace srch2 {
namespace httpwrapper {


AclAttributeReadNotification::AclAttributeReadNotification(const string & roleId,
		const NodeTargetShardInfo & target,
		boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview){
	this->roleId = roleId;
	this->target = target;
	this->clusterReadview = clusterReadview;
}
AclAttributeReadNotification::AclAttributeReadNotification(){
	ShardManager::getReadview(clusterReadview);
}

bool AclAttributeReadNotification::resolveNotification(SP(ShardingNotification) _notif){
	SP(AclAttributeReadNotification::ACK) response =
			ShardManager::getShardManager()->getDPInternal()->
			resolveAclAttributeListRead(boost::dynamic_pointer_cast<AclAttributeReadNotification>(_notif));
	if(! response){
		response = create<AclAttributeReadNotification::ACK>();
	}
    response->setSrc(_notif->getDest());
    response->setDest(_notif->getSrc());
	send(response);
	return true;
}

void * AclAttributeReadNotification::serializeBody(void * buffer) const{
	buffer = target.serialize(buffer);
	buffer = srch2::util::serializeString(roleId, buffer);
	return buffer;
}

unsigned AclAttributeReadNotification::getNumberOfBytesBody() const{
	unsigned numberOfBytes = 0;
	numberOfBytes += target.getNumberOfBytes();
	numberOfBytes += srch2::util::getNumberOfBytesString(roleId);
	return numberOfBytes;
}

void * AclAttributeReadNotification::deserializeBody(void * buffer) {
	buffer = target.deserialize(buffer);
	buffer = srch2::util::deserializeString(buffer, roleId);
	return buffer;
}

const NodeTargetShardInfo & AclAttributeReadNotification::getTarget(){
	return target;
}

const string & AclAttributeReadNotification::getRoleId() const {
	return roleId;
}

boost::shared_ptr<const ClusterResourceMetadata_Readview> AclAttributeReadNotification::getReadview() const{
	return clusterReadview;
}



void * AclAttributeReadNotification::ACK::serializeBody(void * buffer) const{
	buffer = srch2::util::serializeVectorOfFixedTypes(listOfRefiningAttributes, buffer);
	buffer = srch2::util::serializeVectorOfFixedTypes(listOfSearchableAttributes, buffer);
	return buffer;
}
unsigned AclAttributeReadNotification::ACK::getNumberOfBytesBody() const{
	unsigned numberOfBytes = 0;
	numberOfBytes += srch2::util::getNumberOfBytesVectorOfFixedTypes(listOfRefiningAttributes);
	numberOfBytes += srch2::util::getNumberOfBytesVectorOfFixedTypes(listOfSearchableAttributes);
	return numberOfBytes;
}
void * AclAttributeReadNotification::ACK::deserializeBody(void * buffer) {
	buffer = srch2::util::deserializeVectorOfFixedTypes(buffer, listOfRefiningAttributes);
	buffer = srch2::util::deserializeVectorOfFixedTypes(buffer, listOfSearchableAttributes);
	return buffer;
}
bool AclAttributeReadNotification::ACK::resolveNotification(SP(ShardingNotification) _notif){
	ShardManager::getStateMachine()->handle(_notif);
	return true;
}
vector<unsigned> & AclAttributeReadNotification::ACK::getListOfRefiningAttributes(){
	return listOfRefiningAttributes;
}
vector<unsigned> & AclAttributeReadNotification::ACK::getListOfSearchableAttributes(){
	return listOfSearchableAttributes;
}

void AclAttributeReadNotification::ACK::setListOfRefiningAttributes(vector<unsigned> & list) {
	listOfRefiningAttributes.assign(list.begin(), list.end());
}
void AclAttributeReadNotification::ACK::setListOfSearchableAttributes(vector<unsigned> & list) {
	listOfSearchableAttributes.assign(list.begin(), list.end());
}


}
}
