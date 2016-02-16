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
#ifndef __SHARDING_SHARDING_ACL_RECORD_COMMAND_HTTP_H__
#define __SHARDING_SHARDING_ACL_RECORD_COMMAND_HTTP_H__

#include "../../state_machine/State.h"
#include "../../notifications/Notification.h"
#include "../../metadata_manager/Shard.h"

#include "core/util/Logger.h"
#include "core/util/Assert.h"
#include "server/HTTPJsonResponse.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

/*
 *  HTTP handler for ACL record APIs.
 */
class AclRecordCommandHttpHandler: public ReadviewTransaction, public ConsumerInterface {
public:

    static void runCommand(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
            evhttp_request *req, unsigned coreId, srch2is::RecordAclCommandType commandType){

    	SP(AclRecordCommandHttpHandler) aclCommandHttpHandler =
    			SP(AclRecordCommandHttpHandler)( new AclRecordCommandHttpHandler(
        		clusterReadview, req, coreId, commandType)); //
        Transaction::startTransaction(aclCommandHttpHandler);
        return ;
    }

    ~AclRecordCommandHttpHandler(){
    	finalize();
    	delete aclCommand;
    }

    static void prepareAclDataForApiLayer(std::map< string, vector<string> > &recordAclDataForApiLayer,
    		const string primaryKey, const vector<string>& roleIdsList) {
    	std::map< string, vector<string> >::iterator iter =
    			recordAclDataForApiLayer.find(primaryKey);
    	if (iter != recordAclDataForApiLayer.end()) {
    		vector<string> unionList;
    		std::set_union(iter->second.begin(), iter->second.end(), roleIdsList.begin(),
    				roleIdsList.end(), back_inserter(unionList));
    		iter->second.assign(unionList.begin(), unionList.end());
    	} else {
    		recordAclDataForApiLayer.insert(make_pair(primaryKey, roleIdsList));
    	}
    }

private:
    AclRecordCommandHttpHandler(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
            evhttp_request *req, unsigned coreId, srch2is::RecordAclCommandType commandType /*, and maybe other arguments */):ReadviewTransaction(clusterReadview){
    	this->req = req;
    	this->coreInfo = clusterReadview->getCore(coreId);
    	ASSERT(this->coreInfo != NULL);
        aclCommand = NULL;
        recordAclCommandType = commandType;
    }

    /*
     * The main work of AclCommandHttpHandler starts in this function
     * Example of this work : parsing req object and get HTTP req information.
     *
     */
    void run(){
    	JsonResponseHandler * responseObject = this->getTransaction()->getSession()->response;
    	if(coreInfo == NULL){
    		responseObject->addError(JsonResponseHandler::getJsonSingleMessage(HTTP_JSON_Core_Does_Not_Exist));
    		responseObject->finalizeOK();
    		return ;
    	}

    	std::map< string, vector<string> > recordAclDataForApiLayer;

    	if(coreInfo->getHasRecordAcl()){ // this core has record Acl

    		size_t length = EVBUFFER_LENGTH(req->input_buffer);

    		if (length == 0) {
    			responseObject->addError(JsonResponseHandler::getJsonSingleMessage(HTTP_JSON_Empty_Body));
    			responseObject->finalizeOK();
    			return ;
    		}

    		const char *post_data = (char *) EVBUFFER_DATA(req->input_buffer);

    		// Parse example data
    		Json::Value root;
    		Json::Reader reader;
    		bool parseSuccess = reader.parse(post_data, root, false);

    		if (parseSuccess == false) {
    			responseObject->addError(JsonResponseHandler::getJsonSingleMessage(HTTP_JSON_Parse_Error));
    			responseObject->finalizeOK();
    			return ;
    		}else{
    			if(root.type() == Json::arrayValue) { // The input is an array of JSON objects.
    				for ( int index = 0; index < root.size(); ++index ) {
    					Json::Value defaultValueToReturn = Json::Value("");
    					const Json::Value doc = root.get(index,
    							defaultValueToReturn);
        				vector<string> roleIds;
    					string primaryKeyID;
    					std::stringstream log_str;
    					// extract all the role ids from the query
    					bool success = JSONRecordParser::_extractResourceAndRoleIds(roleIds, primaryKeyID,
    							doc, coreInfo, log_str);
    					if (success) {
    						HTTPPrintInfo hpi = { primaryKeyID, true, "" };
    						inputRecordInfoArr.push_back(hpi);
    						prepareAclDataForApiLayer(recordAclDataForApiLayer, primaryKeyID, roleIds);
    					} else {
    						HTTPPrintInfo hpi = { primaryKeyID, false,  log_str.str()};
    						inputRecordInfoArr.push_back(hpi);
    					}
    				}
    			}else{ // The input is only one JSON object.
    				const Json::Value doc = root;
    				vector<string> roleIds;
    				string primaryKeyID;
    				std::stringstream log_str;
    				// extract all the role ids from the query
    				bool success = JSONRecordParser::_extractResourceAndRoleIds(roleIds, primaryKeyID, doc, coreInfo, log_str);
    				if(!success){
    					responseObject->addMessage("error:" + coreInfo->getName() + " : "  + log_str.str());
    					responseObject->finalizeOK();
    					return ;
    				} else {
    					HTTPPrintInfo hpi = { primaryKeyID, true, "" };
    					inputRecordInfoArr.push_back(hpi);
    					prepareAclDataForApiLayer(recordAclDataForApiLayer, primaryKeyID, roleIds);
    				}
    			}
    		}

    	}else{
			responseObject->addMessage("error:" + coreInfo->getName() + " does not have record-based access control.");
			responseObject->finalizeOK();
			return ;
    	}

        // When query parameters are parsed successfully, we must create and run AclCommand class and get back
        // its response in a 'consume' callback function.

    	aclCommand = new WriteCommand(this, recordAclDataForApiLayer, recordAclCommandType, coreInfo);

        aclCommand->produce();

        return;
    }

