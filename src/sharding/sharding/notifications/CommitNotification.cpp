#include "CommitNotification.h"
#include "../ShardManager.h"
#include "../metadata_manager/ResourceMetadataManager.h"
#include "../state_machine/StateMachine.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


CommitNotification::CommitNotification(MetadataChange * metadataChange){
	ASSERT(metadataChange != NULL);
	this->metadataChange = metadataChange;
}

CommitNotification::CommitNotification(){
	metadataChange = NULL;
};
CommitNotification::~CommitNotification(){
}


void * CommitNotification::serializeBody(void * buffer) const{
	ASSERT(metadataChange != NULL);
	buffer = srch2::util::serializeFixedTypes(metadataChange->getType(), buffer);
	buffer = metadataChange->serialize(buffer);
	return buffer;
}
unsigned CommitNotification::getNumberOfBytesBody() const{
	unsigned numberOfBytes = 0;
	numberOfBytes += sizeof(MetadataChangeType);
	numberOfBytes += metadataChange->getNumberOfBytes();
	return numberOfBytes;
}
void * CommitNotification::deserializeBody(void * buffer){
	MetadataChangeType type;
	buffer = srch2::util::deserializeFixedTypes(buffer, type);
	switch (type) {
		case ShardingChangeTypeNodeAdd:
			metadataChange = new NodeAddChange();
			break;
		case ShardingChangeTypeShardAssign:
			metadataChange = new ShardAssignChange();
			break;
		case ShardingChangeTypeShardMove:
			metadataChange = new ShardMoveChange();
			break;
		case ShardingChangeTypeLoadChange:
			metadataChange = new ShardLoadChange();
			break;
		default:
			ASSERT(false);
			break;
	}
	buffer = metadataChange->deserialize(buffer);
	return buffer;
}

bool CommitNotification::CommitNotification::resolveNotification(SP(ShardingNotification) notif){
	ShardManager::getShardManager()->getMetadataManager()->resolve(boost::dynamic_pointer_cast<CommitNotification>(notif));
	return true;
}

MetadataChange * CommitNotification::getMetadataChange() const{
	return this->metadataChange;
}


//Returns the type of message which uses this kind of object as transport
ShardingMessageType CommitNotification::messageType() const{
	return ShardingCommitMessageType;
}

bool CommitNotification::operator==(const CommitNotification & right){
	return *metadataChange == *(right.metadataChange);
}

bool CommitNotification::ACK::operator==(const CommitNotification::ACK & right){
	return true;
}

bool CommitNotification::ACK::resolveNotification(SP(ShardingNotification) ack){
	ShardManager::getShardManager()->getStateMachine()->handle(ack);
	return true;
}

}
}