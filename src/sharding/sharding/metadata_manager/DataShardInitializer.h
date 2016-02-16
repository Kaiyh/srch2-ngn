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
#ifndef __SHARDING_SHARDING_DATA_SHARD_INITIALIZER_H__
#define __SHARDING_SHARDING_DATA_SHARD_INITIALIZER_H__

#include "Shard.h"
#include "Cluster_Writeview.h"
#include "../ShardManager.h"
#include "../../configuration/CoreInfo.h"
#include "server/Srch2Server.h"
#include "core/util/Logger.h"
#include "core/operation/IndexData.h"//for debug purpose

#include <boost/shared_ptr.hpp>
#include <boost/serialization/shared_ptr.hpp>


namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

class InitialShardHandler{
public:
	InitialShardHandler(ShardId * shardId){
		this->shardId = shardId;
		successFlag = false;
	}
	virtual void prepare(bool shouldLockWriteview = true) = 0 ;
	virtual ~InitialShardHandler(){
		if(shardId != NULL){
			delete shardId;
		}
	};
	bool isSuccessful() const{
		return this->successFlag;
	}
	boost::shared_ptr<Srch2Server> getShardServer() const{
		return server;
	}
protected:
	bool successFlag;
	boost::shared_ptr<Srch2Server> server;
	ShardId * shardId;
};
class InitialShardLoader : public InitialShardHandler{
public:
	InitialShardLoader(ShardId * shardId, const string & indexDirectory):InitialShardHandler(shardId),
	indexDirectory(indexDirectory){};
	void prepare(bool shouldLockWriteview = true){
		boost::shared_lock<boost::shared_mutex> sLock;
		const CoreInfo_t * indexDataConfig = NULL;
		if(shouldLockWriteview){
			indexDataConfig = ShardManager::getWriteview_read(sLock)->cores.at(shardId->coreId);
		}else{
			indexDataConfig = ShardManager::getWriteview_nolock()->cores.at(shardId->coreId);
		}

		server = boost::shared_ptr<Srch2Server>(new Srch2Server(indexDataConfig, indexDirectory, ""));

		//load the index from the data source
	    try{
	        server->init();
	        if(server->__debugShardingInfo != NULL){
				server->__debugShardingInfo->shardName = shardId->toString();
				stringstream ss;
				ss << "result of shard loader, indexDirectory : " << indexDirectory << " with " <<
            			server->getIndexer()->getNumberOfDocumentsInIndex() << " records.";
				server->__debugShardingInfo->explanation = ss.str();
	        }
	    } catch(exception& ex) {
	        /*
	         *  We got some fatal error during server initialization. Print the error
	         *  message and exit the process.
	         *
	         *  Note: Other internal modules should make sure that no recoverable
	         *        exception reaches this point. All exceptions that reach here are
	         *        considered fatal and the server will stop.
	         */
	        srch2::util::Logger::error(ex.what());
	        exit(-1);
	    }
	    this->successFlag = true;
	}
private:
	const string indexDirectory;
};

class InitialShardBuilder : public InitialShardHandler{
public:
	InitialShardBuilder(ShardId * shardId, const string & indexDirectory,
			const string & jsonFileCompletePath):
		InitialShardHandler(shardId),
		indexDirectory(indexDirectory), jsonFileCompletePath(jsonFileCompletePath){};
	void prepare(bool shouldLockWriteview = true){
		boost::shared_lock<boost::shared_mutex> sLock;
		const CoreInfo_t * indexDataConfig = NULL;
		if(shouldLockWriteview){
			indexDataConfig = ShardManager::getWriteview_read(sLock)->cores.at(shardId->coreId);
		}else{
			indexDataConfig = ShardManager::getWriteview_nolock()->cores.at(shardId->coreId);
		}

		server = boost::shared_ptr<Srch2Server>(new Srch2Server(indexDataConfig, indexDirectory, jsonFileCompletePath));

		//load the index from the data source
	    try{
	        server->init();
	        if(server->__debugShardingInfo != NULL){
				server->__debugShardingInfo->shardName = shardId->toString();
				stringstream ss;
				ss << "result of initial shard builder, indexDirectory : " << indexDirectory << " with " <<
            			server->getIndexer()->getNumberOfDocumentsInIndex() << " records.";
				server->__debugShardingInfo->explanation = ss.str();

	        }
	    } catch(exception& ex) {
	        /*
	         *  We got some fatal error during server initialization. Print the error
	         *  message and exit the process.
	         *
	         *  Note: Other internal modules should make sure that no recoverable
	         *        exception reaches this point. All exceptions that reach here are
	         *        considered fatal and the server will stop.
	         */
	        srch2::util::Logger::error(ex.what());
	        exit(-1);
	    }
	    this->successFlag = true;
	}
	string getJsonFileCompletePath() const{
		return jsonFileCompletePath;
	}
	string getIndexDirectory() const{
		return indexDirectory;
	}
private:
	const string indexDirectory;
	const string jsonFileCompletePath ;
};

class EmptyShardBuilder : public InitialShardHandler{
public:
	EmptyShardBuilder(ShardId * shardId, const string & indexDirectory):
		InitialShardHandler(shardId),
		indexDirectory(indexDirectory){};
	void prepare(bool shouldLockWriteview = true){
		boost::shared_lock<boost::shared_mutex> sLock;
		const CoreInfo_t * indexDataConfig = NULL;
		if(shouldLockWriteview){
			indexDataConfig = ShardManager::getWriteview_read(sLock)->cores.at(shardId->coreId);
		}else{
			indexDataConfig = ShardManager::getWriteview_nolock()->cores.at(shardId->coreId);
		}

		server = boost::shared_ptr<Srch2Server>(new Srch2Server(indexDataConfig, indexDirectory, ""));

		//load the index from the data source
	    try{
	        server->init();
	        if(server->__debugShardingInfo != NULL){
				server->__debugShardingInfo->shardName = shardId->toString();
				stringstream ss;
				ss << "result of empty shard builder, indexDirectory : " << indexDirectory << " .";
				server->__debugShardingInfo->explanation = ss.str();
	        }
	    } catch(exception& ex) {
	        /*
	         *  We got some fatal error during server initialization. Print the error
	         *  message and exit the process.
	         *
	         *  Note: Other internal modules should make sure that no recoverable
	         *        exception reaches this point. All exceptions that reach here are
	         *        considered fatal and the server will stop.
	         */
	        srch2::util::Logger::error(ex.what());
	        exit(-1);
	    }
	    this->successFlag = true;
	}
	string getIndexDirectory() const{
		return indexDirectory;
	}
private:
	const string indexDirectory;
};

}
}

#endif // __SHARDING_SHARDING_DATA_SHARD_INITIALIZER_H__
