#ifndef __SHARDING_SHARDING_TRNASACTION_TRANSACTION_SESSION_H__
#define __SHARDING_SHARDING_TRNASACTION_TRANSACTION_SESSION_H__


#include "core/util/Assert.h"
#include "core/util/Logger.h"
#include "include/instantsearch/Constants.h"
#include "../metadata_manager/Cluster.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

class JsonResponseHandler;

class TransactionSession {
public:

//	void setResponse(JsonResponseHandler * response){
//		this->response = response;
//	}

	JsonResponseHandler * response;

	boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview;

private:
};

}
}

#endif // __SHARDING_SHARDING_TRNASACTION_TRANSACTION_SESSION_H__