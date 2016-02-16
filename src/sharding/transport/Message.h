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
#ifndef __SHARDING_TRANSPORT_MESSAGE_H__
#define __SHARDING_TRANSPORT_MESSAGE_H__

#include "src/sharding/configuration/ShardingConstants.h"
#include "src/sharding/configuration/ConfigManager.h"
#include "src/core/util/SerializationHelper.h"

namespace srch2 {
namespace httpwrapper {

typedef uint32_t MessageID_t;

const int MSG_HEADER_CONST_SIZE = 17;

const char MSG_LOCAL_MASK = 0x1;        // 00000001
const char MSG_DISCOVERY_MASK = 0x2;    // 00000010
const char MSG_DP_REQUEST_MASK = 0x4;   // 00000100
const char MSG_DP_REPLY_MASK = 0x8;     // 00001000
const char MSG_SHARDING_MASK = 0x10;     // 00010000

const char MSG_MIGRATION_MASK = 0x40;  //01000000
class Message {

public:

   //helper Functions

   bool isValidMask(){
	   char maskBackup = this->getMask();
	   unsetMask(maskBackup, MSG_LOCAL_MASK);
	   unsetMask(maskBackup, MSG_DISCOVERY_MASK);
	   unsetMask(maskBackup, MSG_DP_REQUEST_MASK);
	   unsetMask(maskBackup, MSG_DP_REPLY_MASK);
	   unsetMask(maskBackup, MSG_SHARDING_MASK);
	   unsetMask(maskBackup, MSG_MIGRATION_MASK);
	   return (maskBackup == 0);
   }

   void unsetMask(char & mask, const char msg_mask) const{
	   mask &= ~(msg_mask);
   }

   bool isSMRelated(){
       char * mask = this->_getMask();
	 return (*mask == 0);
   }

   bool isLocal(){
       char * mask = this->_getMask();
     return *mask & MSG_LOCAL_MASK;
   }
   bool isDPRequest() {
       char * mask = this->_getMask();
     return *mask & MSG_DP_REQUEST_MASK;
   }
   bool isDPReply() {
       char * mask = this->_getMask();
     return *mask & MSG_DP_REPLY_MASK;
   }
   bool isDiscovery() {
       char * mask = this->_getMask();
     return *mask & MSG_DISCOVERY_MASK;
   }

   bool isMigration() {
       char * mask = this->_getMask();
	   return *mask & MSG_MIGRATION_MASK;
   }

   bool isSharding() {
       char * mask = this->_getMask();
     return *mask & MSG_SHARDING_MASK;
   }
   Message * setLocal(){
       char * mask = this->_getMask();
       *mask |= MSG_LOCAL_MASK;
	   return this;
   }
   Message * setDPRequestMask(){
       char * mask = this->_getMask();
       *mask |= MSG_DP_REQUEST_MASK;
	   return this;
   }
   Message * setDPReplyMask(){
       char * mask = this->_getMask();
       *mask |= MSG_DP_REPLY_MASK;
	   return this;
   }
   Message * setDiscoveryMask(){
       char * mask = this->_getMask();
       *mask |= MSG_DISCOVERY_MASK;
	   return this;
   }

   Message * setMigrationMask(){
	   char * mask = this->_getMask();
	   *mask |= MSG_MIGRATION_MASK;
	   return this;
   }

   Message * setShardingMask(){
	   char * mask = this->_getMask();
	   *mask |= MSG_SHARDING_MASK;
	   return this;
   }
   unsigned getBodySize(){
	   return *this->_getBodySize();
   }
   unsigned getTotalSize(){
	   return this->getBodySize() + sizeof(Message);
   }
   unsigned getHeaderSize(){
	   return sizeof(Message); // sizeof(Message bytes are used for the header but only the array conaines information - padding!)
   }
   void setMessageId(MessageID_t id){
	   MessageID_t * idRef = this->_getMessageId();
	   *idRef = id;
   }
   MessageID_t getMessageId(){
	   return *this->_getMessageId();
   }
   void setRequestMessageId(MessageID_t requestMessageId){
	   MessageID_t * requestMessageIdRef = this->_getReqMessageId();
	   *requestMessageIdRef = requestMessageId;
   }
   MessageID_t getRequestMessageId(){
	   return *this->_getReqMessageId();
   }
   void setBodyAndBodySize(void * src, unsigned bodySize){
	   setBodySize(bodySize);
	   if(*this->_getBodySize() == 0){
		   return;
	   }
	   memcpy(this->getMessageBody(), src, *this->_getBodySize());
   }
   ShardingMessageType getType(){
	   uint32_t intVar = *_getShardingMessageType();
	   return (ShardingMessageType)intVar;
   }
   void setType(ShardingMessageType type){
	   uint32_t * typeRef = (uint32_t *)_getShardingMessageType();
	   *typeRef = (uint32_t)type;
   }
   void setBodySize(unsigned bodySize){
	   unsigned int * bodySizeRef = _getBodySize();
	   *bodySizeRef = bodySize;
   }

