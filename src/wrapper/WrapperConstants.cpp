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
#include "WrapperConstants.h"

#include "../core/util/Assert.h"
using namespace srch2::instantsearch;
namespace srch2 {
namespace httpwrapper {

std::map<JsonMessageCode, std::string> __jsonMessageCodeStrings = boost::assign::map_list_of
		(HTTP_JSON_Parse_Error, "JSON object parse error")
		(HTTP_JSON_Empty_Body, "Http body is empty.")
		(HTTP_JSON_All_Shards_Down_Error, "No data shard is available for this request.")
		(HTTP_JSON_Custom_Error, "Custom Error.")
		(HTTP_JSON_Node_Timeout_Warning, "Node timeout.")
		(HTTP_JSON_PK_Exists_Error, "The record with same primary key already exists")
		(HTTP_JSON_Doc_Limit_Reached_Error, "Document limit reached. Email support@srch2.com for account upgrade.")
		(HTTP_JSON_Existing_Record_Update_Info, "Existing record updated successfully.")
		(HTTP_JSON_Update_Failed_Error, "Record update failed.")
		(HTTP_JSON_Recover_Failed_Error, "Existing record with the same primary key deleted but failed to recover.")
		(HTTP_JSON_Delete_Record_Not_Found_Error, "No record with given primary key")
		(HTTP_JSON_ResetLogger_Reopen_Failed_Error, "The logger file repointing failed. Could not create new logger file")
		(HTTP_JSON_PK_NOT_PROVIDED, "Primary key not provided.")
		(HTTP_JSON_Search_Res_Format_Wrong_Error, "The indexed data failed to export to disk, The request need to set search-response-format to be 0 or 2")
		(HTTP_JSON_Merge_Already_Done_Info, "No records require merge at this time. Merge is up-to-date.")
		(HTTP_JSON_Merge_DISABLED, "Merge is disabled. Use 'set' parameter to enable it.")
        (HTTP_JSON_Commit_Already_Done_Info, "Commit is finished. No need to commit anymore.")
        (HTTP_JSON_Cluster_Not_Ready_Error, "Cluster is not ready to accept this request now, please try again later.")
        (HTTP_JSON_Merge_Parameter_Not_Recognized, "Parameter not recognized.")
        (HTTP_JSON_Request_Rejected_Due_To_Load_Balancing, "Request rejected due to ongoing load-balancing task.")
        (HTTP_JSON_Request_ACL_Not_Available, "Core does not have record-based access control.")
        (HTTP_Json_DUP_PRIMARY_KEY, "Primary key repeats in the same batch, first one is used.")
        (HTTP_Json_Partition_Is_Locked, "The corresponding partition of data is not usable now. Please try again later.")
        (HTTP_Json_No_Data_Shard_Available_For_Write, "No data shard is currently available for this record. This is mostly caused by node failure. Please try again later.")
        (HTTP_JSON_Core_Does_Not_Exist, "Core name is not recognized.")
        (HTTP_JSON_PK_Does_Not_Exist, "Primary key provided does not exist.")
        (HTTP_Json_General_Error, "Unknown Error.")
        (HTTP_Json_Role_Id_Does_Not_Exist, "Role ID does not exist.")
        (HTTP_Json_Failed_Due_To_Node_Failure, "Node failure.")
        (HTTP_Json_Data_File_Does_Not_Exist, "Data file does not exist.")
        (HTTP_Json_Node_Failure, "Node failure.")
        (HTTP_Json_Cannot_Acquire_Locks, "Could not acquire necessary locks. Please try again later.")
        (HTTP_Json_Cannot_Save_Data, "Could not save indices. Please try again.")
        (HTTP_Json_NOT_SUPPORTED, "Operation not supported.");

const std::string getJsonMessageCodeStr(const JsonMessageCode code){
	if(__jsonMessageCodeStrings.find(code) == __jsonMessageCodeStrings.end()){
		ASSERT(false);
		return "Unknown Error.";
	}else{
		return __jsonMessageCodeStrings[code];
	}
}

}
}