    /*
     * One example of consume callback that will be called by the producer class
     * If another set of arguments is needed for this module, a new consume method must be
     * added to ConsumerInterface, it must be overridden here to process the results of AclCommand
     * and it must be called in finalize method of AclCommand to return back to this consumer.
     */
    void consume(const map<string, bool> & results,
			map<string, map<ShardId * ,vector<JsonMessageCode>, ShardPtrComparator > > & messageCodes){
    	//print per record result
    	typedef map<ShardId * ,vector<JsonMessageCode>, ShardPtrComparator > MessageCodes;

		string actionName;
		switch (recordAclCommandType) {
			case Acl_Record_Add:
				actionName = string(c_action_acl_record_add);
				break;
			case Acl_Record_Append:
				actionName = string(c_action_acl_record_append);
				break;
			case Acl_Record_Delete:
				actionName = string(c_action_acl_record_delete);
				break;
		}

    	JsonResponseHandler * responseObject = this->getTransaction()->getSession()->response;
    	for (unsigned i = 0; i < inputRecordInfoArr.size(); ++i) {
			const string& pk = inputRecordInfoArr[i].primaryKey;
			bool httpLayerSuccess = inputRecordInfoArr[i].httpLayerSuccess;

			Json::Value recordShardResponse =
					JsonRecordOperationResponse::getRecordJsonResponse(pk, actionName.c_str(),
							httpLayerSuccess, coreInfo->getName());

    		if (httpLayerSuccess) {
    			map<string, bool>::const_iterator iter = results.find(pk);
    			if ( iter != results.end()) {
    				recordShardResponse[c_status] = iter->second;;
    				if(messageCodes.find(iter->first) != messageCodes.end()){
    					MessageCodes &primaryKeyMessageCode = messageCodes[iter->first];
    					for(MessageCodes::iterator shardItr =
    							primaryKeyMessageCode.begin(); shardItr != primaryKeyMessageCode.end(); ++shardItr){
    						JsonRecordOperationResponse::addRecordMessages(recordShardResponse, shardItr->second);
    					}
    				} else {
    					ASSERT(false);
    				}
    			} else {
    				ASSERT(false);
    			}
    		} else {
    			JsonRecordOperationResponse::addRecordError(recordShardResponse,
    					HTTP_JSON_Custom_Error, inputRecordInfoArr[i].httpLayerMsg);
    		}

    		JsonRecordOperationResponse * responseChannel = (JsonRecordOperationResponse *) this->getSession()->response;
    		responseChannel->addRecordShardResponse(recordShardResponse);
    	}
    }

    void finalizeWork(Transaction::Params * params){
		this->getSession()->response->printHTTP(req);
    }

    /*
     * This function must be overridden for each transaction class so that producers can use the
     * transaction and it's getSession() inteface.
     */
    SP(Transaction) getTransaction() {
        return sharedPointer;
    }

    ShardingTransactionType getTransactionType(){
        return ShardingTransactionType_RecordAclCommandCode; // returns the unique type identifier of this transaction
    }

    string getName() const {return "record-acl-command-http" ;};


private:
    WriteCommand * aclCommand;
    evhttp_request *req;
    const CoreInfo_t * coreInfo;
    srch2::instantsearch::RecordAclCommandType recordAclCommandType;
    struct HTTPPrintInfo{
    	string primaryKey;
    	bool httpLayerSuccess;
    	string httpLayerMsg;
    };
	vector<HTTPPrintInfo> inputRecordInfoArr;
};


}
}



#endif // __SHARDING_SHARDING_ACL_RECORD_COMMAND_HTTP_H__