   void setMask(char mask) {
	   char * maskRef = _getMask();
	   *maskRef = mask;
   }
   char getMask(){return *this->_getMask();};
   char * getMessageBody() {
	   return (char *)(body);
   }

   static Message * getMessagePointerFromBodyPointer( void * bodyPointer){
	   return (Message *)((char *)bodyPointer - sizeof(Message));
   }
   static char * getBodyPointerFromMessagePointer(Message * messagePointer){
	   return (char *)(messagePointer->body);
//	   return (char *)((char *)messagePointer + sizeof(Message));
   }

   void populateHeader(char * headerDataStart /* a byte array that containes MSG_HEADER_CONST_SIZE bytes which is the data of message */){
	   memcpy(headerData, headerDataStart, MSG_HEADER_CONST_SIZE);
   }

   inline char * getHeaderInfoStart(){
	   return headerData;
   }

   string getDescription(){
	   return string(getShardingMessageTypeStr((ShardingMessageType)*_getShardingMessageType()));
   }


private:
   inline uint32_t * _getShardingMessageType(){
	   return (uint32_t *)(headerData + _getShardingMessageTypeOffset());
   }
   inline unsigned _getShardingMessageTypeOffset(){
	   return 0;
   }
   inline char * _getMask(){
	   return headerData + _getMaskOffset();
   }
   inline unsigned _getMaskOffset(){
	   uint32_t intVar;
	   return _getShardingMessageTypeOffset() +
			   srch2::util::getNumberOfBytesFixedTypes(intVar);
   }
   inline uint32_t * _getBodySize(){
	   return (unsigned *)(headerData + _getBodySizeOffset());
   }
   inline unsigned _getBodySizeOffset(){
	   return _getMaskOffset() + sizeof(char);
   }
   inline uint32_t * _getMessageId(){
	   return (MessageID_t *)(headerData + _getMessageIdOffset());
   }
   inline unsigned _getMessageIdOffset(){
	   uint32_t intVar;
	   return _getBodySizeOffset() + srch2::util::getNumberOfBytesFixedTypes(intVar);
   }
   inline uint32_t * _getReqMessageId(){
	   return (MessageID_t *)(headerData + _getReqMessageIdOffset());
   }
   inline unsigned _getReqMessageIdOffset(){
	   uint32_t intVar;
	   return _getMessageIdOffset() + srch2::util::getNumberOfBytesFixedTypes(intVar);
   }
   // 4 + 1 + 4 + 4 + 4 = 17 = MSG_CONST_SIZE
   // because different architectures can have different alignments
   // we store all data of message header in a byte array and access them through getter methods
   // that return reference variables.
   // NOTE : this must be offset zero
   char headerData[MSG_HEADER_CONST_SIZE];
   /// now, we can always safely use the Message * cast technique
//   ShardingMessageType shardingMessageType;
//   char mask;
//   unsigned int bodySize;
//   MessageID_t id;
//   MessageID_t requestMessageId; //used by response type messages

   /*
    * NOTE : allocate must be like allocate( sizeof(Message) + length of body )
    *        but serialize and deserialize must be only on the headerData array because different
    *        platforms can have different paddings ...
    */
   char body[0]; // zero-length array, aka Struct Hack.
   // body acts as a pointer which always points to
   // end of the header message.
};

}
}

#endif // __SHARDING_ROUTING_MESSAGE_H__
